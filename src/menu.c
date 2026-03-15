#include "menu.h"
#include "text.h"
#include "input.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <vita2d.h>

static float loading_progress = 0.0f;
static int loading_frame = 0;

#define LOGO_TEXT "VitaSSH"

void show_loading_screen(vita2d_pgf *font) {
    vita2d_start_drawing();
    vita2d_clear_screen();
    
    for (int y = 0; y < 544; y++) {
        int color_value = 20 + (y / 544.0f) * 30;
        vita2d_draw_line(0, y, 960, y, RGBA8(color_value, color_value, color_value, 255));
    }
    
    int logo_y = 150;
    vita2d_pgf_draw_text(font, 960/2 - vita2d_pgf_text_width(font, 1.2f, LOGO_TEXT)/2, 
                        logo_y, RGBA8(0, 255, 0, 255), 1.2f, LOGO_TEXT);
    
    vita2d_pgf_draw_text(font, 960/2 - vita2d_pgf_text_width(font, 0.8f, "SSH Client for PlayStation Vita")/2,
                        logo_y + 50, RGBA8(180, 180, 180, 255), 0.8f, "SSH Client for PlayStation Vita");
    
    int bar_width = 600;
    int bar_x = (960 - bar_width) / 2;
    int bar_y = 350;
    int bar_height = 20;
    
    vita2d_draw_rectangle(bar_x - 2, bar_y - 2, bar_width + 4, bar_height + 4, RGBA8(80, 80, 80, 255));
    
    vita2d_draw_rectangle(bar_x, bar_y, bar_width, bar_height, RGBA8(40, 40, 40, 255));

    int progress_width = (int)(bar_width * loading_progress);
    vita2d_draw_rectangle(bar_x, bar_y, progress_width, bar_height, RGBA8(0, 255, 0, 255));
    
    char loading_text[32];
    const char *loading_dots[] = {".", "..", "..."};
    snprintf(loading_text, sizeof(loading_text), "Loading%s", loading_dots[loading_frame % 3]);
    
    vita2d_pgf_draw_text(font, 960/2 - vita2d_pgf_text_width(font, 0.7f, loading_text)/2,
                        bar_y + 40, RGBA8(200, 200, 200, 255), 0.7f, loading_text);
    
    vita2d_pgf_draw_text(font, 960/2 - vita2d_pgf_text_width(font, 0.6f, "By Rompelhd")/2,
                        500, RGBA8(150, 150, 150, 255), 0.6f, "By Rompelhd");
    
    vita2d_end_drawing();
    vita2d_swap_buffers();
    
    loading_progress += 0.02f;
    if (loading_progress > 1.0f) loading_progress = 1.0f;
    
    loading_frame++;
}

void show_main_menu(vita2d_pgf *font, AppState *current_state, int *menu_selection) {
    vita2d_start_drawing();
    vita2d_clear_screen();
    
    vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(30, 30, 40, 255));
    
    vita2d_pgf_draw_text(font, 960/2 - vita2d_pgf_text_width(font, 1.2f, "VitaSSH")/2,
                        80, RGBA8(0, 255, 0, 255), 1.2f, "VitaSSH");
    
    const char *menu_items[] = {
        "PROFILES",
        "ABOUT",
        "EXIT"
    };
    
    int menu_y = 200;
    int item_spacing = 70;
    
    for (int i = 0; i < 3; i++) {
        int item_y = menu_y + (i * item_spacing);
        int text_width = vita2d_pgf_text_width(font, 0.9f, menu_items[i]);
        int item_x = 960/2 - text_width/2;
        
        if (i == *menu_selection) {
            vita2d_draw_rectangle(item_x - 20, item_y - 25, text_width + 40, 40, RGBA8(0, 100, 0, 100));
            
            vita2d_draw_rectangle(item_x - 20, item_y - 25, text_width + 40, 2, RGBA8(0, 255, 0, 255));
            vita2d_draw_rectangle(item_x + text_width + 18, item_y - 25, 2, 42, RGBA8(0, 255, 0, 255));
            vita2d_draw_rectangle(item_x - 20, item_y + 15, text_width + 40, 2, RGBA8(0, 255, 0, 255));
            vita2d_draw_rectangle(item_x - 20, item_y - 25, 2, 42, RGBA8(0, 255, 0, 255));
            
            vita2d_pgf_draw_text(font, item_x, item_y, RGBA8(0, 255, 0, 255), 0.9f, menu_items[i]);
            
            vita2d_pgf_draw_text(font, item_x - 40, item_y, RGBA8(0, 255, 0, 255), 0.9f, ">");
            vita2d_pgf_draw_text(font, item_x + text_width + 10, item_y, RGBA8(0, 255, 0, 255), 0.9f, "<");
        } else {
            vita2d_pgf_draw_text(font, item_x, item_y, RGBA8(180, 180, 180, 255), 0.9f, menu_items[i]);
        }
    }
    
    vita2d_pgf_draw_text(font, 20, 500, RGBA8(150, 150, 150, 255), 0.6f, 
                        "Use UP/DOWN to navigate, X to select, START to exit");
    
    vita2d_end_drawing();
    vita2d_swap_buffers();
}

