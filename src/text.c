#include "text.h"
#include "ansi_colors.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CHARS 256
#define MAX_LINES 30

typedef struct {
    char text[MAX_CHARS];
    unsigned int color;
} TerminalLine;

static TerminalLine terminal_buffer[MAX_LINES];
static int line_count = 0;
static unsigned int current_color = RGBA8(0, 255, 0, 255); // Green Default

void set_terminal_color(unsigned int color) {
    current_color = color;
}

void terminal_print(const char *text) {
    terminal_print_color(text, current_color);
}

void terminal_print_color(const char *text, unsigned int color) {
    if (line_count < MAX_LINES) {
        strncpy(terminal_buffer[line_count].text, text, MAX_CHARS - 1);
        terminal_buffer[line_count].text[MAX_CHARS - 1] = '\0';
        terminal_buffer[line_count].color = color;
        line_count++;
    } else {
        // Scroll buffer when full
        for (int i = 0; i < MAX_LINES - 1; i++) {
            strcpy(terminal_buffer[i].text, terminal_buffer[i + 1].text);
            terminal_buffer[i].color = terminal_buffer[i + 1].color;
        }
        strncpy(terminal_buffer[MAX_LINES - 1].text, text, MAX_CHARS - 1);
        terminal_buffer[MAX_LINES - 1].text[MAX_CHARS - 1] = '\0';
        terminal_buffer[MAX_LINES - 1].color = color;
    }
}

void terminal_print_ansi(const char *text) {
    char processed_text[MAX_CHARS];
    unsigned int new_color = current_color;
    
    // Process ANSI color codes
    process_ansi_colors(text, processed_text, &new_color);
    
    // Print with the new color
    terminal_print_color(processed_text, new_color);
    
    // Update current color for next prints
    current_color = new_color;
}

void terminal_clear() {
    line_count = 0;
    current_color = RGBA8(0, 255, 0, 255); // Reset to green
}

int get_line_count() {
    return line_count;
}

const char* get_line(int index) {
    if (index >= 0 && index < line_count) {
        return terminal_buffer[index].text;
    }
    return "";
}

unsigned int get_line_color(int index) {
    if (index >= 0 && index < line_count) {
        return terminal_buffer[index].color;
    }
    return RGBA8(0, 255, 0, 255); // Green Default
}
