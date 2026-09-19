#ifndef DEBUG_H
#define DEBUG_H

void debug_init(void);
void debug_log(const char *fmt, ...);
void debug_render(vita2d_pgf *font);
void debug_toggle(void);
int debug_is_active(void);

#endif