#ifndef ANSI_COLORS_H
#define ANSI_COLORS_H

#include <vita2d.h>

unsigned int ansi_to_vita_color(int ansi_code);

void process_ansi_colors(const char *input, char *output, unsigned int *current_color);

#endif
