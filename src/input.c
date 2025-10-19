#include "input.h"
#include <string.h>
#include <stdbool.h>

int current_input_step = 0;
bool input_complete = false;
bool show_cmd_dialog = false;
int shown_dial = 0;

uint16_t ip_input[MAX_TEXT_LENGTH + 1] = {0};
uint16_t port_input[MAX_TEXT_LENGTH + 1] = {0};
uint16_t user_input[MAX_TEXT_LENGTH + 1] = {0};
uint16_t pass_input[MAX_TEXT_LENGTH + 1] = {0};
uint16_t cmd_input[MAX_TEXT_LENGTH + 1] = {0};

void wchar_to_char(const uint16_t *wstr, char *str, size_t max_len) {
    size_t i;
    for (i = 0; i < max_len - 1 && wstr[i] != 0; i++) {
        str[i] = (char)wstr[i];
    }
    str[i] = '\0';
}

void show_current_input_dialog() {
    SceImeDialogParam param;
    sceImeDialogParamInit(&param);

    param.supportedLanguages = SCE_IME_LANGUAGE_ENGLISH;
    param.languagesForced = SCE_TRUE;
    param.type = SCE_IME_TYPE_BASIC_LATIN;
    param.option = 0;
    
    switch (current_input_step) {
        case 0:
            param.title = u"Server IP Address:";
            param.initialText = u"192.168.1.1";
            param.inputTextBuffer = ip_input;
            break;
        case 1:
            param.title = u"SSH Port:";
            param.initialText = u"22";
            param.inputTextBuffer = port_input;
            break;
        case 2:
            param.title = u"Username:";
            param.initialText = u"root";
            param.inputTextBuffer = user_input;
            break;
        case 3:
            param.title = u"Password:";
            param.initialText = u"";
            param.inputTextBuffer = pass_input;
            param.textBoxMode = SCE_IME_DIALOG_TEXTBOX_MODE_PASSWORD;
            break;
    }
    
    param.maxTextLength = MAX_TEXT_LENGTH;
    shown_dial = sceImeDialogInit(&param) > 0;
}

void show_command_dialog() {
    SceImeDialogParam param;
    sceImeDialogParamInit(&param);

    param.supportedLanguages = SCE_IME_LANGUAGE_ENGLISH;
    param.languagesForced = SCE_TRUE;
    param.type = SCE_IME_TYPE_BASIC_LATIN;
    param.option = 0;
    param.title = u"Command to execute:";
    param.initialText = u"ls -la";
    param.inputTextBuffer = cmd_input;
    param.maxTextLength = MAX_TEXT_LENGTH;
    
    shown_dial = sceImeDialogInit(&param) > 0;
    show_cmd_dialog = true;
}
