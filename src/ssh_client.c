#include "ssh_client.h"
#include "text.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libssh2.h>

char ip[64] = "";
char port[16] = "22";
char username[64] = "";
char password[64] = "";

LIBSSH2_SESSION *session = NULL;
LIBSSH2_CHANNEL *channel = NULL;
int sock = -1;

static LIBSSH2_CHANNEL *interactive_channel = NULL;
static int in_interactive_mode = 0;

static int current_term_width = TERMINAL_WIDTH;
static int current_term_height = TERMINAL_HEIGHT;

void set_terminal_size(int width, int height) {
    current_term_width = width;
    current_term_height = height;
}

int get_terminal_width() {
    return current_term_width;
}

int get_terminal_height() {
    return current_term_height;
}

int execute_ssh_command(const char *cmd) {
    if (!session) {
        terminal_print("Error: SSH session not initialized");
        return -1;
    }

    if (channel) {
        libssh2_channel_free(channel);
        channel = NULL;
    }
    
    channel = libssh2_channel_open_session(session);
    if (!channel) {
        terminal_print("Error opening SSH channel");
        return -1;
    }
    
    int rc = libssh2_channel_exec(channel, cmd);
    if (rc) {
        terminal_print("Error executing command");
        libssh2_channel_free(channel);
        channel = NULL;
        return -1;
    }

    char buffer[4096];
    ssize_t n;
    int has_output = 0;
    
    char big_buffer[16384] = {0};
    int big_buffer_pos = 0;
    
    while ((n = libssh2_channel_read(channel, buffer, sizeof(buffer)-1)) > 0) {
        buffer[n] = '\0';
        
        if (big_buffer_pos + n < sizeof(big_buffer) - 1) {
            //strcat(big_buffer + big_buffer_pos, buffer);
            memcpy(big_buffer + big_buffer_pos, buffer, n);
            big_buffer_pos += n;
        }
        has_output = 1;
    }

    if (has_output && big_buffer_pos > 0) {
        terminal_print_ansi(big_buffer);
    } else if (!has_output) {
        terminal_print("(No output)");
    }

    if (n < 0 && n != LIBSSH2_ERROR_EAGAIN) {
        char error_msg[64];
        snprintf(error_msg, sizeof(error_msg), "Error reading output: %d", (int)n);
        terminal_print(error_msg);
    }
    
    libssh2_channel_free(channel);
    channel = NULL;
    return 0;
}

int ssh_connect() {
    terminal_print("Initializing libssh2...");
    int rc = libssh2_init(0);
    if (rc != 0) {
        terminal_print("Error initializing libssh2");
        return -1;
    }

    int port_num = atoi(port);
    if (port_num <= 0) port_num = 22;

    terminal_print("Creating socket...");
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        terminal_print("Error creating socket");
        goto cleanup_libssh2;
    }

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port_num);
    sin.sin_addr.s_addr = inet_addr(ip);

    if (sin.sin_addr.s_addr == INADDR_NONE) {
        terminal_print("Error: Invalid IP address format");
        goto cleanup_socket;
    }

    char conn_msg[128];
    snprintf(conn_msg, sizeof(conn_msg), "Connecting to %s:%d...", ip, port_num);
    terminal_print(conn_msg);
    
    rc = connect(sock, (struct sockaddr *)&sin, sizeof(sin));
    if (rc < 0) {
        terminal_print("Error connecting to server");
        goto cleanup_socket;
    }

    terminal_print("Creating SSH session...");
    session = libssh2_session_init();
    if (!session) {
        terminal_print("Error creating libssh2 session");
        goto cleanup_socket;
    }

    terminal_print("Performing SSH handshake...");
    rc = libssh2_session_handshake(session, sock);
    if (rc) {
        terminal_print("Error in SSH handshake");
        goto cleanup_session;
    }

    terminal_print("SSH session established");

    char auth_msg[128];
    snprintf(auth_msg, sizeof(auth_msg), "Authenticating as %s...", username);
    terminal_print(auth_msg);
    
    rc = libssh2_userauth_password(session, username, password);
    if (rc) {
        terminal_print("Error authenticating");
        goto cleanup_session;
    }

    terminal_print("Authentication successful");

    terminal_print("Opening SSH channel...");
    channel = libssh2_channel_open_session(session);
    if (!channel) {
        terminal_print("Error opening SSH channel");
        goto cleanup_session;
    }

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

