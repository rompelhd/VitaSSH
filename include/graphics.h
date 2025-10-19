#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <psp2/gxm.h>
#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>

#define DISPLAY_WIDTH 960
#define DISPLAY_HEIGHT 544
#define DISPLAY_STRIDE_IN_PIXELS 1024
#define DISPLAY_BUFFER_COUNT 2
#define DISPLAY_MAX_PENDING_SWAPS 1

typedef struct {
    void* data;
    SceGxmSyncObject* sync;
    SceGxmColorSurface surf;
    SceUID uid;
} displayBuffer;

void gxm_init();
void gxm_swap();
void gxm_term();
void clear_screen();

extern displayBuffer dbuf[DISPLAY_BUFFER_COUNT];
extern unsigned int backBufferIndex;
extern unsigned int frontBufferIndex;

#endif
