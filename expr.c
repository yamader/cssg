#include "expr.h"

#include <stdlib.h>

void expr_args_init(expr* e) {
  e->data.args_len = 0;
  e->data.args_cap = 2;
  e->data.args = malloc(sizeof(expr) * e->data.args_cap);
}

void expr_args_push(expr* e, expr arg) {
  if (e->data.args_len >= e->data.args_cap) {
    e->data.args_cap *= 2;
    e->data.args = realloc(e->data.args, sizeof(expr) * e->data.args_cap);
  }
  e->data.args[e->data.args_len++] = arg;
}

void free_expr(expr* e) {
  free(e->data.args);
}
