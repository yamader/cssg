#ifndef CSSG_EXPR_H
#define CSSG_EXPR_H

#include "types.h"

typedef struct expr expr;
struct expr {
  enum {
    EXP_SYM,
    EXP_CALL
  } type;
  struct {
    str sym;
    expr* args;
    size_t args_len, args_cap;
  } data;
};

#endif
