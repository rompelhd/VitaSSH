#include "menu.h"
#include "text.h"
#include <string.h>
#include <stdio.h>
#include <vita2d.h>
#include <psp2/kernel/processmgr.h>

#define PROFILES_FILE "ux0:data/vitassh_profiles.dat"

void show_loading_screen(vita2d_pgf *font) {
    vita2d_start_drawing();
    vita2d_clear_screen();
    
    vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(20, 20, 30, 255));
    
    // Draw title
    vita2d_pgf_draw_text(font, 300, 200, RGBA8(0, 255, 0, 255), 1.2f, "VitaSSH");
    
    // Draw loading text
    vita2d_pgf_draw_text(font, 380, 280, RGBA8(200, 200, 200, 255), 0.8f, "Loading...");
    
    // Draw version
    vita2d_pgf_draw_text(font, 400, 320, RGBA8(150, 150, 150, 255), 0.6f, "v1.0");
    
    vita2d_end_drawing();
    vita2d_swap_buffers();
}

void show_main_menu(vita2d_pgf *font, AppState *current_state, int *menu_selection) {
    vita2d_start_drawing();
    vita2d_clear_screen();
    
    vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(30, 30, 40, 255));
    
    // Draw title
    vita2d_pgf_draw_text(font, 350, 80, RGBA8(0, 255, 0, 255), 1.0f, "VitaSSH - Main Menu");
    
    // Menu options
    const char *options[] = {
        "Simple SSH",
        "Profiles",
        "About",
        "Exit"
    };
    
    int start_y = 180;
    int spacing = 50;
    
    for (int i = 0; i < 4; i++) {
        unsigned int color = (*menu_selection == i) ? RGBA8(255, 255, 0, 255) : RGBA8(200, 200, 200, 255);
        const char *prefix = (*menu_selection == i) ? "> " : "  ";
        
        char line[128];
        snprintf(line, sizeof(line), "%s%s", prefix, options[i]);
        vita2d_pgf_draw_text(font, 380, start_y + i * spacing, color, 0.8f, line);
    }
    
    // Draw controls
    vita2d_pgf_draw_text(font, 200, 480, RGBA8(150, 150, 150, 255), 0.6f, 
                        "UP/DOWN: Navigate  X: Select  START: Exit");
    
    vita2d_end_drawing();
    vita2d_swap_buffers();
}

void show_profiles_menu(vita2d_pgf *font, AppState *current_state, SshProfile profiles[], int *profile_count, int *profile_selection) {
    vita2d_start_drawing();
    vita2d_clear_screen();
    
    vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(30, 30, 40, 255));
    
    // Draw title
    vita2d_pgf_draw_text(font, 350, 60, RGBA8(0, 255, 0, 255), 0.9f, "SSH Profiles");
    
    if (*profile_count == 0) {
        vita2d_pgf_draw_text(font, 300, 200, RGBA8(200, 200, 200, 255), 0.7f, 
                            "No profiles saved");
        vita2d_pgf_draw_text(font, 250, 240, RGBA8(150, 150, 150, 255), 0.6f, 
                            "Press TRIANGLE to create a new profile");
    } else {
        int start_y = 120;
        int spacing = 60;
        int max_display = 6;
        
        for (int i = 0; i < *profile_count && i < max_display; i++) {
            unsigned int color = (*profile_selection == i) ? RGBA8(255, 255, 0, 255) : RGBA8(200, 200, 200, 255);
            const char *prefix = (*profile_selection == i) ? "> " : "  ";
            
            char line[256];
            snprintf(line, sizeof(line), "%s%s - %s@%s:%s", 
                    prefix, profiles[i].name, profiles[i].username, 
                    profiles[i].ip, profiles[i].port);
            vita2d_pgf_draw_text(font, 50, start_y + i * spacing, color, 0.65f, line);
        }
    }
    
    // Draw controls
    vita2d_pgf_draw_text(font, 50, 480, RGBA8(150, 150, 150, 255), 0.55f, 
                        "UP/DOWN: Navigate  X: Connect  TRIANGLE: New  SQUARE: Edit  START: Delete  O: Back");
    
    vita2d_end_drawing();
    vita2d_swap_buffers();
}

