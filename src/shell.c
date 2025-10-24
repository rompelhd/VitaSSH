#include "shell.h"
#include "text.h"
#include "ssh_client.h"
#include "input.h"
#include <string.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <stdio.h>
#include <vita2d.h>

#define MAX_INPUT_LENGTH 256

static int shell_active = 0;
static int show_keyboard = 0;
static uint16_t keyboard_buffer[MAX_TEXT_LENGTH + 1] = {0};

static SceUID interactive_thread = -1;
static int interactive_thread_running = 0;

static char current_prompt[64] = "ssh> ";
static char current_username[64] = "";
static char current_hostname[64] = "";

static int interactive_reader_thread(SceSize args, void *argp) {
    interactive_thread_running = 1;
    
    while (interactive_thread_running && shell_active) {
        char buffer[1024];
        int n = interactive_shell_read(buffer, sizeof(buffer));
        
        if (n > 0) {
            terminal_print(buffer);
        } else if (n < 0) {
            break;
        }
        
        sceKernelDelayThread(10000);
    }
    
    interactive_thread_running = 0;
    return 0;
}

void shell_start_interactive() {
    if (start_interactive_shell() == 0) {
        shell_active = 1;
        show_keyboard = 0;
        
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
        
        terminal_print("=== SHELL INTERACTIVE ===");
        terminal_print("-----------------------------------");
    } else {
        terminal_print("Error");
        shell_active = 0;
    }
}

void shell_stop_interactive() {
    interactive_thread_running = 0;
    shell_active = 0;
    
    if (interactive_thread >= 0) {
        sceKernelWaitThreadEnd(interactive_thread, NULL, NULL);
        sceKernelDeleteThread(interactive_thread);
        interactive_thread = -1;
    }
    
    stop_interactive_shell();
    terminal_print("Shell interactive closed");
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

void shell_process_input() {
    
    if (!show_keyboard) {
        return;
    }
    
    int result = kb(u"Shell SSH:", u"", keyboard_buffer, 0);
    
    if (result) {
        char command[MAX_INPUT_LENGTH];
        utf16_to_utf8(keyboard_buffer, command, sizeof(command));
        
        if (strlen(command) > 0) {
            interactive_shell_write(command, strlen(command));
            interactive_shell_write("\n", 1);
            char display[300];
            snprintf(display, sizeof(display), "%s", command);
            terminal_print(display);
        }
        
        memset(keyboard_buffer, 0, sizeof(keyboard_buffer));
    } else {
        terminal_print("Entry canceled");
    }
    
    show_keyboard = 0;
}

void shell_handle_controls(SceCtrlData *pad, unsigned int new_buttons) {
    if (!shell_active) return;
    
    if (new_buttons & SCE_CTRL_TRIANGLE && !show_keyboard) {
        show_keyboard = 1;
    }
    
    if (new_buttons & SCE_CTRL_START) {
        shell_stop_interactive();
        return;
    }

}

void shell_render(vita2d_pgf *font) {
    if (!shell_active) return;
    
    int max_visible_lines = 20;
    int start_y = 30;
    int line_height = 20;
    
    int total_lines = get_line_count();
    int start_index = 0;
    
    if (total_lines > max_visible_lines) {
        start_index = total_lines - max_visible_lines;
    }
    
    for (int i = 0; i < max_visible_lines && (start_index + i) < total_lines; i++) {
        vita2d_pgf_draw_text(font, 20, start_y + i * line_height,
                             RGBA8(0, 255, 0, 255), 1.0f, get_line(start_index + i));
    }
    
    int status_y = 480;
    char status[128];
    
    if (show_keyboard) {
        strcpy(status, "[Writing...]");
    } else {
        strcpy(status, "SHELL INTERACTIVE - △:Keyboard  START:Exit");
    }
    
    vita2d_pgf_draw_text(font, 20, status_y, RGBA8(255, 255, 0, 255), 0.7f, status);
    
    if (total_lines > max_visible_lines) {
        char scroll_indicator[32];
        snprintf(scroll_indicator, sizeof(scroll_indicator), "[%d+]", total_lines - max_visible_lines);
        vita2d_pgf_draw_text(font, 900, status_y, RGBA8(255, 255, 0, 255), 0.7f, scroll_indicator);
    }
}
