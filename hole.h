/*
 * hole.h - generic text buffer implementation for text editors;
 *          simple and lightweight, designed for embedded systems.
 *
 * Hole is licensed under the open source MIT license. For more
 * details see https://opensource.org/license/MIT. Copyright 2026,
 * Beau Constrictor.
 *
 * To build this, in one source file that includes this file do:
 *     #define HOLE_IMPLEMENTATION
 *
 * Hole is a component of CEDIT, a lightweight text editor, but is
 * perfectly capable of powering your own text editor. All the
 * documentation you will need is in this file.
 *
 */

#ifndef HOLE_H
#define HOLE_H

#define BLUE "\033[34m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#define MAX_BUF_PATH_LENGTH 80
#define LINE_NUM_WIDTH "5"

typedef struct {
    char* start; // first byte of buffer
    char* end;   // first byte after buffer
    char* gap;   // first byte of gap
    char* after; // first byte after gap

    unsigned int scroll; // lines from top
    char* path;  // string path of the buffer
    bool dirty;
} buffer;

buffer* create_buf(size_t size);
void free_buf(buffer* buf);

// marks the buffer as 'dirty'
void buf_insertc(buffer* buf, char ch);
void buf_inserts(buffer* buf, const char* str);
void buf_insertf(buffer* buf, FILE* f);
void buf_backspace(buffer* buf);

// returns '\0' if buffer is empty of cursor is at end of buffer
char char_under_cursor(const buffer* buf);

// returns the number of characters before the cursor
size_t cursor_index(const buffer* buf);
// returns the number of characters after cursor
size_t backward_cursor_index(const buffer* buf);
// returns the number of characters in the entire buffer
size_t buf_len(const buffer* buf);

// move cursor left until a newline is reached
void cursor_up(buffer* buf);
// move cursor right until a newline is reached
void cursor_down(buffer* buf);
void cursor_left(buffer* buf);
void cursor_right(buffer* buf);

// draw height lines of the text buffer, starting at the buffer's
// scroll, vi-style.
void print_buf(const buffer* buf, size_t height);

// marks the buffer as 'clean'
void buf_fwrite(buffer* buf, FILE* f);
// TODO: implement
// void buf_snprint(const buffer* buf, char* str, size_t n);

#endif

#ifdef HOLE_IMPLEMENTATION

// create a new empty text buffer that can hold size chars
buffer* create_buf(size_t size) {
    buffer* buf = malloc(sizeof(buffer));
    assert(buf != NULL);

    buf->start = malloc(size);
    assert(buf->start != NULL);
    buf->end   = buf->start + size;
    buf->gap   = buf->start;
    buf->after = buf->end;

    buf->scroll = 0;

    buf->path = malloc(MAX_BUF_PATH_LENGTH);
    assert(buf->path != NULL);
    *buf->path = '\0';

    buf->dirty = false;

    return buf;
}

// free a buffer from the heap
void free_buf(buffer* buf) {
    free(buf->start);
    free(buf->path);
    free(buf);
}

// insert one character at the start of the buffer
void buf_insertc(buffer* buf, char ch) {
    if (buf->gap >= buf->after) return;

    *buf->gap = ch;
    buf->gap++;

    buf->dirty = true;
}

void buf_inserts(buffer* buf, const char* str) {
    while (*str != 0) {
        buf_insertc(buf, *str);
        str++;
    }
}

void buf_backspace(buffer* buf) {
    if (buf->gap <= buf->start) return;
    buf->gap--;

    buf->dirty = true;
}

// move one character from before the gap to after it
void cursor_left(buffer* buf) {
    if (buf->gap <= buf->start) return;

    buf->gap--;
    char ch = *buf->gap;
    buf->after--;
    *buf->after = ch;
}

// move one character from after the gap to before it
void cursor_right(buffer* buf) {
    if (buf->after >= buf->end) return;

    char ch = *buf->after;
    buf->after++;
    *buf->gap = ch;
    buf->gap++;
}

char char_under_cursor(const buffer* buf) {
    char* ch = buf->after;

    if (ch < buf->start) return '\0';
    if (ch >= buf->end) return '\0';

    return *ch;
}

size_t cursor_index(const buffer* buf) {
    return buf->gap - buf->start;
}

size_t buf_len(const buffer* buf) {
    return (buf->end - buf->start) - (buf->after - buf->gap);
}

size_t backward_cursor_index(const buffer* buf) {
    return buf->end - buf->after;
}

void cursor_up(buffer* buf) {
    while (cursor_index(buf) > 0) {
        cursor_left(buf);
        char ch = char_under_cursor(buf);
        if (ch == '\n' || ch == '\0') break;
    } 
    cursor_left(buf);
}

void cursor_down(buffer* buf) {
    while (backward_cursor_index(buf) > 0) {
        cursor_right(buf);
        char ch = char_under_cursor(buf);
        if (ch == '\n' || ch == '\0') break;
    } 
}

// print the contents of a text buffer
void print_buf(const buffer* buf, size_t height) {
    const char* ch = buf->start;
    size_t line = 0;

    printf(YELLOW"%"LINE_NUM_WIDTH"d "RESET, line+1);

    while (ch < buf->end) {
        if (ch == buf->gap) {
            const char* cursor = buf->after;

            if (cursor == buf->end) {
                printf("\033[7m \033[0m");
                break;
            }

            if (*cursor == '\n') {
                printf("\033[7m \033[0m");
                line++;
                if (line >= height) break;
                printf(YELLOW"\n%"LINE_NUM_WIDTH"d "RESET, line+1);
            } else {
                printf("\033[7m%c\033[0m", *cursor);
            }

            ch = cursor + 1;
            continue;
        }

        if (*ch == '\n') {
            line++;
            if (line >= height) break;
            printf(YELLOW"\n%"LINE_NUM_WIDTH"d "RESET, line+1);
        } else {
            putchar(*ch);
        }

        ch++;
    }

    while (line < height - 1) {
        printf("\n"BLUE"~"RESET);
        line++;
    }
}

void buf_insertf(buffer* buf, FILE* f) {
    int ch;
    while ((ch = fgetc(f)) != EOF) {
        unsigned char byte = (unsigned char)ch;
        buf_insertc(buf, byte);
    }
}

void buf_fwrite(buffer* buf, FILE* f) {
    char* ch = buf->start;

    char last_ch = '\0';

    while (ch < buf->end) {
        if (ch == buf->gap) {
            ch = buf->after;
            if (ch == buf->end) break;
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

#endif // HOLE_IMPLEMENTATION
