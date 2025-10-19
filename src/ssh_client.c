#include "ssh_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "debugScreen.h"

#define printf psvDebugScreenPrintf

char ip[64] = "";
char port[16] = "22";
char username[64] = "";
char password[64] = "";

LIBSSH2_SESSION *session = NULL;
LIBSSH2_CHANNEL *channel = NULL;
int sock = -1;

void filter_special_chars(char *str) {
    char *src = str;
    char *dst = str;
    int in_escape = 0;
    
    while (*src) {
        if (*src == 0x1B) {
            in_escape = 1;
            src++;
            continue;
        }
        
        if (in_escape) {
            if (*src == '[') {
                while (*src && (*src < 'A' || *src > 'Z') && *src != 'm') {
                    src++;
                }
                if (*src) src++;
                in_escape = 0;
                continue;
            } else {
                in_escape = 0;
            }
        }
        
        if ((*src >= 32 && *src <= 126) || *src == '\n' || *src == '\t' || *src == '\r') {
            *dst++ = *src;
        } else {
            *dst++ = ' ';
        }
        src++;
    }
    *dst = '\0';
}

int execute_ssh_command(const char *cmd) {
    if (!session || !channel) {
        printf("Error: SSH session not initialized\n");
        return -1;
    }

    printf("Executing: %s\n", cmd);
    
    if (channel) {
        libssh2_channel_free(channel);
        channel = NULL;
    }
    
    channel = libssh2_channel_open_session(session);
    if (!channel) {
        printf("Error opening SSH channel\n");
        return -1;
    }
    
    int rc = libssh2_channel_exec(channel, cmd);
    if (rc) {
        printf("Error executing command: %d\n", rc);
        return -1;
    }

    printf("Output:\n");
    char buffer[1024];
    ssize_t n;
    
    while ((n = libssh2_channel_read(channel, buffer, sizeof(buffer)-1)) > 0) {
        buffer[n] = '\0';
        filter_special_chars(buffer);
        if (strlen(buffer) > 0) {
            printf("%s", buffer);
        }
    }

    if (n < 0 && n != LIBSSH2_ERROR_EAGAIN) {
        printf("\nError reading output: %ld\n", n);
    } else {
        printf("\nCommand completed\n");
    }
    
    return 0;
}

int ssh_connect() {
    printf("Initializing libssh2...\n");
    int rc = libssh2_init(0);
    if (rc != 0) {
        printf("Error initializing libssh2: %d\n", rc);
        return -1;
    }

    int port_num = atoi(port);
    if (port_num <= 0) port_num = 22;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("Error creating socket: %d\n", sock);
        goto cleanup_libssh2;
    }

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port_num);
    sin.sin_addr.s_addr = inet_addr(ip);

    printf("Connecting to %s:%d...\n", ip, port_num);
    rc = connect(sock, (struct sockaddr *)&sin, sizeof(sin));
    if (rc < 0) {
        printf("Error connecting: %d\n", rc);
        goto cleanup_socket;
    }

    session = libssh2_session_init();
    if (!session) {
        printf("Error creating libssh2 session\n");
        goto cleanup_socket;
    }

    rc = libssh2_session_handshake(session, sock);
    if (rc) {
        printf("Error in SSH handshake: %d\n", rc);
        goto cleanup_session;
    }

    printf("SSH session established\n");

    rc = libssh2_userauth_password(session, username, password);
    if (rc) {
        printf("Error authenticating: %d\n", rc);
        goto cleanup_session;
    }

    printf("Authentication successful\n");

    channel = libssh2_channel_open_session(session);
    if (!channel) {
        printf("Error opening SSH channel\n");
        goto cleanup_session;
    }

    printf("SSH connection established successfully\n");
    return 0;

cleanup_session:
    if (session) {
        libssh2_session_free(session);
        session = NULL;
    }

cleanup_socket:
    if (sock >= 0) {
        close(sock);
        sock = -1;
    }

cleanup_libssh2:
    libssh2_exit();
    return -1;
}

void cleanup_ssh() {
    if (channel) {
        libssh2_channel_close(channel);
        libssh2_channel_free(channel);
        channel = NULL;
    }

    if (session) {
        libssh2_session_disconnect(session, "Normal Shutdown");
        libssh2_session_free(session);
        session = NULL;
    }

    if (sock >= 0) {
        close(sock);
        sock = -1;
    }

    libssh2_exit();
}
