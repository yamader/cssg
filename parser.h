#ifndef CSSG_PARSER_H
#define CSSG_PARSER_H

#include "types.h"

/* common parsers ------------------------------------------- */

typedef struct exp exp;
struct exp {
  enum {
    EXP_SYM,
    EXP_CALL
  } type;
  struct {
    str sym;
    exp* args;
    size_t args_len, args_cap;
  } data;
};

/* template parsers ----------------------------------------- */

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
      exp src;
    } for_;
    exp exp;
  } data;
} tmpl_node;

typedef struct {
  tmpl_node* buf;
  size_t len, cap;
} tmpl_list;

tmpl_list parse_tmpl(const str s);

/* markdown parsers ----------------------------------------- */

typedef struct {
  enum {
    TMPL_HEAD,
    TMPL_CODE,
  } type;
  union {
    str text;
  } data;
} md_node;

typedef struct {
  md_node* buf;
  size_t len, cap;
} md_list;

md_list parse_md(const str s);

/* frontmatter parsers -------------------------------------- */

#endif
