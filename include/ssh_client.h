#ifndef TEXT_H
#define TEXT_H

#include <vita2d.h>
#include "display.h"

#define MAX_SCROLLBACK 1000
#define MAX_LINES 1000
#define MAX_LINE_LENGTH 512

typedef struct {
    char character;
    unsigned int fg_color;
    unsigned int bg_color;
    int bold;
    int underline;
} terminal_cell_t;

typedef struct {
    terminal_cell_t screen[TERMINAL_HEIGHT][TERMINAL_WIDTH];
    int cursor_x;
    int cursor_y;
    int saved_cursor_x;
    int saved_cursor_y;
    unsigned int current_fg;
    unsigned int current_bg;
    int bold;
    int underline;
    int reverse_video;
    
    char scrollback[MAX_SCROLLBACK][TERMINAL_WIDTH];
    int scrollback_count;
    int scrollback_pos;
    
    char ansi_buffer[32];
    int ansi_buffer_pos;
    int ansi_params[16];
    int ansi_param_count;
} terminal_t;

extern char lines[MAX_LINES][MAX_LINE_LENGTH];
extern unsigned int line_colors[MAX_LINES];
extern int line_count;
extern unsigned int current_color;

extern int scroll_offset;
extern int auto_scroll;

extern unsigned int ansi_colors[];

void terminal_init();
void terminal_clear_screen();
void terminal_clear_line(int line);
void terminal_put_char(char c);
void terminal_scroll_up();
void terminal_process_data(const char *data, int length);
void terminal_render(vita2d_pgf *font);
void process_ansi_command();
void process_ansi_colors();

void set_interactive_mode(int enabled);
int is_interactive_mode();

void set_terminal_font(vita2d_pgf *font);
void terminal_print(const char *text);
void terminal_print_color(const char *text, unsigned int color);
void terminal_print_ansi(const char *text);
void terminal_clear();
int get_line_count();
const char* get_line(int index);
unsigned int get_line_color(int index);
void set_terminal_color(unsigned int color);
void set_scroll_offset(int offset);
int get_scroll_offset();
void adjust_scroll(int delta);
void scroll_to_bottom();

#endif
