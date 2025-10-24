#ifndef INPUT_H
#define INPUT_H

#include <psp2/common_dialog.h>
#include <psp2/ime_dialog.h>

#define MAX_TEXT_LENGTH SCE_IME_DIALOG_MAX_TEXT_LENGTH

int kb(const uint16_t* title, uint16_t* initial_text, uint16_t* output_buffer, int password_mode);

void utf16_to_utf8(const uint16_t* src, char* dst, size_t dst_size);

#endif
