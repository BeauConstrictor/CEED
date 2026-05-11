#include "constants.h"
#include "hole.h"
#include "csrpc.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"

#define RPC_FUNC(name) static struct csrpc_resp rpc_##name(editor *ceed, \
    unsigned int argc, char **args)

#define ENSURE_ENOUGH_ARGS(count)                             \
  if (argc < count) {                                         \
      return (struct csrpc_resp){"missing argument(s)\n", 1}; \
  }

#define SUCCESS() return (struct csrpc_resp){"", 0}

#define ADD_FUNC(name) { #name, rpc_##name }

char *long_resp_buf = NULL;
char *install_dir;

RPC_FUNC(quit) {
  long code = argc == 1 ? 0 : strtol(args[1], NULL, 10);
  ceed->exit = code;
  SUCCESS();
}

RPC_FUNC(setv) {
  ENSURE_ENOUGH_ARGS(3);

  if (strcmp(args[1], "path") == 0) {
    snprintf(ceed->buf->path, ceed->buf->pathsize, "%s", args[2]);
  } else if (strcmp(args[1], "dirty") == 0) {
    if (strcmp(args[2], "yes") == 0)
      ceed->buf->dirty = true;
    else if (strcmp(args[2], "no") == 0)
      ceed->buf->dirty = false;
    else
      return (struct csrpc_resp){"value cannot be interpreted as a flag", 1};
  } else {
    char errmsg[128];
    snprintf(errmsg, sizeof(errmsg), "unknown var: '%s'\n", args[1]);
    return (struct csrpc_resp){errmsg, 1};
  }

  SUCCESS();
}

RPC_FUNC(getv) {
  ENSURE_ENOUGH_ARGS(2);

  if        (strcmp(args[1], "bytes") == 0) {
    static char slen[16];
    snprintf(slen, sizeof(slen), "%zu\n", buf_len(ceed->buf));
    return (struct csrpc_resp){slen, 0};
  } else if (strcmp(args[1], "path") == 0) {
    return (struct csrpc_resp){ceed->buf->path, 0};
  } else if (strcmp(args[1], "dirty") == 0) {
    return (struct csrpc_resp){ceed->buf->dirty ? "yes": "no", 0};
  } else if (strcmp(args[1], "buf") == 0) {
    size_t size = buf_len(ceed->buf) + 1;
    long_resp_buf = malloc(size);
    snprint_buf(long_resp_buf, size, ceed->buf);
    return (struct csrpc_resp){long_resp_buf, 0};
  } else {
    char errmsg[128];
    snprintf(errmsg, sizeof(errmsg), "unknown var: '%s'\n", args[1]);
    return (struct csrpc_resp){errmsg, 1};
 }
}

RPC_FUNC(enew) {
  free_buf(ceed->buf);
  ceed->buf = create_buf(INITIAL_BUFFER_SIZE, PATH_LENGTH);
  SUCCESS();
}

RPC_FUNC(insert) {
  ENSURE_ENOUGH_ARGS(2);
  buf_inserts(ceed->buf, args[1]);
  SUCCESS();
}

RPC_FUNC(finsert) {
  ENSURE_ENOUGH_ARGS(2);
  FILE *f = fopen(args[1], "r");
  if (!f)
    return (struct csrpc_resp){strerror(errno), 1};
  buf_insertf(ceed->buf, f);
  fclose(f);
  SUCCESS();
}

RPC_FUNC(write) {
  char path[PATH_LENGTH];
  snprintf(path, sizeof(path), "%s",
      argc == 1 ? ceed->buf->path : args[1]);
  if (strlen(path) == 0)
    return (struct csrpc_resp){"no file name", 1};

  FILE *f = fopen(path, "w");
  buf_fwrite(ceed->buf, f);
  fclose(f);

  SUCCESS();
}

RPC_FUNC(lcur) {
  cursor_left(ceed->buf);
  SUCCESS();
}

RPC_FUNC(rcur) {
  cursor_right(ceed->buf);
  SUCCESS();
}

RPC_FUNC(lcur_u) {
  ENSURE_ENOUGH_ARGS(2);
  cursor_left_until(ceed->buf, args[1]);
  SUCCESS();
}

RPC_FUNC(rcur_u) {
  ENSURE_ENOUGH_ARGS(2);
  cursor_right_until(ceed->buf, args[1]);
  SUCCESS();
}

RPC_FUNC(bind) {
  ENSURE_ENOUGH_ARGS(3);

  if (strlen(args[1]) > 1)
    return (struct csrpc_resp){"invalid keymap literal", 1};

  binding *bind = malloc(sizeof(binding));
  bind->key = args[1][0];
  bind->next = NULL;
  snprintf(bind->cmd, STATUS_LENGTH, "%s", args[2]);

  if (ceed->bindings) bind->next = ceed->bindings;
  ceed->bindings = bind;

  SUCCESS();
}

typedef struct csrpc_resp (*rpc_cmd_handler)(editor *ceed,
  unsigned int argc, char **args);

struct rpc_cmd {
  char *name;
  rpc_cmd_handler handler;
};

static struct rpc_cmd cmds[] = {
  ADD_FUNC(quit),
  ADD_FUNC(setv), ADD_FUNC(getv),
  ADD_FUNC(enew),
  ADD_FUNC(insert), ADD_FUNC(finsert),
  ADD_FUNC(lcur), ADD_FUNC(rcur),
  ADD_FUNC(lcur_u), ADD_FUNC(rcur_u),
  ADD_FUNC(write),
  ADD_FUNC(bind),
};

static const size_t cmd_count = sizeof(cmds) / sizeof(struct rpc_cmd);

static void free_long_resp() {
  if (long_resp_buf) {
    free(long_resp_buf);
    long_resp_buf = NULL;
  }
}

static struct csrpc_resp handle_rpc_call(struct csrpc_call *call, void *ceed) {
  free_long_resp();

  ceed = (editor*)ceed;
  unsigned int argc = call->argc;
  char **args = call->args;
  char *cmd = call->args[0];

  for (unsigned int i = 0; i < cmd_count; i++) {
    if (strcmp(cmd, cmds[i].name) == 0) {
      return cmds[i].handler(ceed, argc, args);
    }
  }

  return (struct csrpc_resp){"csrpc: command not found\n", 1};
}

void run_command(editor *ceed, char *cmd) {
  ceed->status[0] = '\0';

  char ceedsh[1024];
  snprintf(ceedsh, sizeof(ceedsh), "%s/cfglib/ceed.sh", install_dir);

  FILE* f = csrpc_run(cmd, ceedsh, handle_rpc_call, ceed);

  free_long_resp();

  size_t n = fread(ceed->status, 1, STATUS_LENGTH-1, f);
  ceed->status[n] = '\0';
  fclose(f);

  char *s = ceed->status;
  while (*s) {
    if (*s == '\n') *s = ' ';
    s++;
  }
}

void init_commands() {
  install_dir = getenv("CEED_INSTALL");

  if (!install_dir) {
    fprintf(stderr, "ceed: CEED_INSTALL is not set\n"
      "Make sure to add this to your system's '~/.bashrc' equivalent:\n"
      "\texport CEED_INSTALL=~/.local/share/ceed/\n");
    exit(1);
  }

  char new_path[4096];
  char *orig_path = getenv("PATH");
  snprintf(new_path, sizeof(new_path),
      "%s/cfglib:%s", install_dir, orig_path);
  setenv("PATH", new_path, 1);

  setenv("IMPORT_CEED", ". \"$(command -v ceed.sh)\"", 1);
}
