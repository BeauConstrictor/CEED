#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "hole.h"

// create a new empty text buffer that can hold size chars
buffer *create_buf(size_t size, size_t pathlen) {
  buffer *buf = malloc(sizeof(buffer));
  assert(buf != NULL);

  buf->start = malloc(size);
  assert(buf->start != NULL);
  buf->end = buf->start + size;
  buf->gap = buf->start;
  buf->after = buf->end;

  buf->scroll = 0;

  buf->pathsize = pathlen;
  buf->path = malloc(pathlen);
  assert(buf->path != NULL);
  *buf->path = '\0';

  buf->dirty = false;

  return buf;
}

buffer *expand_buf(buffer *buf, size_t new_size) {
  assert(new_size > buf_len(buf));

  buffer *new_buf = create_buf(new_size, 0);

  size_t cu_idx = cursor_index(buf);
  size_t bc_idx = backward_cursor_index(buf);

  memcpy(new_buf->start, buf->start, cu_idx);
  memcpy(new_buf->end - bc_idx, buf->after, bc_idx);

  new_buf->gap = new_buf->start + cu_idx;
  new_buf->after = new_buf->end - bc_idx;

  new_buf->scroll = buf->scroll;
  free(new_buf->path); // we use the old buf's path
  new_buf->path = buf->path;
  new_buf->dirty = buf->dirty;

  free(buf->start);
  free(buf);

  return new_buf;
}

// free a buffer from the heap
void free_buf(buffer *buf) {
  free(buf->start);
  free(buf->path);
  free(buf);
}

// insert one character at the start of the buffer
void buf_insertc(buffer *buf, char ch) {
  if (buf->gap >= buf->after)
    return;

  *buf->gap = ch;
  buf->gap++;

  buf->dirty = true;
}

void buf_inserts(buffer *buf, const char *str) {
  while (*str != 0) {
    buf_insertc(buf, *str);
    str++;
  }
}

void buf_backspace(buffer *buf) {
  if (buf->gap <= buf->start)
    return;
  buf->gap--;

  buf->dirty = true;
}

// move one character from before the gap to after it
void cursor_left(buffer *buf) {
  if (buf->gap <= buf->start)
    return;

  buf->gap--;
  char ch = *buf->gap;
  buf->after--;
  *buf->after = ch;
}

// move one character from after the gap to before it
void cursor_right(buffer *buf) {
  if (buf->after >= buf->end)
    return;

  char ch = *buf->after;
  buf->after++;
  *buf->gap = ch;
  buf->gap++;
}

void n_cursor_left(buffer *buf, unsigned int n) {
  while (n) {
    cursor_left(buf);
    n--;
  }
}

void n_cursor_right(buffer *buf, unsigned int n) {
  while (n) {
    cursor_right(buf);
    n--;
  }
}

char char_under_cursor(const buffer *buf) {
  char *ch = buf->after;

  if (ch < buf->start)
    return '\0';
  if (ch >= buf->end)
    return '\0';

  return *ch;
}

size_t cursor_index(const buffer *buf) { return buf->gap - buf->start; }

size_t buf_len(const buffer *buf) {
  return (buf->end - buf->start) - (buf->after - buf->gap);
}

size_t buf_size(const buffer *buf) { return buf->end - buf->start; }

size_t backward_cursor_index(const buffer *buf) {
  return buf->end - buf->after;
}

typedef void (*buffer_void_fn)(buffer *);
typedef size_t (*buffer_sizet_fn)(const buffer *);

static void cursor_move_until(buffer *buf, const char *until,
                              buffer_void_fn move_cur,
                              buffer_sizet_fn cur_idx) {
  while (cur_idx(buf) > 0) {
    move_cur(buf);
    char ch = char_under_cursor(buf);

    if (strchr(until, ch) != NULL) {
      break;
    }
  }
}

void cursor_left_until(buffer *buf, const char *until) {
  cursor_move_until(buf, until, cursor_left, cursor_index);
}
void cursor_right_until(buffer *buf, const char *until) {
  cursor_move_until(buf, until, cursor_right, backward_cursor_index);
}

// print the contents of a text buffer, vi-style
void print_buf(buffer *buf, int height, int highlight_col,
               const char *eof_lines, const char *linenums,
               bool save_cursor, const char *tab) {
#define draw_highlight_col()                                                   \
  while (col < highlight_col) {                                                \
    col++;                                                                     \
    printf(" ");                                                               \
  }                                                                            \
  if (col == highlight_col)                                                    \
    printf("\033[100m \033[0m");

#define pb_next_line()                                                         \
  draw_highlight_col() col = 0;                                                \
  line++;                                                                      \
  if (line >= height && height > 0)                                            \
    break;                                                                     \
  printf("\n");                                                                \
  if (linenums)                                                                \
    printf(linenums, scroll + line + 1);

#define print_ch(ch)                                                           \
  if (col == highlight_col)                                                    \
    printf("\033[90m\033[7m");                                                 \
  putchar(ch);                                                                 \
  if (col == highlight_col)                                                    \
    printf("\033[0m");                                                         \
  col++;

  int scroll = buf->scroll;

  const char *ch = buf->start;
  int above_scroll = scroll;
  int line = 0;
  int col = 0;

  if (tab == NULL)
    tab = "    ";

  printf(linenums, scroll + line + 1);

  while (ch < buf->end) {
    if (above_scroll) {
      if (ch == buf->gap)
        ch = buf->after + 1;
      if (*ch == '\n') {
        above_scroll--;
      }
      ch++;
      continue;
    }

    if (ch == buf->gap) {
      if (save_cursor)
        printf("\033[s");
      ch = buf->after;
      continue;
    }

    if (*ch == '\n') {
      pb_next_line();
    } else if (*ch == '\t') {
      int tablen = strlen(tab);
      for (int i = 0; i < tablen; i++) {
        print_ch(tab[i]);
      }
    } else {
      print_ch(*ch);
    }

    ch++;
  }

  draw_highlight_col();

  while (line < height - 1 && height > 0) {
    printf("%s", eof_lines);
    line++;
  }
}

void snprint_buf(char *str, size_t size, buffer *buf) {
  char *s = buf->start;
  size_t i = 0;
  while (i < size-1) {
    if (s == buf->gap) s = buf->after;
    if (s == buf->end) break;
    str[i] = *s;
    s++;
    i++;
  }
  str[i] = '\0';
}

void scroll_buf(buffer *buf, int l) {
  while (l != 0) {
    if (l > 0) {
      cursor_right_until(buf, "\n");
      buf->scroll += 1;
      l--;
    } else if (l < 0) {
      if (buf->scroll == 0)
        break;
      cursor_left_until(buf, "\n");
      buf->scroll -= 1;
      l++;
    }
  }
}

void buf_insertf(buffer *buf, FILE *f) {
  int ch;
  while ((ch = fgetc(f)) != EOF) {
    unsigned char byte = (unsigned char)ch;
    buf_insertc(buf, byte);
  }
}

void buf_fwrite(buffer *buf, FILE *f) {
  char *ch = buf->start;

  char last_ch = '\0';

  while (ch < buf->end) {
    if (ch == buf->gap) {
      ch = buf->after;
      if (ch == buf->end)
        break;
    }
    fputc(*ch, f);
    last_ch = *ch;
    ch++;
  }

  ch--;
  if (last_ch != '\n') {
    fputc('\n', f);
  }

  buf->dirty = false;
}
