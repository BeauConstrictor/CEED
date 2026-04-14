/*
 * ceed.c - an extremely lightweight vi-like text editor designed for
 *          remoting into embedded devices (serial/telnet).
 *
 * CEED is licensed under the open source MIT license. For more 
 * details see https://opensource.org/license/MIT. Copyright 2026,
 * Beau Constrictor.
 *
 * CEED has no dependencies besides libc, and uses straightforward
 * code with minimal macros, so is easy to read and learn from. The
 * core, hole.h (a generic, performant text buffer implementation),
 * is easy to use as a starting point for your own text editor
 * project.
 *
 */

// TODO:
// * print all output to a screen buffer and diff it with the 
//   previous buffer, manually moving cursor and drawing changes
//   only.

#include "hole.h"
#include "editor.h"
#include "commands.h"

#include "implementations.h"
#include "constants.h"

#include <sys/ioctl.h>
#include <termios.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define GREETING "CEED - C Embedded EDitor (v0.1.0)"

void draw_editor(editor* cedit) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    printf("\033[H\033[2J");

    print_buf(cedit->buf, w.ws_row-1);
    printf("\n%s", cedit->status);
    if (cedit->mode == command) printf("\033[7m \033[0m");
}

void handle_normal_mode_key(editor *cedit, char key) {
    switch (key) {
        case 'h':
            cursor_left(cedit->buf);
            break;
        case 'j':
            cursor_down(cedit->buf);
            break;
        case 'k':
            cursor_up(cedit->buf);
            break;
        case 'l':
            cursor_right(cedit->buf);
            break;

        case 'a':
            cursor_right(cedit->buf);
            // fallthrough to reuse 'i' key logic
        case 'i':
            sprintf(cedit->status, "-- INSERT --");
            cedit->mode = insert;
            break;

        case ':':
            sprintf(cedit->status, ":");
            cedit->mode = command;
            break;

        default:
            sprintf(cedit->status, "%c", key);
    }
}

void handle_insert_mode_key(editor* cedit, char key) {
    switch (key) {
        case '\b':
        case '\177':
            buf_backspace(cedit->buf);
            break;

        case '\033':
            cedit->mode = normal;
            sprintf(cedit->status, "");
            break;

        default:
            buf_insertc(cedit->buf, key);
    }
}

void handle_command_mode_key(editor* cedit, char key) {
    size_t cmd_len = strlen(cedit->status) - 1;

    switch (key) {
        case '\n':
            char cmd[STATUS_LENGTH];
            strcpy(cmd, cedit->status+1);
            run_command(cedit, cmd);
            cedit->mode = normal;
            break;

        case '\b':
        case '\177':
            if (cmd_len < 1) break;
            cedit->status[cmd_len] = '\0';
            break;

        case '\033':
            cedit->mode = normal;
            *cedit->status = '\0';
            break;

        default:
            if (strlen(cedit->status) >= STATUS_LENGTH) break;
            cedit->status[cmd_len+1] = key;
            cedit->status[cmd_len+2] = '\0';
    }
}

void handle_key(editor* cedit, char key) {
    switch (cedit->mode) {
        case normal:
            handle_normal_mode_key(cedit, key);
            break;

        case insert:
            handle_insert_mode_key(cedit, key);
            break;

        case command:
            handle_command_mode_key(cedit, key);
            break;
    }
}

struct termios oldt, newt;

void initialise_terminal() {
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    printf("\033[?25l");
    printf("\033[?1049h");
}

void cleanup_terminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    printf("\033[?25h");
    printf("\033[?1049l");
}

int main(void) {
    editor cedit;
    cedit.mode = normal;
    snprintf(cedit.status, sizeof(cedit.status), GREETING);
   
    cedit.buf = create_buf(INITIAL_BUFFER_SIZE);

    initialise_terminal();
    atexit(cleanup_terminal);

    while (true) {
        draw_editor(&cedit);
    
        char key = getchar();
        handle_key(&cedit, key);
    }

    return 0;
}
