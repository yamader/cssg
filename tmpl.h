#ifndef CSSG_TMPL_H
#define CSSG_TMPL_H

#include "expr.h"
#include "types.h"

typedef struct {
  enum {
    TMPL_TEXT,
    TMPL_EXP, /* {{ v }} */
    TMPL_FOR, /* {{ for e, i of a }} */
    TMPL_END  /* {{ end }} */
  } type;
  union {
    str text;
    struct {
      str elem, idx;
      bool_ has_idx;
      expr src;
    } for_;
    expr expr;
  } data;
} tmpl_node;

typedef struct {
  tmpl_node* buf;
  size_t len, cap;
} tmpl_list;

tmpl_list new_tmpl_list();
void free_tmpl_list(tmpl_list* list);
void tmpl_list_push(tmpl_list* list, tmpl_node node);

heap_str eval_tmpl(tmpl_list list);

#endif
