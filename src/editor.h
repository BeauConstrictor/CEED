#ifndef EDITOR_H
#define EDITOR_H

#include "hole.h"
#include "constants.h"

typedef enum {
    normal,
    insert,
    command
} editor_mode;

typedef struct {
    buffer* buf;
    editor_mode mode;
    char status[STATUS_LENGTH];
} editor;

#endif
