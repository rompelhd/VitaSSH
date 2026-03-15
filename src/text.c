#include "text.h"
#include "display.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <vita2d.h>

char lines[MAX_LINES][MAX_LINE_LENGTH];
unsigned int line_colors[MAX_LINES];
int line_count = 0;
unsigned int current_color = RGBA8(0, 255, 0, 255);
vita2d_pgf *terminal_font = NULL;

int scroll_offset = 0;
int auto_scroll = 1;

static int in_interactive_mode = 0;

unsigned int ansi_colors[] = {
    RGBA8(0, 0, 0, 255),       // 0: Black
    RGBA8(255, 0, 0, 255),     // 1: Red
    RGBA8(0, 255, 0, 255),     // 2: Green
    RGBA8(255, 255, 0, 255),   // 3: Yellow
    RGBA8(0, 0, 255, 255),     // 4: Blue
    RGBA8(255, 0, 255, 255),   // 5: Magenta
    RGBA8(0, 255, 255, 255),   // 6: Cyan
    RGBA8(255, 255, 255, 255), // 7: White
    RGBA8(128, 128, 128, 255), // 8: Bright Black (Gray)
    RGBA8(255, 128, 128, 255), // 9: Bright Red
    RGBA8(128, 255, 128, 255), // 10: Bright Green
    RGBA8(255, 255, 128, 255), // 11: Bright Yellow
    RGBA8(128, 128, 255, 255), // 12: Bright Blue
    RGBA8(255, 128, 255, 255), // 13: Bright Magenta
    RGBA8(128, 255, 255, 255), // 14: Bright Cyan
    RGBA8(255, 255, 255, 255)  // 15: Bright White
};

static terminal_t term;

static char last_processed_line[256] = "";
static int suppress_duplicate_prompt = 0;

void set_interactive_mode(int enabled) {
    in_interactive_mode = enabled;
    if (enabled) {
        memset(last_processed_line, 0, sizeof(last_processed_line));
        suppress_duplicate_prompt = 1;
    }
}

int is_interactive_mode() {
    return in_interactive_mode;
}

void terminal_init() {
    memset(&term, 0, sizeof(term));
    term.current_fg = RGBA8(0, 255, 0, 255);
    term.current_bg = RGBA8(0, 0, 0, 255);
    term.cursor_x = 0;
    term.cursor_y = 0;
    terminal_clear_screen();
}

void terminal_clear_screen() {
    for (int y = 0; y < TERMINAL_HEIGHT; y++) {
        for (int x = 0; x < TERMINAL_WIDTH; x++) {
            term.screen[y][x].character = ' ';
            term.screen[y][x].fg_color = term.current_fg;
            term.screen[y][x].bg_color = term.current_bg;
            term.screen[y][x].bold = term.bold;
            term.screen[y][x].underline = term.underline;
        }
    }
    term.cursor_x = 0;
    term.cursor_y = 0;
}

void terminal_clear_line(int line) {
    if (line < 0 || line >= TERMINAL_HEIGHT) return;
    
    for (int x = 0; x < TERMINAL_WIDTH; x++) {
        term.screen[line][x].character = ' ';
        term.screen[line][x].fg_color = term.current_fg;
        term.screen[line][x].bg_color = term.current_bg;
        term.screen[line][x].bold = term.bold;
        term.screen[line][x].underline = term.underline;
    }
}

void terminal_put_char(char c) {
    if (term.cursor_y >= TERMINAL_HEIGHT) {
        terminal_scroll_up();
        term.cursor_y = TERMINAL_HEIGHT - 1;
    }
    
    if (term.cursor_x >= TERMINAL_WIDTH) {
        term.cursor_x = 0;
        term.cursor_y++;
        if (term.cursor_y >= TERMINAL_HEIGHT) {
            terminal_scroll_up();
            term.cursor_y = TERMINAL_HEIGHT - 1;
        }
    }
    
    term.screen[term.cursor_y][term.cursor_x].character = c;
    term.screen[term.cursor_y][term.cursor_x].fg_color = term.current_fg;
    term.screen[term.cursor_y][term.cursor_x].bg_color = term.current_bg;
    term.screen[term.cursor_y][term.cursor_x].bold = term.bold;
    term.screen[term.cursor_y][term.cursor_x].underline = term.underline;
    
    term.cursor_x++;
}