void show_profiles_menu(vita2d_pgf *font, AppState *current_state, SshProfile profiles[], int *profile_count, int *profile_selection) {
    vita2d_start_drawing();
    vita2d_clear_screen();
    
    vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(30, 30, 40, 255));
    
    vita2d_pgf_draw_text(font, 960/2 - vita2d_pgf_text_width(font, 1.1f, "SSH PROFILES")/2,
                        60, RGBA8(0, 255, 0, 255), 1.1f, "SSH PROFILES");
    
    int list_y = 120;
    int item_height = 40;
    int max_visible = 8;
    
    vita2d_draw_rectangle(100, 100, 760, 350, RGBA8(20, 20, 25, 255));
    vita2d_draw_rectangle(98, 98, 764, 354, RGBA8(0, 255, 0, 255));
    
    if (*profile_count == 0) {
        vita2d_pgf_draw_text(font, 960/2 - vita2d_pgf_text_width(font, 0.8f, "No profiles yet")/2,
                            200, RGBA8(150, 150, 150, 255), 0.8f, "No profiles yet");
    } else {
        int start_idx = 0;
        if (*profile_selection >= max_visible) {
            start_idx = *profile_selection - max_visible + 1;
        }
        
        for (int i = 0; i < max_visible && (start_idx + i) < *profile_count; i++) {
            int idx = start_idx + i;
            int item_y = list_y + (i * item_height);
            
            char profile_display[128];
            snprintf(profile_display, sizeof(profile_display), "%s - %s@%s:%s",
                    profiles[idx].name, profiles[idx].username, profiles[idx].ip, profiles[idx].port);
            
            if (idx == *profile_selection) {
                vita2d_draw_rectangle(110, item_y - 15, 740, 35, RGBA8(0, 100, 0, 100));
                vita2d_pgf_draw_text(font, 120, item_y, RGBA8(0, 255, 0, 255), 0.8f, profile_display);
                
                vita2d_pgf_draw_text(font, 100, item_y, RGBA8(0, 255, 0, 255), 0.8f, ">");
            } else {
                vita2d_pgf_draw_text(font, 120, item_y, RGBA8(200, 200, 200, 255), 0.8f, profile_display);
            }
        }
        
        if (*profile_count > max_visible) {
            char scroll_text[32];
            snprintf(scroll_text, sizeof(scroll_text), "%d/%d", *profile_selection + 1, *profile_count);
            vita2d_pgf_draw_text(font, 860, 460, RGBA8(150, 150, 150, 255), 0.6f, scroll_text);
        }
    }
    
    int options_y = 470;
    vita2d_pgf_draw_text(font, 100, options_y, RGBA8(0, 255, 0, 255), 0.7f, "△: New Profile");
    vita2d_pgf_draw_text(font, 250, options_y, RGBA8(150, 150, 255, 255), 0.7f, "○: Back");
    vita2d_pgf_draw_text(font, 380, options_y, RGBA8(0, 255, 0, 255), 0.7f, "X: Connect");
    vita2d_pgf_draw_text(font, 520, options_y, RGBA8(255, 255, 0, 255), 0.7f, "□: Edit");
    vita2d_pgf_draw_text(font, 650, options_y, RGBA8(255, 0, 0, 255), 0.7f, "START: Delete");
    
    vita2d_end_drawing();
    vita2d_swap_buffers();
}

