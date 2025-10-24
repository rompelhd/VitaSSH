#ifndef TEXT_H
#define TEXT_H

#include <vita2d.h>

void terminal_print(const char *text);
void terminal_print_color(const char *text, unsigned int color);
void terminal_print_ansi(const char *text);
void terminal_clear();
int get_line_count();
const char* get_line(int index);
unsigned int get_line_color(int index);
void set_terminal_color(unsigned int color);

#endif
