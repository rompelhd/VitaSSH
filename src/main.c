#include <stdio.h>
#include <string.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/common_dialog.h>
#include <vita2d.h>
#include <stdlib.h>
#include <psp2/touch.h>

#include "display.h"
#include "network.h"
#include "ssh_client.h"
#include "text.h"
#include "input.h"
#include "shell.h"
#include "menu.h"

#define MAX_TEXT_LENGTH SCE_IME_DIALOG_MAX_TEXT_LENGTH

static AppState app_state = APP_STATE_LOADING;
static int menu_selection = 0;
static int profile_selection = 0;
static SshProfile profiles[MAX_PROFILES];
static int profile_count = 0;
static SshProfile selected_profile;

int credentials_entered = 0;
int ssh_connected = 0;

static int touch_start_y = 0;
static int touch_dragging = 0;
static int touch_scroll_sensitivity = 3;

static uint64_t loading_start_time = 0;

static int delete_confirm = 0;
static uint64_t delete_timer = 0;

void get_ssh_credentials_from_profile(SshProfile *profile) {
    uint16_t ip_input[MAX_TEXT_LENGTH + 1] = {0};
    uint16_t port_input[MAX_TEXT_LENGTH + 1] = {0};
    uint16_t user_input[MAX_TEXT_LENGTH + 1] = {0};
    uint16_t pass_input[MAX_TEXT_LENGTH + 1] = {0};
    

    char temp_ip[64], temp_port[16], temp_user[64], temp_pass[64];
    strncpy(temp_ip, profile->ip, sizeof(temp_ip));
    strncpy(temp_port, profile->port, sizeof(temp_port));
    strncpy(temp_user, profile->username, sizeof(temp_user));
    strncpy(temp_pass, profile->password, sizeof(temp_pass));
    
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
                
                utf16_to_utf8(ip_input, profile->ip, sizeof(profile->ip));
                utf16_to_utf8(port_input, profile->port, sizeof(profile->port));
                utf16_to_utf8(user_input, profile->username, sizeof(profile->username));
                utf16_to_utf8(pass_input, profile->password, sizeof(profile->password));
                
                strcpy(ip, profile->ip);
                strcpy(port, profile->port);
                strcpy(username, profile->username);
                strcpy(password, profile->password);
                
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
    
    vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(60, 60, 60, 255));
    
    int max_visible_lines = 25;
    int start_y = TEXT_START_Y;
    int total_lines = get_line_count();
    int scroll_offset = get_scroll_offset();
    
    vita2d_draw_rectangle(TERM_X, TERM_Y, TERM_WIDTH, TERM_HEIGHT, RGBA8(0, 0, 0, 255));
    vita2d_draw_rectangle(TERM_X + 1, TERM_Y + 1, TERM_WIDTH - 2, TERM_HEIGHT - 2, RGBA8(30, 30, 30, 255));
    
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
    
    if (total_lines > max_visible_lines) {
        int scrollbar_height = (max_visible_lines * TERM_HEIGHT) / total_lines;
        if (scrollbar_height < 20) scrollbar_height = 20;
        int scrollbar_y = TERM_Y + (scroll_offset * TERM_HEIGHT) / total_lines;
        vita2d_draw_rectangle(TERM_X + TERM_WIDTH - 10, scrollbar_y, 5, scrollbar_height, RGBA8(150, 150, 150, 255));

        char scroll_info[64];
        snprintf(scroll_info, sizeof(scroll_info), "Lines: %d/%d", 
                scroll_offset + max_visible_lines, total_lines);
        vita2d_pgf_draw_text(font, 800, 480, RGBA8(255, 255, 0, 255), 0.6f, scroll_info);
    }

    int controls_y = TERM_Y + TERM_HEIGHT + 15;
    if (controls_y > 500) controls_y = 500;
    
    vita2d_draw_rectangle(0, controls_y - 10, 960, 544 - controls_y + 10, RGBA8(40, 40, 40, 255));
    
    char controls[256];
    if (ssh_connected) {
        strcpy(controls, "△: Shell  ○: Command  □: Disconnect  START: Back to Menu");
    } else {
        strcpy(controls, "△: Config SSH  ○: Credentials  X: Connect  START: Back to Menu");
    }
    
    vita2d_pgf_draw_text(font, 20, controls_y,
                        RGBA8(255, 255, 255, 255), 0.7f, controls);
    
    vita2d_pgf_draw_text(font, 20, controls_y + 25,
                        RGBA8(200, 200, 200, 255), 0.5f, "Touch: Scroll  Back touch: Scroll to bottom");

    vita2d_end_drawing();
    vita2d_swap_buffers();
}

