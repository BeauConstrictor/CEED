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
// * add multi-character key combos in normal mode, such as gg to go
//   to start of buffer.

#include "commands.h"
#include "editor.h"
#include "hole.h"

#include "constants.h"

#include <sys/ioctl.h>
#include <termios.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

#define word_BREAK "\n\t !\"#$%&'()*+,-./:;<=>?@[\\]^`{|}~"
#define WORD_BREAK "\n\t "

static const char *HELP_MSG =
    "Usage: ceed [FILE]\n"
    "\n"
    "Options:\n"
    "   -h, --help     Show this help message\n"
    "   -v, --version  Print version number\n"
    "\n"
    "If FILE is not found, it will be created on save.\n";

struct termios oldt, newt;
bool say_exit_with_q = false;

void draw_editor(editor *ceed) {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  printf("\033[H\033[2J\033[?25l");

  print_buf(ceed->buf, w.ws_row - 1, 70,
      "\n" GREY "~" RESET, GREY "%5d " RESET,
      true, "    ");

  if (say_exit_with_q) {
    say_exit_with_q = false;
    if (ceed->mode == normal)
      sprintf(ceed->status, "Exit with ':q'");
  }
  printf("\n%s", ceed->status);

  printf("\033[?25h");
  if (ceed->mode != command) 
    printf("\033[u");
  printf("\033[%d q", ceed->cursor_shape);

  fflush(stdout);
}

void chmode(editor *ceed, editor_mode mode) {
  switch (mode) {
    case normal:
      ceed->mode = normal;
      run_command(ceed, "stat");
      ceed->cursor_shape = 0;
     break;

     case insert:
      ceed->mode = insert;
      sprintf(ceed->status, BOLD YELLOW "-- INSERT --" RESET);
      ceed->cursor_shape = 6;
      break;

    case command:
      ceed->mode = command;
      sprintf(ceed->status, ":");
      ceed->cursor_shape = 4;
      break;
  }
}

char *resolve_binding(binding *bind, char key) {
  char *cmd = NULL;

  while (bind) {
    if (key == bind->key) {
      cmd = strdup(bind->cmd);
      break;
    } else {
      bind = bind->next;
    }
  }

  return cmd;
}

void handle_normal_mode_key(editor *ceed, char key) {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  switch (key) {
  case 'h':
    cursor_left(ceed->buf);
    break;
  case 'l':
    cursor_right(ceed->buf);
    break;

  case 'j':
    cursor_right_until(ceed->buf, "\n");
    break;
  case 'k':
    cursor_left_until(ceed->buf, "\n");
    break;

  case 'w':
    cursor_right_until(ceed->buf, word_BREAK);
    break;
  case 'b':
    cursor_left_until(ceed->buf, word_BREAK);
    break;
  case 'W':
    cursor_right_until(ceed->buf, WORD_BREAK);
    break;
  case 'B':
    cursor_left_until(ceed->buf, WORD_BREAK);
    break;

  case 'x':
    cursor_right(ceed->buf);
    buf_backspace(ceed->buf);
    break;

  case 'o':
    if (char_under_cursor(ceed->buf) != '\n')
      cursor_right_until(ceed->buf, "\n");
    buf_insertc(ceed->buf, '\n');
    chmode(ceed, insert);
    break;
  case 'O':
    cursor_left_until(ceed->buf, "\n");
    buf_insertc(ceed->buf, '\n');
    chmode(ceed, insert);
    break;

  case 'g':
    cursor_left_until(ceed->buf, "\0");
    break;
  case 'G':
    cursor_right_until(ceed->buf, "\0");
    break;

  case 'd':
    scroll_buf(ceed->buf, w.ws_row/2);
    break;
  case 'D':
    scroll_buf(ceed->buf, 1);
    break;
  case 'u':
    scroll_buf(ceed->buf, -w.ws_row/2);
    break;
  case 'U':
    scroll_buf(ceed->buf, -1);
    break;

  case 'a':
    cursor_right(ceed->buf);
    /* fall through */
  case 'i':
    chmode(ceed, insert);
    break;

  case ':':
    chmode(ceed, command);
    break;

  default:
    char *cmd = resolve_binding(ceed->bindings, key);
    if (!cmd) break;
    snprintf(ceed->status, STATUS_LENGTH, ":%s", cmd);
    run_command(ceed, cmd);
    free(cmd);
    break;
  }
}

