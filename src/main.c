#include <stdio.h>
#include <string.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/common_dialog.h>
#include <vita2d.h>
#include "network.h"
#include "ssh_client.h"
#include "text.h"
#include "input.h"
#include "shell.h"

#define MAX_TEXT_LENGTH SCE_IME_DIALOG_MAX_TEXT_LENGTH

static uint16_t ip_input[MAX_TEXT_LENGTH + 1] = {0};
static uint16_t port_input[MAX_TEXT_LENGTH + 1] = {0};
static uint16_t user_input[MAX_TEXT_LENGTH + 1] = {0};
static uint16_t pass_input[MAX_TEXT_LENGTH + 1] = {0};

int credentials_entered = 0;
int ssh_connected = 0;

void get_ssh_credentials() {
    int current_input_step = 0;
    int all_inputs_completed = 0;
    
    while (!all_inputs_completed) {
        int input_result = 0;
        
        switch (current_input_step) {
            case 0:
                input_result = kb(u"Server IP Address:", u"192.168.1.1", ip_input, 0);
                break;
            case 1:
                input_result = kb(u"SSH Port:", u"22", port_input, 0);
                break;
            case 2:
                input_result = kb(u"Username:", u"root", user_input, 0);
                break;
            case 3:
                input_result = kb(u"Password:", u"", pass_input, 1);
                break;
        }
        
        if (input_result) {
            current_input_step++;
            if (current_input_step >= 4) {
                all_inputs_completed = 1;
                credentials_entered = 1;
                
                // Convert data to UTF-8 and copy to global variables
                utf16_to_utf8(ip_input, ip, sizeof(ip));
                utf16_to_utf8(port_input, port, sizeof(port));
                utf16_to_utf8(user_input, username, sizeof(username));
                utf16_to_utf8(pass_input, password, sizeof(password));
                
                terminal_print("Credentials saved!");
            }
        } else {
            if (current_input_step > 0) {
                current_input_step--;
                terminal_print("Field cancelled, returning to previous...");
            } else {
                terminal_print("Configuration cancelled");
                break;
            }
        }
    }
}

void display_credentials() {
    if (credentials_entered) {
        terminal_print("--- SSH Credentials ---");
        
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "IP: %s", ip);
        terminal_print(buffer);
        
        snprintf(buffer, sizeof(buffer), "Port: %s", port);
        terminal_print(buffer);
        
        snprintf(buffer, sizeof(buffer), "Username: %s", username);
        terminal_print(buffer);
        
        terminal_print("Password: ********");
        terminal_print("-------------------");
    } else {
        terminal_print("No credentials configured");
        terminal_print("Use △ to configure them");
    }
}

void connect_ssh() {
    if (!credentials_entered) {
        terminal_print("Error: First configure SSH credentials");
        return;
    }
    
    terminal_print("Starting SSH connection...");
    
    if (load_network_modules() < 0) {
        terminal_print("Error: Could not load network modules");
        return;
    }
    
    terminal_print("Network modules loaded");
    
    if (init_network() < 0) {
        terminal_print("Error: Could not initialize network");
        return;
    }
    
    terminal_print("Network initialized successfully");
    terminal_print("Connecting to SSH...");
    
    if (ssh_connect() == 0) {
        ssh_connected = 1;
        terminal_print("=== SSH Connection Established ===");
        
        // Configure prompt with username and hostname
        shell_set_prompt(username, ip);
        
    } else {
        terminal_print("Error in SSH connection");
        cleanup_network();
    }
}

void disconnect_ssh() {
    if (ssh_connected) {
        cleanup_ssh();
        cleanup_network();
        ssh_connected = 0;
        terminal_print("SSH disconnected");
    } else {
        terminal_print("No active SSH connection");
    }
}

