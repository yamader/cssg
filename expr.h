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

void expr_args_init(expr* e);
void expr_args_push(expr* e, expr arg);
void free_expr(expr* e);

#endif
