#include <string.h>
#include <stdlib.h>
#include <vita2d.h>
#include <stdio.h>

#include "ansi_colors.h"
#include "text.h"

unsigned int ansi_to_vita_color(int ansi_code) {
    char debug_msg[64];
    
    switch (ansi_code) {
        // Standard foreground colors
        case 30: return RGBA8(0, 0, 0, 255);       // Black
        case 31: return RGBA8(255, 0, 0, 255);     // Red
        case 32: return RGBA8(0, 255, 0, 255);     // Green
        case 33: return RGBA8(255, 255, 0, 255);   // Yellow
        case 34: return RGBA8(0, 0, 255, 255);     // Blue
        case 35: return RGBA8(255, 0, 255, 255);   // Magenta
        case 36: return RGBA8(0, 255, 255, 255);   // Cyan
        case 37: return RGBA8(255, 255, 255, 255); // White
        
        // Bright foreground colors
        case 90: return RGBA8(128, 128, 128, 255);   // Gray
        case 91: return RGBA8(255, 128, 128, 255);   // Bright red
        case 92: return RGBA8(128, 255, 128, 255);   // Bright green
        case 93: return RGBA8(255, 255, 128, 255);   // Bright yellow
        case 94: return RGBA8(128, 128, 255, 255);   // Bright blue
        case 95: return RGBA8(255, 128, 255, 255);   // Bright magenta
        case 96: return RGBA8(128, 255, 255, 255);   // Bright cyan
        case 97: return RGBA8(255, 255, 255, 255);   // Bright white
        
        // Background colors (treated as foreground for simplicity)
        case 40: case 100: return RGBA8(0, 0, 0, 255);       // Black background
        case 41: case 101: return RGBA8(255, 0, 0, 255);     // Red background
        case 42: case 102: return RGBA8(0, 255, 0, 255);     // Green background
        case 43: case 103: return RGBA8(255, 255, 0, 255);   // Yellow background
        case 44: case 104: return RGBA8(0, 0, 255, 255);     // Blue background
        case 45: case 105: return RGBA8(255, 0, 255, 255);   // Magenta background
        case 46: case 106: return RGBA8(0, 255, 255, 255);   // Cyan background
        case 47: case 107: return RGBA8(255, 255, 255, 255); // White background
        
        // Text attributes
        case 0:  return RGBA8(0, 255, 0, 255);     // Reset to default green
        case 1:  return RGBA8(255, 255, 255, 255); // Bold -> white
        case 2:  return RGBA8(128, 128, 128, 255); // Dim -> gray
        case 4:  return RGBA8(0, 255, 0, 255);     // Underline -> green
        case 7:  return RGBA8(255, 255, 255, 255); // Inverse -> white
        
        default: 
            snprintf(debug_msg, sizeof(debug_msg), "Unknown ANSI code: %d", ansi_code);
            terminal_print(debug_msg);
            return RGBA8(0, 255, 0, 255); // Default to green
    }
}

void process_ansi_colors(const char *input, char *output, unsigned int *current_color) {
    const char *src = input;
    char *dst = output;
    int in_escape = 0;
    int in_bracket = 0;
    char color_code[16] = {0};
    int color_idx = 0;
    
    while (*src) {
        if (!in_escape && *src == 0x1B) { // ESC character
            in_escape = 1;
            src++;
            continue;
        }
        
        if (in_escape) {
            if (*src == '[') {
                in_bracket = 1;
                src++;
                continue;
            }
            
            if (in_bracket) {
                if (*src >= '0' && *src <= '9') {
                    // Build color code string
                    if (color_idx < 15) {
                        color_code[color_idx++] = *src;
                    }
                } else if (*src == ';') {
                    // Multiple codes separated by semicolon
                    if (color_idx > 0) {
                        color_code[color_idx] = '\0';
                        int code = atoi(color_code);
                        *current_color = ansi_to_vita_color(code);
                        color_idx = 0;
                    }
                } else if (*src == 'm') {
                    if (color_idx > 0) {
                        color_code[color_idx] = '\0';
                        int code = atoi(color_code);
                        *current_color = ansi_to_vita_color(code);
                    }
                    in_escape = 0;
                    in_bracket = 0;
                    color_idx = 0;
                    src++;
                    continue;
                } else {
                    in_escape = 0;
                    in_bracket = 0;
                    color_idx = 0;
                }
            } else {
                in_escape = 0;
            }
            src++;
            continue;
        }
        
        // Normal character
        *dst++ = *src++;
    }
    
    *dst = '\0';
}
