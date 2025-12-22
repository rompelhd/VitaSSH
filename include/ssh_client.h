#ifndef SSH_CLIENT_H
#define SSH_CLIENT_H

#include "display.h"

extern char ip[64];
extern char port[16];
extern char username[64];
extern char password[64];

void set_terminal_size(int width, int height);
int get_terminal_width();
int get_terminal_height();
int ssh_connect();
int start_interactive_shell();
int interactive_shell_read(char *buffer, int buffer_size);
int interactive_shell_write(const char *data, int data_len);
void stop_interactive_shell();
void cleanup_ssh();

#endif