void show_about_screen(vita2d_pgf *font, AppState *current_state) {
    vita2d_start_drawing();
    vita2d_clear_screen();
    
    vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(30, 30, 40, 255));
    
    vita2d_pgf_draw_text(font, 960/2 - vita2d_pgf_text_width(font, 1.2f, "ABOUT VITASSH")/2,
                        70, RGBA8(0, 255, 0, 255), 1.2f, "ABOUT VITASSH");
    
    int content_y = 150;
    int line_spacing = 35;
    
    const char *about_lines[] = {
        "VitaSSH v0.1.3",
        "A full-featured SSH client for PlayStation Vita",
        "",
        "Created by: Rompelhd",
        "",
        "Special thanks to:",
        "- VitaSDK developers",
        "- libssh2 team",
        "",
        "GitHub: github.com/rompelhd/vitassh"
    };
    
    for (int i = 0; i < sizeof(about_lines)/sizeof(about_lines[0]); i++) {
        vita2d_pgf_draw_text(font, 960/2 - vita2d_pgf_text_width(font, 0.7f, about_lines[i])/2,
                            content_y + (i * line_spacing), 
                            (i == 0 || i == 3) ? RGBA8(0, 255, 0, 255) : RGBA8(200, 200, 200, 255), 
                            0.7f, about_lines[i]);
    }
    
   vita2d_pgf_draw_text(font, 960/2 - vita2d_pgf_text_width(font, 0.6f, "Press ○ to return to menu")/2,
                    520, RGBA8(150, 150, 150, 255), 0.6f, "Press ○ to return to menu");

    vita2d_end_drawing();
    vita2d_swap_buffers();
}

void load_profiles(SshProfile profiles[], int *count) {
    *count = 0;
    
    sceIoMkdir("ux0:data/vitassh", 0777);
    
    SceUID fd = sceIoOpen("ux0:data/vitassh/profiles.dat", SCE_O_RDONLY, 0);
    if (fd < 0) {
        add_profile(profiles, count, "Default", "192.168.1.1", "22", "root", "");
        save_profiles(profiles, *count);
        return;
    }
    
    int i = 0;
    while (i < MAX_PROFILES) {
        int bytes = sceIoRead(fd, &profiles[i], sizeof(SshProfile));
        if (bytes <= 0) break;
        i++;
    }
    
    sceIoClose(fd);
    *count = i;
    
    if (*count == 0) {
        add_profile(profiles, count, "Default", "192.168.1.1", "22", "root", "");
    }
}

void save_profiles(SshProfile profiles[], int count) {
    SceUID fd = sceIoOpen("ux0:data/vitassh/profiles.dat", 
                         SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd < 0) return;
    
    for (int i = 0; i < count; i++) {
        sceIoWrite(fd, &profiles[i], sizeof(SshProfile));
    }
    
    sceIoClose(fd);
}

void add_profile(SshProfile profiles[], int *count, const char *name, const char *ip, 
                 const char *port, const char *user, const char *pass) {
    if (*count >= MAX_PROFILES) return;
    
    SshProfile *p = &profiles[*count];
    strncpy(p->name, name, sizeof(p->name) - 1);
    strncpy(p->ip, ip, sizeof(p->ip) - 1);
    strncpy(p->port, port, sizeof(p->port) - 1);
    strncpy(p->username, user, sizeof(p->username) - 1);
    strncpy(p->password, pass, sizeof(p->password) - 1);
    
    (*count)++;
    save_profiles(profiles, *count);
}

void delete_profile(SshProfile profiles[], int *count, int index) {
    if (index < 0 || index >= *count) return;
    
    for (int i = index; i < *count - 1; i++) {
        profiles[i] = profiles[i + 1];
    }
    
    (*count)--;
    
    save_profiles(profiles, *count);
}

void select_profile(SshProfile *selected, SshProfile profiles[], int index) {
    if (index >= 0 && index < MAX_PROFILES) {
        *selected = profiles[index];
    }
}