void show_about_screen(vita2d_pgf *font, AppState *current_state) {
    vita2d_start_drawing();
    vita2d_clear_screen();
    
    vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(20, 20, 30, 255));
    
    // Draw title
    vita2d_pgf_draw_text(font, 400, 60, RGBA8(0, 255, 0, 255), 0.9f, "About VitaSSH");
    
    // Draw info
    int y = 140;
    int spacing = 35;
    
    vita2d_pgf_draw_text(font, 100, y, RGBA8(200, 200, 200, 255), 0.7f, 
                        "VitaSSH - Interactive SSH Client for PS Vita");
    y += spacing;
    
    vita2d_pgf_draw_text(font, 100, y, RGBA8(150, 150, 150, 255), 0.6f, 
                        "Version: 1.0");
    y += spacing;
    
    vita2d_pgf_draw_text(font, 100, y, RGBA8(150, 150, 150, 255), 0.6f, 
                        "Author: Rompelhd");
    y += spacing * 2;
    
    vita2d_pgf_draw_text(font, 100, y, RGBA8(180, 180, 180, 255), 0.65f, 
                        "Features:");
    y += spacing;
    
    vita2d_pgf_draw_text(font, 120, y, RGBA8(160, 160, 160, 255), 0.6f, 
                        "- Interactive SSH sessions with PTY support");
    y += spacing - 5;
    
    vita2d_pgf_draw_text(font, 120, y, RGBA8(160, 160, 160, 255), 0.6f, 
                        "- Single command execution");
    y += spacing - 5;
    
    vita2d_pgf_draw_text(font, 120, y, RGBA8(160, 160, 160, 255), 0.6f, 
                        "- Profile management");
    y += spacing - 5;
    
    vita2d_pgf_draw_text(font, 120, y, RGBA8(160, 160, 160, 255), 0.6f, 
                        "- ANSI color support");
    y += spacing - 5;
    
    vita2d_pgf_draw_text(font, 120, y, RGBA8(160, 160, 160, 255), 0.6f, 
                        "- Special key support (Ctrl+C, Tab, etc.)");
    
    // Draw controls
    vita2d_pgf_draw_text(font, 300, 480, RGBA8(150, 150, 150, 255), 0.6f, 
                        "Press CIRCLE to return");
    
    vita2d_end_drawing();
    vita2d_swap_buffers();
}

void load_profiles(SshProfile profiles[], int *count) {
    *count = 0;
    
    FILE *file = fopen(PROFILES_FILE, "rb");
    if (!file) {
        return;
    }
    
    int saved_count = 0;
    if (fread(&saved_count, sizeof(int), 1, file) != 1) {
        fclose(file);
        return;
    }
    
    if (saved_count < 0 || saved_count > MAX_PROFILES) {
        fclose(file);
        return;
    }
    
    for (int i = 0; i < saved_count && i < MAX_PROFILES; i++) {
        if (fread(&profiles[i], sizeof(SshProfile), 1, file) == 1) {
            (*count)++;
        } else {
            break;
        }
    }
    
    fclose(file);
}

void save_profiles(SshProfile profiles[], int count) {
    if (count < 0 || count > MAX_PROFILES) {
        return;
    }
    
    FILE *file = fopen(PROFILES_FILE, "wb");
    if (!file) {
        return;
    }
    
    fwrite(&count, sizeof(int), 1, file);
    
    for (int i = 0; i < count; i++) {
        fwrite(&profiles[i], sizeof(SshProfile), 1, file);
    }
    
    fclose(file);
}

void add_profile(SshProfile profiles[], int *count, const char *name, const char *ip, const char *port, const char *user, const char *pass) {
    if (*count >= MAX_PROFILES) {
        return;
    }
    
    SshProfile *profile = &profiles[*count];
    
    strncpy(profile->name, name, sizeof(profile->name) - 1);
    profile->name[sizeof(profile->name) - 1] = '\0';
    
    strncpy(profile->ip, ip, sizeof(profile->ip) - 1);
    profile->ip[sizeof(profile->ip) - 1] = '\0';
    
    strncpy(profile->port, port, sizeof(profile->port) - 1);
    profile->port[sizeof(profile->port) - 1] = '\0';
    
    strncpy(profile->username, user, sizeof(profile->username) - 1);
    profile->username[sizeof(profile->username) - 1] = '\0';
    
    strncpy(profile->password, pass, sizeof(profile->password) - 1);
    profile->password[sizeof(profile->password) - 1] = '\0';
    
    profile->is_default = 0;
    
    (*count)++;
    
    save_profiles(profiles, *count);
}

void delete_profile(SshProfile profiles[], int *count, int index) {
    if (index < 0 || index >= *count) {
        return;
    }
    
    for (int i = index; i < *count - 1; i++) {
        profiles[i] = profiles[i + 1];
    }
    
    (*count)--;
    
    save_profiles(profiles, *count);
}

void select_profile(SshProfile *selected, SshProfile profiles[], int index) {
    if (index < 0 || index >= MAX_PROFILES) {
        return;
    }
    
    *selected = profiles[index];
}
