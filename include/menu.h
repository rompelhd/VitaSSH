#ifndef MENU_H
#define MENU_H

#include "display.h"

void show_loading_screen(vita2d_pgf *font);
void show_main_menu(vita2d_pgf *font, AppState *current_state, int *menu_selection);
void show_profiles_menu(vita2d_pgf *font, AppState *current_state, SshProfile profiles[], int *profile_count, int *profile_selection);
void show_about_screen(vita2d_pgf *font, AppState *current_state);

void load_profiles(SshProfile profiles[], int *count);
void save_profiles(SshProfile profiles[], int count);
void add_profile(SshProfile profiles[], int *count, const char *name, const char *ip, const char *port, const char *user, const char *pass);
void delete_profile(SshProfile profiles[], int *count, int index);
void select_profile(SshProfile *selected, SshProfile profiles[], int index);

#endif
