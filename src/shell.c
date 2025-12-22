#include "shell.h"
#include "text.h"
#include "ssh_client.h"
#include "input.h"
#include "display.h"
#include <string.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <stdio.h>
#include <vita2d.h>

#define MAX_INPUT_LENGTH 256

static int shell_active = 0;
static int show_keyboard = 0;
static int show_special_keys = 0;
static uint16_t keyboard_buffer[MAX_TEXT_LENGTH + 1] = {0};

static SceUID interactive_thread = -1;
static int interactive_thread_running = 0;

static char current_prompt[64] = "ssh> ";
static char current_username[64] = "";
static char current_hostname[64] = "";

typedef struct {
    const char *name;
    const char *sequence;
} SpecialKey;

static SpecialKey special_keys[] = {
    {"Ctrl+C", "\x03"},
    {"Ctrl+D", "\x04"},
    {"Ctrl+Z", "\x1A"},
    {"Ctrl+A", "\x01"},
    {"Ctrl+E", "\x05"},
    {"Ctrl+L", "\x0C"},
    {"Ctrl+R", "\x12"},
    {"Tab", "\x09"},
    {"Esc", "\x1B"},
    {"F1", "\x1BOP"},
    {"F2", "\x1BOQ"},
    {"F3", "\x1BOR"},
    {"F4", "\x1BOS"},
    {"F5", "\x1B[15~"},
    {"F6", "\x1B[17~"},
    {"F7", "\x1B[18~"},
    {"F8", "\x1B[19~"},
    {"F9", "\x1B[20~"},
    {"F10", "\x1B[21~"},
    {"F11", "\x1B[23~"},
    {"F12", "\x1B[24~"},
    {"Up Arrow", "\x1B[A"},
    {"Down Arrow", "\x1B[B"},
    {"Right Arrow", "\x1B[C"},
    {"Left Arrow", "\x1B[D"},
    {"Insert", "\x1B[2~"},
    {"Delete", "\x1B[3~"},
    {"Home", "\x1B[1~"},
    {"End", "\x1B[4~"},
    {"Page Up", "\x1B[5~"},
    {"Page Down", "\x1B[6~"}
};

static int special_keys_count = sizeof(special_keys) / sizeof(SpecialKey);

void shell_handle_resize() {
    set_terminal_size(80, 24);
    if (!shell_active) {
        terminal_print("Terminal size set to 80x24");
    }
}

static int interactive_reader_thread(SceSize args, void *argp) {
    interactive_thread_running = 1;
    char buffer[1024];

    
    while (interactive_thread_running && shell_active) {
        int n = interactive_shell_read(buffer, sizeof(buffer));

        if (n > 0) {
            terminal_process_data(buffer, n);
        } else if (n < 0 && n != LIBSSH2_ERROR_EAGAIN) {
            break;
        }

        sceKernelDelayThread(50000); // 50ms
    }

    interactive_thread_running = 0;
    return 0;
}

void shell_start_interactive() {
    if (start_interactive_shell() == 0) {
        shell_active = 1;
        show_keyboard = 0;
        show_special_keys = 0;

        terminal_init();
        
        set_interactive_mode(1);

        interactive_thread = sceKernelCreateThread("interactive_reader", 
                                                  interactive_reader_thread, 
                                                  0x10000100, 
                                                  0x10000, 
                                                  0, 
                                                  0, 
                                                  NULL);
        if (interactive_thread >= 0) {
            sceKernelStartThread(interactive_thread, 0, NULL);
        }

        sceKernelDelayThread(100000); // 100ms
        
    } else {
        terminal_print("Error starting interactive shell");
        shell_active = 0;
        set_interactive_mode(0);
    }
}

void shell_stop_interactive() {
    interactive_thread_running = 0;
    shell_active = 0;
    show_keyboard = 0;
    show_special_keys = 0;
    
    // Restaurar modo normal
    set_interactive_mode(0);

    if (interactive_thread >= 0) {
        sceKernelWaitThreadEnd(interactive_thread, NULL, NULL);
        sceKernelDeleteThread(interactive_thread);
        interactive_thread = -1;
    }

    stop_interactive_shell();
    terminal_print("Interactive shell closed");
}

void shell_set_prompt(const char *username, const char *hostname) {
    strncpy(current_username, username, sizeof(current_username) - 1);
    strncpy(current_hostname, hostname, sizeof(current_hostname) - 1);

    if (strlen(username) > 0 && strlen(hostname) > 0) {
        snprintf(current_prompt, sizeof(current_prompt), "%s@%s:~$ ", username, hostname);
    } else {
        strcpy(current_prompt, "ssh> ");
    }
}

int shell_is_active() {
    return shell_active;
}

void shell_send_special_key(const char *key_sequence) {
    if (shell_active) {
        interactive_shell_write(key_sequence, strlen(key_sequence));
    }
}