void terminal_scroll_up() {
    if (term.scrollback_count < MAX_SCROLLBACK) {
        for (int x = 0; x < TERMINAL_WIDTH; x++) {
            term.scrollback[term.scrollback_count][x] = term.screen[0][x].character;
        }
        term.scrollback_count++;
    }
    
    for (int y = 1; y < TERMINAL_HEIGHT; y++) {
        for (int x = 0; x < TERMINAL_WIDTH; x++) {
            term.screen[y-1][x] = term.screen[y][x];
        }
    }
    
    terminal_clear_line(TERMINAL_HEIGHT - 1);
}

void process_ansi_command() {
    term.ansi_buffer[term.ansi_buffer_pos] = '\0';
    
    char *token = strtok(term.ansi_buffer, ";");
    term.ansi_param_count = 0;
    
    while (token && term.ansi_param_count < 16) {
        term.ansi_params[term.ansi_param_count++] = atoi(token);
        token = strtok(NULL, ";");
    }
    
    char command = term.ansi_buffer[strlen(term.ansi_buffer)-1];
    
    switch (command) {
        case 'H': case 'f':
            term.cursor_y = (term.ansi_param_count > 0 && term.ansi_params[0] > 0) ? 
                           term.ansi_params[0] - 1 : 0;
            term.cursor_x = (term.ansi_param_count > 1 && term.ansi_params[1] > 0) ? 
                           term.ansi_params[1] - 1 : 0;
            break;
            
        case 'A':
            term.cursor_y -= (term.ansi_param_count > 0 && term.ansi_params[0] > 0) ? 
                            term.ansi_params[0] : 1;
            if (term.cursor_y < 0) term.cursor_y = 0;
            break;
            
        case 'B':
            term.cursor_y += (term.ansi_param_count > 0 && term.ansi_params[0] > 0) ? 
                            term.ansi_params[0] : 1;
            if (term.cursor_y >= TERMINAL_HEIGHT) term.cursor_y = TERMINAL_HEIGHT - 1;
            break;
            
        case 'C':
            term.cursor_x += (term.ansi_param_count > 0 && term.ansi_params[0] > 0) ? 
                            term.ansi_params[0] : 1;
            if (term.cursor_x >= TERMINAL_WIDTH) term.cursor_x = TERMINAL_WIDTH - 1;
            break;
            
        case 'D':
            term.cursor_x -= (term.ansi_param_count > 0 && term.ansi_params[0] > 0) ? 
                            term.ansi_params[0] : 1;
            if (term.cursor_x < 0) term.cursor_x = 0;
            break;
            
        case 'J':
            if (term.ansi_params[0] == 2) {
                terminal_clear_screen();
            } else if (term.ansi_params[0] == 0) {
                for (int y = term.cursor_y; y < TERMINAL_HEIGHT; y++) {
                    for (int x = (y == term.cursor_y ? term.cursor_x : 0); x < TERMINAL_WIDTH; x++) {
                        term.screen[y][x].character = ' ';
                    }
                }
            }
            break;
            
        case 'K':
            if (term.ansi_params[0] == 0) {
                for (int x = term.cursor_x; x < TERMINAL_WIDTH; x++) {
                    term.screen[term.cursor_y][x].character = ' ';
                }
            } else if (term.ansi_params[0] == 1) {
                for (int x = 0; x <= term.cursor_x; x++) {
                    term.screen[term.cursor_y][x].character = ' ';
                }
            } else if (term.ansi_params[0] == 2) {
                terminal_clear_line(term.cursor_y);
            }
            break;
            
        case 'm':
            process_ansi_colors();
            break;
            
        case 's':
            term.saved_cursor_x = term.cursor_x;
            term.saved_cursor_y = term.cursor_y;
            break;
            
        case 'u':
            term.cursor_x = term.saved_cursor_x;
            term.cursor_y = term.saved_cursor_y;
            break;
    }
    
    term.ansi_buffer_pos = 0;
}

