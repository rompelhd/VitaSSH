#ifndef INPUT_H
#define INPUT_H

#include <psp2/ime_dialog.h>
#include <stdbool.h>

#define MAX_TEXT_LENGTH SCE_IME_DIALOG_MAX_TEXT_LENGTH

// Input dialogs
void show_current_input_dialog();
void show_command_dialog();

// Utility functions
void wchar_to_char(const uint16_t *wstr, char *str, size_t max_len);

// Global input state
extern int current_input_step;
extern bool input_complete;
extern bool show_cmd_dialog;
extern int shown_dial;

// Input buffers
extern uint16_t ip_input[MAX_TEXT_LENGTH + 1];
extern uint16_t port_input[MAX_TEXT_LENGTH + 1];
extern uint16_t user_input[MAX_TEXT_LENGTH + 1];
extern uint16_t pass_input[MAX_TEXT_LENGTH + 1];
extern uint16_t cmd_input[MAX_TEXT_LENGTH + 1];

#endif