int main() {
    vita2d_init();
    vita2d_set_clear_color(RGBA8(0, 0, 0, 255));

    SceCommonDialogConfigParam configParam;
    memset(&configParam, 0, sizeof(configParam));
    sceCommonDialogSetConfigParam(&configParam);

    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);
    sceTouchEnableTouchForce(SCE_TOUCH_PORT_FRONT);
    sceTouchEnableTouchForce(SCE_TOUCH_PORT_BACK);

    vita2d_pgf *font = vita2d_load_default_pgf();
    if (!font) {
        vita2d_fini();
        return 1;
    }

    set_terminal_font(font);
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

    SceCtrlData pad;
    unsigned int old_buttons = 0;

    load_profiles(profiles, &profile_count);

    loading_start_time = sceKernelGetProcessTimeWide();

    while (1) {
        sceCtrlPeekBufferPositive(0, &pad, 1);
        unsigned int new_buttons = pad.buttons & ~old_buttons;
        
        switch (app_state) {
            case APP_STATE_LOADING:
                show_loading_screen(font);
                
                {
                    uint64_t current_time = sceKernelGetProcessTimeWide();
                    if (current_time - loading_start_time > 2000000) {
                        app_state = APP_STATE_MENU;
                    }
                }
                break;
                
            case APP_STATE_MENU:
                show_main_menu(font, &app_state, &menu_selection);
                
                if (new_buttons & SCE_CTRL_UP) {
                    menu_selection = (menu_selection - 1 + 4) % 4;
                }
                if (new_buttons & SCE_CTRL_DOWN) {
                    menu_selection = (menu_selection + 1) % 4;
                }
                if (new_buttons & SCE_CTRL_CROSS) {
                    switch (menu_selection) {
                        case 0: // SIMPLE SSH
                            app_state = APP_STATE_SIMPLE_SSH;
                            terminal_print("=========================================");
                            terminal_print("    VitaSSH Terminal");
                            terminal_print("=========================================");
                            break;
                        case 1: // PROFILES
                            app_state = APP_STATE_PROFILES;
                            profile_selection = 0;
                            delete_confirm = 0;
                            break;
                        case 2: // ABOUT
                            app_state = APP_STATE_ABOUT;
                            break;
                        case 3: // EXIT
                            goto exit_app;
                    }
                }
                if (new_buttons & SCE_CTRL_START) {
                    goto exit_app;
                }
                break;
                
            case APP_STATE_PROFILES:
                show_profiles_menu(font, &app_state, profiles, &profile_count, &profile_selection);
                
                if (new_buttons & SCE_CTRL_UP) {
                    if (profile_count > 0) {
                        profile_selection = (profile_selection - 1 + profile_count) % profile_count;
                        delete_confirm = 0;
                    }
                }
                if (new_buttons & SCE_CTRL_DOWN) {
                    if (profile_count > 0) {
                        profile_selection = (profile_selection + 1) % profile_count;
                        delete_confirm = 0;
                    }
                }
                if (new_buttons & SCE_CTRL_TRIANGLE) {
                    SshProfile new_profile;
                    memset(&new_profile, 0, sizeof(SshProfile));
                    get_ssh_credentials_from_profile(&new_profile);
                    
                    if (credentials_entered) {
                        char profile_name[32];
                        snprintf(profile_name, sizeof(profile_name), "Profile %d", profile_count + 1);
                        add_profile(profiles, &profile_count, profile_name, 
                                   new_profile.ip, new_profile.port, 
                                   new_profile.username, new_profile.password);
                        delete_confirm = 0;
                    }
                }
                if (new_buttons & SCE_CTRL_CIRCLE) {
                    app_state = APP_STATE_MENU;
                    delete_confirm = 0; 
                }
                if (new_buttons & SCE_CTRL_CROSS && profile_count > 0) {
                    selected_profile = profiles[profile_selection];
                    
                    strcpy(ip, selected_profile.ip);
                    strcpy(port, selected_profile.port);
                    strcpy(username, selected_profile.username);
                    strcpy(password, selected_profile.password);
                    credentials_entered = 1;
                    
                    app_state = APP_STATE_SIMPLE_SSH;
                    
                    terminal_clear();
                    terminal_print("=========================================");
                    terminal_print("    VitaSSH Terminal");
                    terminal_print("=========================================");
                    
                    char profile_msg[128];
                    snprintf(profile_msg, sizeof(profile_msg), "Using profile: %s", selected_profile.name);
                    terminal_print(profile_msg);
                    
                    if (!ssh_connected) {
                        connect_ssh();
                    }
                }
                if (new_buttons & SCE_CTRL_SQUARE && profile_count > 0) {
                    get_ssh_credentials_from_profile(&profiles[profile_selection]);
                    save_profiles(profiles, profile_count);
                    delete_confirm = 0;
                }
                if (new_buttons & SCE_CTRL_START && profile_count > 0) {
                    uint64_t current_time = sceKernelGetProcessTimeWide();
                    
                    if (!delete_confirm) {
                        delete_confirm = 1;
                        delete_timer = current_time;
                        
                        terminal_clear();
                        char confirm_msg[64];
                        snprintf(confirm_msg, sizeof(confirm_msg), 
                                "Delete profile '%s'?", profiles[profile_selection].name);
                        terminal_print(confirm_msg);
                        terminal_print("Press START again to confirm");
                        terminal_print("Or press any other button to cancel");
                        
                        app_state = APP_STATE_SIMPLE_SSH;
                        
                        for (int i = 0; i < 50; i++) {
                            sceKernelDelayThread(10000);
                            sceCtrlPeekBufferPositive(0, &pad, 1);
                            if (pad.buttons) break;
                        }
                        
                        app_state = APP_STATE_PROFILES;
                    } 
                    else if (current_time - delete_timer < 3000000) { // 3 s conf
                        char profile_name[32];
                        strncpy(profile_name, profiles[profile_selection].name, sizeof(profile_name));
                        
                        delete_profile(profiles, &profile_count, profile_selection);
                        
                        if (profile_selection >= profile_count && profile_count > 0) {
                            profile_selection = profile_count - 1;
                        }
                        
                        terminal_clear();
                        char deleted_msg[64];
                        snprintf(deleted_msg, sizeof(deleted_msg), 
                                "Profile '%s' deleted", profile_name);
                        terminal_print(deleted_msg);
                        
                        app_state = APP_STATE_SIMPLE_SSH;
                        sceKernelDelayThread(1000000);
                        app_state = APP_STATE_PROFILES;
                        
                        delete_confirm = 0;
                    } 
                    else {
                        delete_confirm = 0;
                    }
                }
                
                if (delete_confirm && 
                    !(new_buttons & SCE_CTRL_START) && 
                    (new_buttons != 0)) {
                    delete_confirm = 0;
                }
                break;
                
            case APP_STATE_ABOUT:
                show_about_screen(font, &app_state);
                
                if (new_buttons & SCE_CTRL_CIRCLE) {
                    app_state = APP_STATE_MENU;
                }
                break;
                
            case APP_STATE_SIMPLE_SSH: {
                SceTouchData touch;
                sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
                
                if (touch.reportNum > 0) {
                    int touch_y = touch.report[0].y;
                    
                    if (!touch_dragging) {
                        touch_start_y = touch_y;
                        touch_dragging = 1;
                    } else {
                        int delta_y = touch_start_y - touch_y;
                        if (abs(delta_y) > 10) {
                            adjust_scroll(delta_y / touch_scroll_sensitivity);
                            touch_start_y = touch_y;
                        }
                    }
                } else {
                    touch_dragging = 0;
                }

                SceTouchData back_touch;
                sceTouchPeek(SCE_TOUCH_PORT_BACK, &back_touch, 1);
                if (back_touch.reportNum > 0) {
                    scroll_to_bottom();
                }

                if (new_buttons & SCE_CTRL_START) {
                    if (shell_is_active()) {
                        shell_stop_interactive();
                        terminal_print("Command mode closed");
                    } else {
                        app_state = APP_STATE_MENU;
                        if (ssh_connected) {
                            disconnect_ssh();
                        }
                    }
                }

                if (shell_is_active()) {
                    shell_handle_controls(&pad, new_buttons);
                    shell_process_input();
                    
                    vita2d_start_drawing();
                    vita2d_clear_screen();
                    shell_render(font);
                    vita2d_end_drawing();
                    vita2d_swap_buffers();
                } else {
                    if (new_buttons & SCE_CTRL_TRIANGLE) {
                        if (ssh_connected) {
                            shell_start_interactive();
                        } else {
                            // Usar perfil seleccionado
                            if (profile_count > 0) {
                                selected_profile = profiles[profile_selection];
                                strcpy(ip, selected_profile.ip);
                                strcpy(port, selected_profile.port);
                                strcpy(username, selected_profile.username);
                                strcpy(password, selected_profile.password);
                                credentials_entered = 1;
                                terminal_print("Using profile credentials");
                            } else {
                                get_ssh_credentials_from_profile(&selected_profile);
                            }
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

                    render_screen(font);
                }
                break;
            }
                
            case APP_STATE_TERMINAL:
                // No Now
                break;
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

exit_app:
    if (ssh_connected) {
        disconnect_ssh();
    }
    if (shell_is_active()) {
        shell_stop_interactive();
    }

    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_STOP);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_STOP);
    
    vita2d_free_pgf(font);
    vita2d_fini();
    sceKernelExitProcess(0);
    return 0;
}