void process_ansi_colors() {
    for (int i = 0; i < term.ansi_param_count; i++) {
        int code = term.ansi_params[i];
        
        switch (code) {
            case 0: // Reset
                term.current_fg = RGBA8(0, 255, 0, 255);
                term.current_bg = RGBA8(0, 0, 0, 255);
                term.bold = 0;
                term.underline = 0;
                term.reverse_video = 0;
                break;
                
            case 1: // Bold
                term.bold = 1;
                break;
                
            case 4: // Underline
                term.underline = 1;
                break;
                
            case 7: // Reverse video
                term.reverse_video = 1;
                unsigned int temp = term.current_fg;
                term.current_fg = term.current_bg;
                term.current_bg = temp;
                break;
                
            case 30: case 31: case 32: case 33: case 34: case 35: case 36: case 37:
                term.current_fg = ansi_colors[code - 30 + (term.bold ? 8 : 0)];
                break;
                
            case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47:
                term.current_bg = ansi_colors[code - 40];
                break;
                
            case 90: case 91: case 92: case 93: case 94: case 95: case 96: case 97:
                term.current_fg = ansi_colors[code - 90 + 8];
                break;
                
            case 100: case 101: case 102: case 103: case 104: case 105: case 106: case 107:
                term.current_bg = ansi_colors[code - 100 + 8];
                break;
        }
    }
}

static int is_prompt_line(const char *line) {
    if (!line || strlen(line) == 0) return 0;
    
    const char *prompt_patterns[] = {
        "$ ", "> ", "# ", "~$ ", ":~$ ", "]$ ", ")% ", "@", "PS1=", "\\$"
    };
    
    for (int i = 0; i < sizeof(prompt_patterns) / sizeof(prompt_patterns[0]); i++) {
        if (strstr(line, prompt_patterns[i]) != NULL) {
            return 1;
        }
    }
    
    return 0;
}

void terminal_process_data(const char *data, int length) {
    static char last_processed_line[256] = {0};
    static int skip_next_prompt = 0;
    
    for (int i = 0; i < length; i++) {
        char c = data[i];
        
        if (term.ansi_buffer_pos > 0) {
            term.ansi_buffer[term.ansi_buffer_pos++] = c;
            
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
                process_ansi_command();
            } else if (term.ansi_buffer_pos >= sizeof(term.ansi_buffer) - 1) {
                term.ansi_buffer_pos = 0;
            }
        } else if (c == '\033') {
            term.ansi_buffer_pos = 0;
            term.ansi_buffer[term.ansi_buffer_pos++] = c;
        } else {
            switch (c) {
                case '\r':
                    term.cursor_x = 0;
                    break;
                    
                case '\n':
                    term.cursor_x = 0;
                    term.cursor_y++;
                    if (term.cursor_y >= TERMINAL_HEIGHT) {
                        terminal_scroll_up();
                        term.cursor_y = TERMINAL_HEIGHT - 1;
                    }
                    break;
                    
                case '\t':
                    term.cursor_x = (term.cursor_x + 8) & ~7;
                    if (term.cursor_x >= TERMINAL_WIDTH) {
                        term.cursor_x = 0;
                        term.cursor_y++;
                    }
                    break;
                    
                case '\b':
                    if (term.cursor_x > 0) {
                        term.cursor_x--;
                    }
                    break;
                    
                default:
                    if (c >= 32 && c <= 126) {
                        terminal_put_char(c);
                    }
                    break;
            }
        }
    }
}