void shell_process_input() {
    if (show_special_keys) {
        char options[512] = "";
        for (int i = 0; i < special_keys_count; i++) {
            if (i > 0) strcat(options, ",");
            strcat(options, special_keys[i].name);
        }

        static uint16_t special_key_input[256] = {0};
        int result = kb(u"Select Special Key:", u"Ctrl+C", special_key_input, 0);

        if (result) {
            char selected_key[64];
            utf16_to_utf8(special_key_input, selected_key, sizeof(selected_key));

            for (int i = 0; i < special_keys_count; i++) {
                if (strcmp(selected_key, special_keys[i].name) == 0) {
                    shell_send_special_key(special_keys[i].sequence);
                    break;
                }
            }
        }

        show_special_keys = 0;
        return;
    }

    if (!show_keyboard) return;

    int result = kb(u"Shell Command:", u"", keyboard_buffer, 0);

    if (result) {
        char command[MAX_INPUT_LENGTH];
        utf16_to_utf8(keyboard_buffer, command, sizeof(command));

        if (strlen(command) > 0) {
            interactive_shell_write(command, strlen(command));
            interactive_shell_write("\n", 1);
        }

        memset(keyboard_buffer, 0, sizeof(keyboard_buffer));
    }

    show_keyboard = 0;
}

void shell_handle_controls(SceCtrlData *pad, unsigned int new_buttons) {
    if (!shell_active) return;

    if (new_buttons & SCE_CTRL_TRIANGLE && !show_keyboard && !show_special_keys) {
        show_keyboard = 1;
    }

    if (new_buttons & SCE_CTRL_SQUARE && !show_keyboard && !show_special_keys) {
        show_special_keys = 1;
    }

    if (new_buttons & SCE_CTRL_CROSS) {
        shell_send_special_key("\x03");
    }

    if (new_buttons & SCE_CTRL_CIRCLE) {
        shell_send_special_key("\x09");
    }

    if (new_buttons & SCE_CTRL_LTRIGGER) {
        shell_send_special_key("\x1B[A");
    }

    if (new_buttons & SCE_CTRL_RTRIGGER) {
        shell_send_special_key("\x1B[B");
    }

    if (new_buttons & SCE_CTRL_SELECT) {
        shell_handle_resize();
    }

    if (new_buttons & SCE_CTRL_START) {
        shell_stop_interactive();
    }
}

void shell_render(vita2d_pgf *font) {
    if (!shell_active) return;

    int term_x = TERM_X;
    int term_y = TERM_Y;
    int term_width = TERM_WIDTH;
    int term_height = TERM_HEIGHT;

    vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(60, 60, 60, 255));
    vita2d_draw_rectangle(term_x, term_y, term_width, term_height, RGBA8(0, 0, 0, 255));
    vita2d_draw_rectangle(term_x + 1, term_y + 1, term_width - 2, term_height - 2, RGBA8(30, 30, 30, 255));

    int corner_size = 15;
    int line_thickness = 2;

    vita2d_draw_rectangle(term_x, term_y, corner_size, line_thickness, RGBA8(0, 255, 0, 255));
    vita2d_draw_rectangle(term_x, term_y, line_thickness, corner_size, RGBA8(0, 255, 0, 255));
    vita2d_draw_rectangle(term_x + term_width - corner_size, term_y, corner_size, line_thickness, RGBA8(0, 255, 0, 255));
    vita2d_draw_rectangle(term_x + term_width - line_thickness, term_y, line_thickness, corner_size, RGBA8(0, 255, 0, 255));
    vita2d_draw_rectangle(term_x, term_y + term_height - line_thickness, corner_size, line_thickness, RGBA8(0, 255, 0, 255));
    vita2d_draw_rectangle(term_x, term_y + term_height - corner_size, line_thickness, corner_size, RGBA8(0, 255, 0, 255));
    vita2d_draw_rectangle(term_x + term_width - corner_size, term_y + term_height - line_thickness, corner_size, line_thickness, RGBA8(0, 255, 0, 255));
    vita2d_draw_rectangle(term_x + term_width - line_thickness, term_y + term_height - corner_size, line_thickness, corner_size, RGBA8(0, 255, 0, 255));

    terminal_render(font);

    int controls_y = term_y + term_height + 15;
    if (controls_y > 500) controls_y = 500;

    vita2d_draw_rectangle(0, controls_y - 10, 960, 544 - controls_y + 10, RGBA8(40, 40, 40, 255));

    char status[256];
    if (show_keyboard) {
        strcpy(status, "[Entering command...]");
    } else if (show_special_keys) {
        strcpy(status, "[Selecting special key...]");
    } else {
        strcpy(status, "△:Keyboard □:SpecialKeys X:Ctrl+C ○:Tab L/R:Arrows SELECT:Resize START:Exit");
    }

    vita2d_pgf_draw_text(font, 20, controls_y,
                        RGBA8(255, 255, 255, 255), 0.6f, status);
}
