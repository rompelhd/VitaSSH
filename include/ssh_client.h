#ifndef SSH_CLIENT_H
#define SSH_CLIENT_H

#include <libssh2.h>

// Connection configuration
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
void filter_special_chars(char *str);

#endif