void terminal_render(vita2d_pgf *font) {
    int start_x = TEXT_START_X;  // term_x + 10 = 5 + 10 = 15
    int start_y = TEXT_START_Y;  // term_y + 5 = 15 + 5 = 20
    int char_width = CHAR_WIDTH;
    int char_height = CHAR_HEIGHT;
    
    int max_cols = (TERM_WIDTH - 20) / char_width;  // TERM_WIDTH = 950
    int max_rows = (TERM_HEIGHT - 10) / char_height; // TERM_HEIGHT = 430
    
    int render_cols = max_cols < TERMINAL_WIDTH ? max_cols : TERMINAL_WIDTH;
    int render_rows = max_rows < TERMINAL_HEIGHT ? max_rows : TERMINAL_HEIGHT;
    
    for (int y = 0; y < render_rows; y++) {
        for (int x = 0; x < render_cols; x++) {
            terminal_cell_t *cell = &term.screen[y][x];
            
            int screen_x = start_x + x * char_width;
            int screen_y = start_y + y * char_height;

            vita2d_draw_rectangle(
                screen_x,
                screen_y,
                char_width, char_height,
                cell->bg_color
            );
            
            char char_str[2] = {cell->character, '\0'};
            unsigned int text_color = cell->fg_color;

            if (cell->bold) {
                text_color = RGBA8(
                    (text_color >> 0) & 0xFF,
                    (text_color >> 8) & 0xFF, 
                    (text_color >> 16) & 0xFF,
                    255
                );
            }
            
            vita2d_pgf_draw_text(font, 
                screen_x,
                screen_y + char_height - 4,
                text_color, 0.7f, char_str);
        }
    }
    
    static int cursor_blink = 0;
    static uint64_t last_blink_time = 0;
    uint64_t current_time = sceKernelGetSystemTimeWide();
    
    if (current_time - last_blink_time > 500000) { // 500ms
        cursor_blink = !cursor_blink;
        last_blink_time = current_time;
    }
    
    if (cursor_blink && term.cursor_y < render_rows && term.cursor_x < render_cols) {
        int cursor_screen_x = start_x + term.cursor_x * char_width;
        int cursor_screen_y = start_y + term.cursor_y * char_height;
        
        vita2d_draw_rectangle(
            cursor_screen_x,
            cursor_screen_y,
            char_width, char_height,
            term.current_fg
        );
        
        if (term.screen[term.cursor_y][term.cursor_x].character != ' ') {
            char char_str[2] = {term.screen[term.cursor_y][term.cursor_x].character, '\0'};
            vita2d_pgf_draw_text(font,
                cursor_screen_x,
                cursor_screen_y + char_height - 4,
                term.current_bg, 0.7f, char_str);
        }
    }
}

// OLD SYSTEM

void set_terminal_font(vita2d_pgf *font) {
    terminal_font = font;
}

void terminal_print(const char *text) {
    if (in_interactive_mode) {
        return;
    }
    
    if (text == NULL || strlen(text) == 0) return;

    if (line_count >= MAX_LINES) {
        for (int i = 1; i < MAX_LINES; i++) {
            strcpy(lines[i-1], lines[i]);
            line_colors[i-1] = line_colors[i];
        }
        line_count--;
        if (scroll_offset > 0) {
            scroll_offset--;
        }
    }

    strncpy(lines[line_count], text, MAX_LINE_LENGTH - 1);
    lines[line_count][MAX_LINE_LENGTH - 1] = '\0';
    line_colors[line_count] = current_color;
    line_count++;

    if (auto_scroll) {
        scroll_to_bottom();
    }
}

void terminal_print_color(const char *text, unsigned int color) {
    if (in_interactive_mode) {
        return;
    }
    
    unsigned int old_color = current_color;
    current_color = color;
    terminal_print(text);
    current_color = old_color;
}

