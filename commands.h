/*
 * commands.h - simple command dispatcher for ceed
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


void run_command(editor* ceed, char* cmd);

#endif // COMMANDS_H

#ifdef COMMANDS_IMPLEMENTATION

static void cmd_force_quit(editor* ceed, const char* arg) {
    exit(0);
}

static void cmd_quit(editor* ceed, const char* arg) {
    if (ceed->buf->dirty) {
        sprintf(ceed->status, RED"No write since last change"RESET);
    } else {
        cmd_force_quit(ceed, arg);
    }
}

static void cmd_echo(editor* ceed, const char* arg) {
    sprintf(ceed->status, "%s", arg);
}

static void cmd_force_edit(editor* ceed, const char* arg) {
    free_buf(ceed->buf);
    ceed->buf = create_buf(INITIAL_BUFFER_SIZE);
    sprintf(ceed->buf->path, "%s", arg);

    FILE* f = fopen(arg, "r");
    if (f != NULL) {
        buf_insertf(ceed->buf, f);
        fclose(f);
        size_t len = buf_len(ceed->buf);
        sprintf(ceed->status, "'%s', %d bytes", arg, len);
    } else if (*arg == '\0') {
        sprintf(ceed->status, "unnamed buffer, [New]");
    } else {
        sprintf(ceed->status, "'%s', [New]", arg);
    }
}

static void cmd_edit(editor* ceed, const char* arg) {
    if (ceed->buf->dirty) {
        sprintf(ceed->status, RED"No write since last change"RESET);
    } else {
        cmd_force_edit(ceed, arg);
    }
}

static void cmd_path(editor* ceed, const char* arg) {
    char* path = ceed->buf->path;
    if (*path == '\0'){
        sprintf(ceed->status, "unnamed buffer");
    } else {
        sprintf(ceed->status, "'%s'", ceed->buf->path);
    }
}

static void cmd_write(editor* ceed, const char* arg) {
    const char* path;

    if (*arg == '\0' && *ceed->buf->path == '\0') {
        sprintf(ceed->status, RED"No file name"RESET);
        return;
    } else if (*arg != '\0') {
        path = arg;
        snprintf(ceed->buf->path, MAX_BUF_PATH_LENGTH, "%s", path);
    } else {
        path = ceed->buf->path;
    }

    FILE* f = fopen(path, "w");
    if (f == NULL) {
        sprintf(ceed->status, RED"Cannot write into '%s'"RESET, path);
        return;
    }
  
    buf_fwrite(ceed->buf, f);
    fclose(f);
    size_t len = buf_len(ceed->buf);
    sprintf(ceed->status, "'%s', %d bytes written", path, len);
}

static ex_command commands[] = {
    { "q",         cmd_quit          },
    { "!q",        cmd_force_quit    },
    { "echo",      cmd_echo          },
    { "e",         cmd_edit          },
    { "!e",        cmd_force_edit    },
    { "path",      cmd_path          },
    { "w",         cmd_write         },
};

static const size_t cmd_count = sizeof(commands) / sizeof(commands[0]);

void run_command(editor* ceed, char* cmd) {
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
            commands[i].handler(ceed, arg);
            return;
        }
    }

    sprintf(ceed->status, RED"Not an editor command: %s"RESET, cmd);
}

#endif // COMMANDS_IMPLEMENTATION

