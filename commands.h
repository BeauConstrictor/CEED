/*
 * commands.h - simple command dispatcher for cedit
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include "editor.h"
#include "hole.h"
#include "constants.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef void (*command_handler)(editor*, const char* arg);

typedef struct {
    const char* name;

    command_handler handler;
} ex_command;


void run_command(editor* cedit, char* cmd);

#endif // COMMANDS_H

#ifdef COMMANDS_IMPLEMENTATION

static void cmd_quit(editor* cedit, const char* arg) {
    exit(0);
}

static void cmd_echo(editor* cedit, const char* arg) {
    sprintf(cedit->status, "%s", arg);
}

static void cmd_edit(editor* cedit, const char* arg) {
    free_buf(cedit->buf);
    cedit->buf = create_buf(65536);
    sprintf(cedit->buf->path, "%s", arg);

    FILE* f = fopen(arg, "r");
    if (f != NULL) {
        buf_insertf(cedit->buf, f);
        fclose(f);
        size_t len = buf_len(cedit->buf);
        sprintf(cedit->status, "'%s' %d bytes", arg, len);
    } else if (*arg == '\0') {
        sprintf(cedit->status, "unnamed buffer, 0 bytes");
    } else {
        sprintf(cedit->status, "'%s', 0 bytes", arg);
    }
}

static void cmd_path(editor* cedit, const char* arg) {
    char* path = cedit->buf->path;
    if (*path == '\0'){
        sprintf(cedit->status, "unnamed buffer");
    } else {
        sprintf(cedit->status, "'%s'", cedit->buf->path);
    }
}

static void cmd_write(editor* cedit, const char* arg) {
    const char* path;

    if (*arg == '\0' && *cedit->buf->path == '\0') {
        sprintf(cedit->status, RED"No file name"RESET);
        return;
    } else if (*arg != '\0') {
        path = arg;
    } else {
        path = cedit->buf->path;
    }

    FILE* f = fopen(path, "w");
    if (f == NULL) {
        sprintf(cedit->status, RED"Cannot write into '%s'"RESET, path);
        return;
    }
  
    buf_fwrite(cedit->buf, f);
    fclose(f);
    size_t len = buf_len(cedit->buf);
    sprintf(cedit->status, "'%s', %d bytes written", path, len);
}

static ex_command commands[] = {
    { "q",         cmd_quit    },
    { "echo",      cmd_echo    },
    { "e",         cmd_edit    },
    { "path",      cmd_path    },
    { "w",         cmd_write   },
};

static const size_t cmd_count = sizeof(commands) / sizeof(commands[0]);

void run_command(editor* cedit, char* cmd) {
    if (strlen(cmd) == 0) return;

    char* arg = cmd;
    while (*arg != '\0') {
        if (*arg == ' ') {
            *arg = '\0';
            arg++;
            break;
        }
        arg++;
    }

    for (size_t i = 0; i < cmd_count; i++) {
        if (strcmp(commands[i].name, cmd) == 0) {
            commands[i].handler(cedit, arg);
            return;
        }
    }

    sprintf(cedit->status, RED"Not an editor command: %s"RESET, cmd);
}

#endif // COMMANDS_IMPLEMENTATION

