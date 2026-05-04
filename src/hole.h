/*
 * hole.h - generic text buffer implementation for text editors;
 *          simple and lightweight, designed for embedded systems.
 *
 * Hole is licensed under the open source MIT license. For more
 * details see https://opensource.org/license/MIT. Copyright 2026,
 * Beau Constrictor.
 *
 * Hole is a component of CEED, a lightweight text editor, but is
 * perfectly capable of powering your own text editor. All the
 * documentation you will need is in this file.
 *
 */

#ifndef HOLE_H
#define HOLE_H

#include <stddef.h>
#include <stdio.h>

typedef struct {
  char *start; // first byte of buffer
  char *end;   // first byte after buffer
  char *gap;   // first byte of gap
  char *after; // first byte after gap

  int scroll; // lines from top
  char *path; // string path of the buffer
  bool dirty;
} buffer;

// maximum length of text in buffer, and of the buffer's path (+\0)
buffer *create_buf(size_t size, size_t pathlen);
// most insert operations will fail silently if the buffer does not
// have enough space. to prevent this, ensure you check free buffer
// space (buf_size() - buf_len()) before inserting.
buffer *expand_buf(buffer *buf, size_t new_size);
void free_buf(buffer *buf);

// marks the buffer as 'dirty'
void buf_insertc(buffer *buf, char ch);
void buf_inserts(buffer *buf, const char *str);
void buf_insertf(buffer *buf, FILE *f);
void buf_backspace(buffer *buf);

// returns '\0' if buffer is empty of cursor is at end of buffer
char char_under_cursor(const buffer *buf);

// returns the number of characters before the cursor
size_t cursor_index(const buffer *buf);
// returns the number of characters after cursor
size_t backward_cursor_index(const buffer *buf);
// returns the number of characters in the entire buffer
size_t buf_len(const buffer *buf);
// maximum number of characters the buffer can hold
size_t buf_size(const buffer *buf);

void cursor_left(buffer *buf);
void n_cursor_left(buffer *buf, unsigned int n);
void cursor_right(buffer *buf);
void n_cursor_right(buffer *buf, unsigned int n);

// use these to move the cursor up/down, by word, etc.
// they move until a any character in the string until is found
void cursor_right_until(buffer *buf, const char *until);
void cursor_left_until(buffer *buf, const char *until);

// draw height lines of the text buffer, starting at the buffer's
// scroll, vi-style.
// * highlightcol can be used for a line at 80 cols
// * eof_lines is used after end of file to fill to height, like the
//   tildes (~) in vi
// * linenums is a printf format string where %d is replaced with the
//   line number.
// * eof_lines and linenums should usually begin with \n, or be empty.
// * height is the max height to draw in the terminal. set to -1 to
//   draw entire buffer. if using height, you can set the buffer's
//   scroll to offset from the first line.
// * if save_cursors is set, then you can printf("\0338"); to move
//   the terminal cursor to the cursor's location in the text buffer
//   after drawing
void print_buf(buffer *buf, int height, int highlightcol,
               const char *eof_lines, const char *linenums,
               bool save_cursor);

// move scroll and cursor by l lines
void scroll_buf(buffer *buf, int l);

// marks the buffer as 'clean'
void buf_fwrite(buffer *buf, FILE *f);
// TODO: implement
// void buf_snprint(const buffer* buf, char* str, size_t n);

#endif
