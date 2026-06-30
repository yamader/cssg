#ifndef CSSG_TEMPLATE_H
#define CSSG_TEMPLATE_H

#include "types.h"

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
