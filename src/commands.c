#include "constants.h"
#include "hole.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"

void cmd_force_quit(editor *ceed, const char *arg) { exit(0); }

void cmd_quit(editor *ceed, const char *arg) {
  if (ceed->buf->dirty) {
    sprintf(ceed->status, RED "No write since last change" RESET);
  } else {
    cmd_force_quit(ceed, arg);
  }
}

void cmd_echo(editor *ceed, const char *arg) {
  sprintf(ceed->status, "%s", arg);
}

void cmd_force_edit(editor *ceed, const char *arg) {
  FILE *f = fopen(arg, "r");

  long size = 128;
  if (f) {
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
  }

  free_buf(ceed->buf);
  ceed->buf = create_buf(size + 64);
  sprintf(ceed->buf->path, "%s", arg);

  if (f != NULL) {
    buf_insertf(ceed->buf, f);
    fclose(f);
    size_t len = buf_len(ceed->buf);
    sprintf(ceed->status, "'%s', %d bytes", arg, len);
    while (len) {
      cursor_left(ceed->buf);
      len--;
    }
    cursor_right_until(ceed->buf, "\n");
    cursor_right_until(ceed->buf, "\n");
  } else if (*arg == '\0') {
    sprintf(ceed->status, "unnamed buffer, [new]");
  } else {
    sprintf(ceed->status, "'%s', [new]", arg);
  }

  ceed->buf->dirty = false;
}

void cmd_edit(editor *ceed, const char *arg) {
  if (ceed->buf->dirty)
    sprintf(ceed->status, RED "No write since last change" RESET);
  else
    cmd_force_edit(ceed, arg);
}

void cmd_bind(editor *ceed, const char *arg) {
    if (strlen(arg) < 3 || *(arg+1) != ' ') {
        sprintf(ceed->status, RED "Bad argument" RESET);
        return;
    }

    binding *bind = malloc(sizeof(binding));
    bind->next = NULL;
    bind->key = *arg;
    strcpy(bind->cmd, arg+2);
    
    if (ceed->bindings == NULL) {
        ceed->bindings = bind;
        return;
    }

    binding *last_bind = ceed->bindings;
    while (1) {
        if (last_bind->next)
            last_bind = last_bind->next;
        else
            break;
    }
    last_bind->next = bind;
}

void cmd_check(editor *ceed, const char *arg) {
  char *path = ceed->buf->path;
  if (*path == '\0') {
    sprintf(ceed->status, "unnamed buffer");
  } else {
    sprintf(ceed->status, "'%s'", ceed->buf->path);
  }

  if (ceed->buf->dirty)
    strcat(ceed->status, " [modified]");

  size_t status_len = strlen(ceed->status);
  size_t bytes = buf_len(ceed->buf);
  sprintf(ceed->status + status_len, ", %d bytes", bytes);
}

void cmd_write(editor *ceed, const char *arg) {
  const char *path;

  if (*arg == '\0' && *ceed->buf->path == '\0') {
    sprintf(ceed->status, RED "No file name" RESET);
    return;
  } else if (*arg != '\0') {
    path = arg;
    snprintf(ceed->buf->path, MAX_BUF_PATH_LENGTH, "%s", path);
  } else {
    path = ceed->buf->path;
  }

  FILE *f = fopen(path, "w");
  if (!f) {
    char *err = strerror(errno);
    snprintf(ceed->status, STATUS_LENGTH, RED "%s" RESET, err);
    return;
  }

  buf_fwrite(ceed->buf, f);
  fclose(f);
  size_t len = buf_len(ceed->buf);
  sprintf(ceed->status, "'%s', %d bytes written", path, len);
}

void cmd_discard(editor *ceed, const char *arg) { ceed->buf->dirty = false; }

void cmd_writequit(editor *ceed, const char *arg) {
  cmd_write(ceed, arg);
  cmd_quit(ceed, arg); // ignores the argument, so this is fine
}

void cmd_version(editor *ceed, const char *arg) {
  sprintf(ceed->status, GREETING);
}

static ex_command commands[] = {
    { "q",       cmd_quit },  { "q!",   cmd_force_quit },
    { "w",       cmd_write }, { "wq",   cmd_writequit },
    { "e",       cmd_edit },  { "e!",   cmd_force_edit },
    { "echo",    cmd_echo },
    { "bind",    cmd_bind },
    { "check",   cmd_check },
    { "discard", cmd_discard }, 
    { "version", cmd_version },
};

static const size_t cmd_count = sizeof(commands) / sizeof(commands[0]);

void run_command(editor *ceed, char *cmd) {
  if (strlen(cmd) == 0)
    return;

  char *arg = cmd;
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

  sprintf(ceed->status, RED "Not an editor command: %s" RESET, cmd);
}