void render_screen(vita2d_pgf *font) {
    vita2d_start_drawing();
    vita2d_clear_screen();
    
    int max_visible_lines = 22;
    int start_y = 28;
    int line_height = 15;
    
    int total_lines = get_line_count();
    int start_index = 0;
    
    if (total_lines > max_visible_lines) {
        start_index = total_lines - max_visible_lines;
    }
    
    for (int i = 0; i < max_visible_lines && (start_index + i) < total_lines; i++) {
        vita2d_pgf_draw_text(font, 20, start_y + i * line_height,
                             RGBA8(0, 255, 0, 255), 0.9f, get_line(start_index + i));
    }
    
    // Scroll indicator
    if (total_lines > max_visible_lines) {
        char scroll_indicator[32];
        snprintf(scroll_indicator, sizeof(scroll_indicator), "[%d+ lines]", total_lines - max_visible_lines);
        vita2d_pgf_draw_text(font, 20, 480, RGBA8(255, 255, 0, 255), 0.7f, scroll_indicator);
    }

    int controls_y = 500;
    
    vita2d_draw_rectangle(0, controls_y - 5, 960, 544 - controls_y + 5, RGBA8(0, 0, 0, 220));
    
    char controls[256];
    if (ssh_connected) {
        strcpy(controls, "△: Shell  ○: Command  □: Disconnect  START: Exit");
    } else {
        strcpy(controls, "△: Config SSH  ○: Credentials  X: Connect  START: Exit");
    }
    
    vita2d_pgf_draw_text(font, 20, controls_y,
                         RGBA8(255, 255, 255, 255), 0.8f, controls);

    vita2d_end_drawing();
    vita2d_swap_buffers();
}

int main() {
    vita2d_init();
    vita2d_set_clear_color(RGBA8(0, 0, 0, 255));

    SceCommonDialogConfigParam configParam;
    memset(&configParam, 0, sizeof(configParam));
    sceCommonDialogSetConfigParam(&configParam);

    vita2d_pgf *font = vita2d_load_default_pgf();
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

    SceCtrlData pad;
    unsigned int old_buttons = 0;

    terminal_print("SSH Terminal - Vita By The goat Rompelhd");
    terminal_print("-----------------------------------------------------------");

    while (1) {
        sceCtrlPeekBufferPositive(0, &pad, 1);
        unsigned int new_buttons = pad.buttons & ~old_buttons;

        if (new_buttons & SCE_CTRL_START) {
            if (shell_is_active()) {
                shell_stop_interactive();
                terminal_print("Command mode closed");
            } else {
                break; // Exit application
            }
        }

        // If we're in shell or single command mode
        if (shell_is_active()) {
            shell_handle_controls(&pad, new_buttons);
            shell_process_input();
            
            // Render the shell
            vita2d_start_drawing();
            vita2d_clear_screen();
            shell_render(font);
            vita2d_end_drawing();
            vita2d_swap_buffers();
        } else {

            if (new_buttons & SCE_CTRL_TRIANGLE) {
                if (ssh_connected) {
                    shell_start_interactive();
                    terminal_print("Starting interactive shell...");
                } else {
                    get_ssh_credentials();
                }
            }


            if (new_buttons & SCE_CTRL_CIRCLE) {
                if (ssh_connected) {
                    static uint16_t cmd_input[MAX_TEXT_LENGTH + 1] = {0};
                    int result = kb(u"SSH Command:", u"ls", cmd_input, 0);
                    if (result) {
                        char command[256];
                        utf16_to_utf8(cmd_input, command, sizeof(command));
                        if (strlen(command) > 0) {
                            char buffer[300];
                            snprintf(buffer, sizeof(buffer), ">>> %s", command);
                            terminal_print(buffer);
                            execute_ssh_command(command);
                        }
                    }
                } else {
                    display_credentials();
                }
            }

            if (new_buttons & SCE_CTRL_CROSS) {
                if (!ssh_connected) {
                    connect_ssh();
                }
            }

            if (new_buttons & SCE_CTRL_SQUARE) {
                if (ssh_connected) {
                    disconnect_ssh();
                }
            }

            // Render normal screen
            render_screen(font);
        }

        // Update commondialogs
        SceCommonDialogUpdateParam updateParam;
        memset(&updateParam, 0, sizeof(updateParam));
        updateParam.renderTarget.colorSurfaceData = vita2d_get_current_fb();
        updateParam.renderTarget.surfaceType = SCE_GXM_COLOR_SURFACE_LINEAR;
        updateParam.renderTarget.colorFormat = SCE_GXM_COLOR_FORMAT_A8B8G8R8;
        updateParam.renderTarget.width = 960;
        updateParam.renderTarget.height = 544;
        updateParam.renderTarget.strideInPixels = 960;
        sceCommonDialogUpdate(&updateParam);

        old_buttons = pad.buttons;
        sceKernelDelayThread(10000);
    }

    // Cleanup
    if (ssh_connected) {
        disconnect_ssh();
    }
    if (shell_is_active()) {
        shell_stop_interactive();
    }

    vita2d_fini();
    vita2d_free_pgf(font);
    sceKernelExitProcess(0);
    return 0;
}
