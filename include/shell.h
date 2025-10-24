#ifndef SHELL_H
#define SHELL_H

#include <psp2/ctrl.h>
#include <vita2d.h>

void shell_start_interactive();
void shell_process_input();
void shell_render(vita2d_pgf *font);
void shell_handle_controls(SceCtrlData *pad, unsigned int new_buttons);
int shell_is_active();
void shell_stop_interactive();
void shell_set_prompt(const char *username, const char *hostname);

#endif