void handle_insert_mode_key(editor *ceed, char key) {
  switch (key) {
  case '\b':
  case '\177':
    buf_backspace(ceed->buf);
    break;

  case '\033':
    chmode(ceed, normal);
    break;

  default:
    buf_insertc(ceed->buf, key);
  }
}

void handle_command_mode_key(editor *ceed, char key) {
  size_t cmd_len = strlen(ceed->status) - 1;

  switch (key) {
    case '\n':
      char cmd[STATUS_LENGTH];
      strcpy(cmd, ceed->status + 1);
      chmode(ceed, normal);
      run_command(ceed, cmd);
      break;

    case '\b':
    case '\177': {
      if (cmd_len < 1)
        break;
      ceed->status[cmd_len] = '\0';
      break;
    }

    case '\033':
      chmode(ceed, normal);
      break;

    default: {
      if (strlen(ceed->status) >= STATUS_LENGTH)
        break;
      ceed->status[cmd_len + 1] = key;
      ceed->status[cmd_len + 2] = '\0';
    }
  }
}

void handle_key(editor *ceed, char key) {
  switch (ceed->mode) {
  case normal:
    handle_normal_mode_key(ceed, key);
    break;

  case insert:
    handle_insert_mode_key(ceed, key);
    break;

  case command:
    handle_command_mode_key(ceed, key);
    break;
  }
}

void set_exit_with_q(int) {
  say_exit_with_q = true;
}

void initialise_terminal() {
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  printf("\033[?1049h");
  printf("\033[2 q");

  signal(SIGINT, set_exit_with_q);
}

void cleanup_terminal() {
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

  printf("\033[?1049l");
  printf("\033[2 q");
}

void init_editor(editor *ceed) {
  ceed->exit = -1;
  ceed->buf = create_buf(INITIAL_BUFFER_SIZE, PATH_LENGTH);
  chmode(ceed, normal);
}

int repl(editor *ceed) {
  char cmd[1024];
  printf("%s\n: ", ceed->status);
  while (fgets(cmd, sizeof(cmd), stdin)) {
    if (strcmp(cmd, "q\n") == 0) break;
    run_command(ceed, cmd);
    printf("%s\n: ", ceed->status);
  }
  return 0;
}

void process_args(editor *ceed, int argc, char *argv[]) {
  if (argc > 2) {
    fprintf(stderr, "ceed: too many arguments\n");
    fprintf(stderr, "Try 'ceed --help' for more information.\n");
    exit(1);
  } else if (argc == 2 &&
             (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
    printf(HELP_MSG);
    exit(0);
  } else if (argc == 2 && strcmp(argv[1], "--version") == 0) {
    printf("ceed (C Embedded EDitor) " CEED_VERSION "\n");
    exit(0);
  } else if (argc == 2 && strcmp(argv[1], "--repl") == 0) {
    exit(repl(ceed));
  } else if (argc == 2) {
    char cmd[STATUS_LENGTH];
    snprintf(cmd, STATUS_LENGTH, "edit \"%s\"", argv[1]);
    run_command(ceed, cmd);
  }
}

void run_config_script(editor *ceed) {
  char cmd[128];
  snprintf(cmd, sizeof(cmd), ". \"%s\"", CONFIG_PATH);
  run_command(ceed, cmd);
}

int main(int argc, char *argv[]) {
  init_commands();

  editor ceed = {0};
  init_editor(&ceed);

  process_args(&ceed, argc, argv);

  run_config_script(&ceed);
  if (strlen(ceed.status) == 0)
    snprintf(ceed.status, sizeof(ceed.status), GREETING);

  initialise_terminal();
  atexit(cleanup_terminal);

  while (true) {
    if (ceed.exit >= 0)
      exit(ceed.exit);

    draw_editor(&ceed);

    char key = getchar();
    handle_key(&ceed, key);

    if (buf_size(ceed.buf) - buf_len(ceed.buf) < 2) {
      ceed.buf = expand_buf(ceed.buf, buf_size(ceed.buf) * 2);
    }
  }

  return 0;
}
