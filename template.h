#ifndef TEMPLATE_H
#define TEMPLATE_H

#include "utils.h"

typedef struct {
  str src;
  str el;
} tmpl_for;

typedef struct {
  enum {
    TMPL_TEXT,
    TMPL_EXPR
  } type;
  union {
    str str;
    tmpl_for for_;
  } data;
} tmpl_node;

typedef struct {
  tmpl_node* nodes;
} tmpl;

#endif