void process_ansi_sequence(const char *seq) {
    if (seq[0] != '[') return;
    
    const char *ptr = seq + 1;
    int code = 0;
    
    while (*ptr >= '0' && *ptr <= '9') {
        code = code * 10 + (*ptr - '0');
        ptr++;
    }
    
    char command = *ptr;
    
    switch (command) {
        case 'm':
            switch (code) {
                case 0: current_color = RGBA8(0, 255, 0, 255); break;
                case 1:
                    if (current_color == RGBA8(0, 255, 0, 255)) 
                        current_color = RGBA8(128, 255, 128, 255);
                    break;
                case 30: case 31: case 32: case 33: case 34: case 35: case 36: case 37:
                    current_color = ansi_colors[code - 30];
                    break;
                case 90: case 91: case 92: case 93: case 94: case 95: case 96: case 97:
                    current_color = ansi_colors[code - 90 + 8];
                    break;
                default:
                    if (code >= 30 && code <= 37) {
                        current_color = ansi_colors[code - 30];
                    }
                    break;
            }
            break;
        case 'H': case 'f': case 'A': case 'B': case 'C': case 'D': 
        case 'J': case 'K': case 's': case 'u': case 'h': case 'l': case 'r':
            break;
        default:
            break;
    }
}

void terminal_print_ansi(const char *text) {
    if (in_interactive_mode) {
        return;
    }
    
    if (text == NULL) return;
    
    char clean_text[MAX_LINE_LENGTH] = {0};
    int clean_pos = 0;
    const char *ptr = text;
    
    while (*ptr && clean_pos < MAX_LINE_LENGTH - 1) {
        if (*ptr == '\033') {
            ptr++;
            const char *seq_start = ptr;
            while (*ptr && (*ptr < 'A' || *ptr > 'Z') && (*ptr < 'a' || *ptr > 'z')) {
                ptr++;
            }
            if (*ptr) {
                char ansi_seq[32] = {0};
                int seq_len = ptr - seq_start + 1;
                if (seq_len > 0 && seq_len < 32) {
                    strncpy(ansi_seq, seq_start, seq_len);
                    ansi_seq[seq_len] = '\0';
                    process_ansi_sequence(ansi_seq);
                }
                ptr++;
            }
        } 
        else if (*ptr == '\r') {
            ptr++;
        }
        else if (*ptr == '\n') {
            if (clean_pos > 0) {
                clean_text[clean_pos] = '\0';
                terminal_print(clean_text);
                clean_pos = 0;
            }
            ptr++;
        }
        else if (*ptr == '\t') {
            if (clean_pos < MAX_LINE_LENGTH - 4) {
                clean_text[clean_pos++] = ' ';
                clean_text[clean_pos++] = ' ';
                clean_text[clean_pos++] = ' ';
                clean_text[clean_pos++] = ' ';
            }
            ptr++;
        }
        else {
            clean_text[clean_pos++] = *ptr++;
        }
    }
    
    if (clean_pos > 0) {
        clean_text[clean_pos] = '\0';
        terminal_print(clean_text);
    }
}

void terminal_clear() {
    line_count = 0;
    scroll_offset = 0;
    current_color = RGBA8(0, 255, 0, 255);
    auto_scroll = 1;
    
    if (in_interactive_mode) {
        terminal_init();
    }
}

int get_line_count() {
    return line_count;
}

const char* get_line(int index) {
    if (index >= 0 && index < line_count) {
        return lines[index];
    }
    return "";
}

unsigned int get_line_color(int index) {
    if (index >= 0 && index < line_count) {
        return line_colors[index];
    }
    return RGBA8(0, 255, 0, 255);
}

void set_terminal_color(unsigned int color) {
    current_color = color;
}

void set_scroll_offset(int offset) {
    scroll_offset = offset;
    int max_scroll = line_count > 25 ? line_count - 25 : 0;
    if (scroll_offset > max_scroll) {
        scroll_offset = max_scroll;
    }
    if (scroll_offset < 0) {
        scroll_offset = 0;
    }
    if (scroll_offset < max_scroll) {
        auto_scroll = 0;
    }
}

int get_scroll_offset() {
    return scroll_offset;
}

void adjust_scroll(int delta) {
    set_scroll_offset(scroll_offset + delta);
}

void scroll_to_bottom() {
    int max_scroll = line_count > 25 ? line_count - 25 : 0;
    scroll_offset = max_scroll;
    auto_scroll = 1;
}
