#ifndef DISPLAY_H
#define DISPLAY_H

#include <vita2d.h>

#define TERM_X 5
#define TERM_Y 15
#define TERM_WIDTH 950
#define TERM_HEIGHT 430

#define TERMINAL_WIDTH 120
#define TERMINAL_HEIGHT 24

#define TEXT_START_X (TERM_X + 10)
#define TEXT_START_Y (TERM_Y + 5)

#define CHAR_WIDTH 8
#define CHAR_HEIGHT 18

typedef enum {
    APP_STATE_LOADING = 0,
    APP_STATE_MENU,
    APP_STATE_SIMPLE_SSH,
    APP_STATE_PROFILES,
    APP_STATE_ABOUT,
    APP_STATE_TERMINAL
} AppState;

typedef struct {
    char name[32];
    char ip[64];
    char port[16];
    char username[64];
    char password[64];
    int is_default;
} SshProfile;

#define MAX_PROFILES 10

#endif
