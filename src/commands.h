#ifndef COMMANDS_H
#define COMMANDS_H

#include "editor.h"

typedef void (*command_handler)(editor *, const char *arg);

typedef struct {
  const char *name;

  command_handler handler;
} ex_command;

void cmd_check(editor *ceed, const char *arg);

// modifies cmd, make sure you strncpy beforehand
void run_command(editor *ceed, char *cmd);

#endif // COMMANDS_H
