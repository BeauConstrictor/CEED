#ifndef EDITOR_H
#define EDITOR_H

#include "constants.h"
#include "hole.h"

typedef enum { normal, insert, command } editor_mode;

typedef struct binding binding;
struct binding {
  char cmd[STATUS_LENGTH];
  char key;
  binding *next;
};

typedef struct {
  buffer *buf;
  editor_mode mode;
  char status[STATUS_LENGTH];
  int cursor_shape;
  binding *bindings;
} editor;

#endif