int start_interactive_shell() {
    if (!session) {
        terminal_print("Error: SSH session not initialized");
        return -1;
    }

    terminal_print("Starting interactive shell (pty)...");

    if (channel) {
        libssh2_channel_close(channel);
        libssh2_channel_free(channel);
        channel = NULL;
    }
    
    if (interactive_channel) {
        libssh2_channel_free(interactive_channel);
        interactive_channel = NULL;
    }
    
    interactive_channel = libssh2_channel_open_session(session);
    if (!interactive_channel) {
        terminal_print("Error opening interactive channel");
        return -1;
    }

    int width  = current_term_width;
    int height = current_term_height;

    char cols_str[8];
    char lines_str[8];
    snprintf(cols_str, sizeof(cols_str), "%d", width);
    snprintf(lines_str, sizeof(lines_str), "%d", height);
    
    int rc = libssh2_channel_request_pty(interactive_channel, "xterm");
    if (rc) {
        terminal_print("Error requesting pty");
        libssh2_channel_free(interactive_channel);
        interactive_channel = NULL;
        return -1;
    }
    
    rc = libssh2_channel_request_pty_size(interactive_channel, width, height);
    if (rc) {
        char warn_msg[128];
        snprintf(warn_msg, sizeof(warn_msg), "Warning: Could not set terminal size (%dx%d)", width, height);
        terminal_print(warn_msg);
    }
    
    libssh2_channel_setenv(interactive_channel, "TERM", "xterm");
    libssh2_channel_setenv(interactive_channel, "COLUMNS", cols_str);
    libssh2_channel_setenv(interactive_channel, "LINES", lines_str);
    
    rc = libssh2_channel_shell(interactive_channel);
    if (rc) {
        terminal_print("Error starting shell");
        libssh2_channel_free(interactive_channel);
        interactive_channel = NULL;
        return -1;
    }
    
    in_interactive_mode = 1;
    
    return 0;
}

int interactive_shell_read(char *buffer, int buffer_size) {
    if (!interactive_channel || !in_interactive_mode) {
        return -1;
    }
    
    int n = libssh2_channel_read(interactive_channel, buffer, buffer_size - 1);
    
    if (n > 0) {
        buffer[n] = '\0';
        return n;
    } else if (n == 0) {
        return 0;
    } else if (n == LIBSSH2_ERROR_EAGAIN) {
        return 0;
    }
    
    return n;
}

int interactive_shell_write(const char *data, int data_len) {
    if (!interactive_channel || !in_interactive_mode) {
        return -1;
    }
    
    int written = 0;
    while (written < data_len) {
        int n = libssh2_channel_write(interactive_channel, data + written, data_len - written);
        if (n > 0) {
            written += n;
        } else if (n == LIBSSH2_ERROR_EAGAIN) {
            break;
        } else {
            return -1;
        }
    }
    
    return written;
}

void stop_interactive_shell() {
    if (interactive_channel) {
        libssh2_channel_send_eof(interactive_channel);
        libssh2_channel_close(interactive_channel);
        libssh2_channel_free(interactive_channel);
        interactive_channel = NULL;
    }
    in_interactive_mode = 0;
    terminal_print("Interactive shell closed");
}

void cleanup_ssh() {
    terminal_print("Cleaning up SSH connection...");
    
    if (in_interactive_mode) {
        stop_interactive_shell();
    }
    
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
    terminal_print("SSH cleanup completed");
}
