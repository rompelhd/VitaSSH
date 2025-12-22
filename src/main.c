#include <stdio.h>
#include <string.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/common_dialog.h>
#include <vita2d.h>
#include <stdlib.h>

#include "display.h"
#include "network.h"
#include "ssh_client.h"
#include "text.h"
#include "input.h"
#include "shell.h"

#define MAX_TEXT_LENGTH SCE_IME_DIALOG_MAX_TEXT_LENGTH

int credentials_entered = 0;
int ssh_connected = 0;

void get_ssh_credentials() {
    uint16_t ip_input[MAX_TEXT_LENGTH + 1] = {0};
    uint16_t port_input[MAX_TEXT_LENGTH + 1] = {0};
    uint16_t user_input[MAX_TEXT_LENGTH + 1] = {0};
    uint16_t pass_input[MAX_TEXT_LENGTH + 1] = {0};
    
    char temp_ip[64], temp_port[16], temp_user[64], temp_pass[64];
    strncpy(temp_ip, ip, sizeof(temp_ip));
    strncpy(temp_port, port, sizeof(temp_port));
    strncpy(temp_user, username, sizeof(temp_user));
    strncpy(temp_pass, password, sizeof(temp_pass));
    
    mbstowcs((wchar_t*)ip_input, temp_ip, MAX_TEXT_LENGTH);
    mbstowcs((wchar_t*)port_input, temp_port, MAX_TEXT_LENGTH);
    mbstowcs((wchar_t*)user_input, temp_user, MAX_TEXT_LENGTH);
    mbstowcs((wchar_t*)pass_input, temp_pass, MAX_TEXT_LENGTH);
    
    int current_input_step = 0;
    int all_inputs_completed = 0;
    
    while (!all_inputs_completed) {
        int input_result = 0;
        
        switch (current_input_step) {
            case 0:
                input_result = kb(u"Server IP Address:", ip_input, ip_input, 0);
                break;
            case 1:
                input_result = kb(u"SSH Port:", port_input, port_input, 0);
                break;
            case 2:
                input_result = kb(u"Username:", user_input, user_input, 0);
                break;
            case 3:
                input_result = kb(u"Password:", pass_input, pass_input, 1);
                break;
        }
        
        if (input_result) {
            current_input_step++;
            if (current_input_step >= 4) {
                all_inputs_completed = 1;
                credentials_entered = 1;
                
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
        
        shell_set_prompt(username, ip);
        
        // Automatically start interactive shell
        if (start_interactive_shell() == 0) {
            shell_start_interactive();
        }
    } else {
        terminal_print("Error in SSH connection");
        cleanup_network();
    }
}

void disconnect_ssh() {
    if (ssh_connected) {
        if (shell_is_active()) {
            shell_stop_interactive();
        }
        cleanup_ssh();
        cleanup_network();
        ssh_connected = 0;
        terminal_print("SSH disconnected");
    } else {
        terminal_print("No active SSH connection");
    }
}

int main() {
    vita2d_init();
    vita2d_set_clear_color(RGBA8(0, 0, 0, 255));

    SceCommonDialogConfigParam configParam;
    memset(&configParam, 0, sizeof(configParam));
    sceCommonDialogSetConfigParam(&configParam);

    vita2d_pgf *font = vita2d_load_default_pgf();
    if (!font) {
        vita2d_fini();
        return 1;
    }

    set_terminal_font(font);
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

    SceCtrlData pad;
    unsigned int old_buttons = 0;

    terminal_print("=========================================");
    terminal_print("    VitaSSH - Interactive Shell Only");
    terminal_print("=========================================");
    terminal_print("");
    terminal_print("Press TRIANGLE to configure SSH credentials");
    terminal_print("Press START to exit");

    while (1) {
        sceCtrlPeekBufferPositive(0, &pad, 1);
        unsigned int new_buttons = pad.buttons & ~old_buttons;

        if (shell_is_active()) {
            // In interactive shell mode
            shell_handle_controls(&pad, new_buttons);
            shell_process_input();
            
            vita2d_start_drawing();
            vita2d_clear_screen();
            shell_render(font);
            vita2d_end_drawing();
            vita2d_swap_buffers();
            
            // Check if shell was closed
            if (!shell_is_active()) {
                disconnect_ssh();
                
                terminal_clear();
                terminal_print("=========================================");
                terminal_print("    VitaSSH - Interactive Shell Only");
                terminal_print("=========================================");
                terminal_print("");
                terminal_print("Press TRIANGLE to configure SSH credentials");
                terminal_print("Press START to exit");
            }
        } else {
            // Not in shell mode - handle configuration
            if (new_buttons & SCE_CTRL_TRIANGLE) {
                get_ssh_credentials();
                if (credentials_entered && !ssh_connected) {
                    connect_ssh();
                }
            }

            if (new_buttons & SCE_CTRL_START) {
                break;
            }

            // Render main screen
            vita2d_start_drawing();
            vita2d_clear_screen();
            
            vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(60, 60, 60, 255));
            
            int term_x = TERM_X;
            int term_y = TERM_Y;
            int term_width = TERM_WIDTH;
            int term_height = TERM_HEIGHT;
            
            vita2d_draw_rectangle(term_x, term_y, term_width, term_height, RGBA8(0, 0, 0, 255));
            vita2d_draw_rectangle(term_x + 1, term_y + 1, term_width - 2, term_height - 2, RGBA8(30, 30, 30, 255));
            
            int max_visible_lines = 25;
            int start_y = TEXT_START_Y;
            int total_lines = get_line_count();
            int scroll_offset = get_scroll_offset();
            
            int line_height = CHAR_HEIGHT;
            
            for (int i = 0; i < max_visible_lines; i++) {
                int line_index = scroll_offset + i;
                if (line_index < total_lines) {
                    const char* line = get_line(line_index);
                    if (line != NULL && strlen(line) > 0) {
                        unsigned int color = get_line_color(line_index);
                        vita2d_pgf_draw_text(font, TEXT_START_X, start_y + i * line_height,
                                            color, 0.7f, line);
                    }
                }
            }
            
            int controls_y = term_y + term_height + 15;
            if (controls_y > 500) controls_y = 500;
            
            vita2d_draw_rectangle(0, controls_y - 10, 960, 544 - controls_y + 10, RGBA8(40, 40, 40, 255));
            
            const char *controls = "△: Configure SSH and Connect  START: Exit";
            
            vita2d_pgf_draw_text(font, 20, controls_y,
                                RGBA8(255, 255, 255, 255), 0.7f, controls);

            vita2d_end_drawing();
            vita2d_swap_buffers();
        }

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

    if (ssh_connected) {
        disconnect_ssh();
    }
    if (shell_is_active()) {
        shell_stop_interactive();
    }
    
    vita2d_free_pgf(font);
    vita2d_fini();
    sceKernelExitProcess(0);
    return 0;
}
