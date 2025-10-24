#ifndef SSH_CLIENT_H
#define SSH_CLIENT_H

#include <libssh2.h>

extern char ip[64];
extern char port[16];
extern char username[64];
extern char password[64];

// SSH session globals
extern LIBSSH2_SESSION *session;
extern LIBSSH2_CHANNEL *channel;
extern int sock;

int ssh_connect();
int execute_ssh_command(const char *cmd);
void cleanup_ssh();
int start_interactive_shell();
int interactive_shell_read(char *buffer, int buffer_size);
int interactive_shell_write(const char *data, int data_len);
void stop_interactive_shell();

#endif
