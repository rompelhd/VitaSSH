#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/sysmodule.h>
#include <psp2/ctrl.h>
#include <psp2/apputil.h>
#include <psp2/message_dialog.h>
#include <psp2/ime_dialog.h>
#include <psp2/display.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "graphics.h"
#include "network.h"
#include "ssh_client.h"
#include "input.h"
#include "debugScreen.h"

#define printf psvDebugScreenPrintf

char command[MAX_TEXT_LENGTH + 1] = "";

int main(int argc, char *argv[]) {
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

    sceAppUtilInit(&(SceAppUtilInitParam){}, &(SceAppUtilBootParam){});
    sceCommonDialogSetConfigParam(&(SceCommonDialogConfigParam){});
    gxm_init();

    while (!input_complete) {
        clear_screen();

        if (!shown_dial) {
            show_current_input_dialog();
        }

        if (sceImeDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_FINISHED) {
            SceImeDialogResult result = {};
            sceImeDialogGetResult(&result);
            
            if (result.button == SCE_IME_DIALOG_BUTTON_ENTER) {
                switch (current_input_step) {
                    case 0:
                        wchar_to_char(ip_input, ip, sizeof(ip));
                        current_input_step++;
                        break;
                    case 1:
                        wchar_to_char(port_input, port, sizeof(port));
                        current_input_step++;
                        break;
                    case 2:
                        wchar_to_char(user_input, username, sizeof(username));
                        current_input_step++;
                        break;
                    case 3:
                        wchar_to_char(pass_input, password, sizeof(password));
                        input_complete = true;
                        break;
                }
            }
            
            sceImeDialogTerm();
            shown_dial = 0;
        }

        sceCommonDialogUpdate(&(SceCommonDialogUpdateParam){{
            NULL, dbuf[backBufferIndex].data, 0, 0,
            DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_STRIDE_IN_PIXELS},
            dbuf[backBufferIndex].sync});

        gxm_swap();
        sceDisplayWaitVblankStart();
    }

    gxm_term();
    psvDebugScreenInit();
    
    printf("=== SSH Client on PS Vita ===\n");
    printf("IP: %s\n", ip);
    printf("Port: %s\n", port);
    printf("User: %s\n", username);
    printf("Password: [hidden]\n\n");

    if (load_network_modules() < 0) {
        printf("Could not load network modules.\n");
    } else {
        printf("Starting network...\n");
        if (init_network() < 0) {
            printf("Could not initialize network.\n");
        } else {
            printf("Network started successfully.\n");

            if (ssh_connect() == 0) {
                printf("\n=== SSH Connection Established ===\n");
                printf("Controls:\n");
                printf("  TRIANGLE - Open keyboard for command\n");
                printf("  SQUARE   - Exit\n\n");
                
                SceCtrlData pad;
                bool running = true;
                bool cmd_dialog_active = false;
                
                while (running) {
                    sceCtrlPeekBufferPositive(0, &pad, 1);
                    
                    if ((pad.buttons & SCE_CTRL_TRIANGLE) && !cmd_dialog_active) {
                        printf("Opening keyboard...\n");
                        gxm_init();
                        show_command_dialog();
                        cmd_dialog_active = true;
                    }
                    
                    if (pad.buttons & SCE_CTRL_SQUARE) {
                        printf("\nExiting...\n");
                        running = false;
                    }

                    if (cmd_dialog_active) {
                        if (sceImeDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_FINISHED) {
                            SceImeDialogResult result = {};
                            sceImeDialogGetResult(&result);
                            
                            if (result.button == SCE_IME_DIALOG_BUTTON_ENTER) {
                                wchar_to_char(cmd_input, command, sizeof(command));
                                if (strlen(command) > 0) {
                                    gxm_term();
                                    psvDebugScreenInit();
                                    printf("\n>>> Executing: %s\n", command);
                                    execute_ssh_command(command);
                                    printf("\nPress TRIANGLE for another command or SQUARE to exit\n");
                                }
                            } else {
                                gxm_term();
                                psvDebugScreenInit();
                                printf("\nCommand cancelled\n");
                                printf("\nPress TRIANGLE for another command or SQUARE to exit\n");
                            }
                            
                            sceImeDialogTerm();
                            cmd_dialog_active = false;
                            shown_dial = 0;
                            memset(cmd_input, 0, sizeof(cmd_input));
                        } else {
                            sceCommonDialogUpdate(&(SceCommonDialogUpdateParam){{
                                NULL, dbuf[backBufferIndex].data, 0, 0,
                                DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_STRIDE_IN_PIXELS},
                                dbuf[backBufferIndex].sync});
                            gxm_swap();
                            sceDisplayWaitVblankStart();
                        }
                    }

                    sceKernelDelayThread(50000);
                }
            } else {
                printf("Error in SSH connection\n");
            }
        }
    }

    // Cleanup
    cleanup_ssh();
    cleanup_network();
    
    printf("Program terminated.\n");
    sceKernelDelayThread(3 * 1000 * 1000);
    sceKernelExitProcess(0);
    
    return 0;
}
