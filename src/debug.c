#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <vita2d.h>
#include "debug.h"
#include "text.h"

#define DEBUG_MAX_LINES 100
#define DEBUG_LINE_LEN 256

static char debug_lines[DEBUG_MAX_LINES][DEBUG_LINE_LEN];
static int debug_line_count = 0;
static int debug_active = 0;

void debug_init(void) {
    debug_line_count = 0;
    debug_active = 0;
}

void debug_toggle(void) {
    debug_active = !debug_active;
}

int debug_is_active(void) {
    return debug_active;
}

void debug_log(const char *fmt, ...) {
    char buffer[DEBUG_LINE_LEN];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, DEBUG_LINE_LEN, fmt, args);
    va_end(args);

    if (debug_line_count < DEBUG_MAX_LINES) {
        strcpy(debug_lines[debug_line_count++], buffer);
    } else {
        for (int i = 1; i < DEBUG_MAX_LINES; i++) {
            strcpy(debug_lines[i - 1], debug_lines[i]);
        }
        strcpy(debug_lines[DEBUG_MAX_LINES - 1], buffer);
    }
}

void debug_render(vita2d_pgf *font) {
    vita2d_start_drawing();
    vita2d_clear_screen();

    vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(0, 0, 0, 220));

    int y = 20;
    for (int i = 0; i < debug_line_count; i++) {
        vita2d_pgf_draw_text(font, 10, y,
            RGBA8(0, 255, 0, 255), 0.6f,
            debug_lines[i]);
        y += 18;
        if (y > 520) break;
    }

    vita2d_end_drawing();
    vita2d_swap_buffers();
}