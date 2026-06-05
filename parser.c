#include "parser.h"

#include <ctype.h>
#include <string.h>

typedef struct {
  str str;
  size_t n;
} str_n;

typedef struct {
  const str _input;
  size_t cur;
} ctx;

str ctx_rest(ctx* ctx) {
  str res;
  res.buf = ctx->_input.buf + ctx->cur;
  res.len = ctx->_input.len - ctx->cur;
  return res;
}

/* basic parsers -------------------------------------------- */

str p_spc(ctx* ctx) {
  str rest = ctx_rest(ctx);
  size_t i = 0;
  while (i < rest.len && isspace(rest.buf[i])) i++;
  ctx->cur += i;
  return str_take(rest, i);
}

str p_sym(ctx* ctx) {
  str rest = ctx_rest(ctx);
  size_t i = 0;
  while (i < rest.len && (isalnum(rest.buf[i]) || rest.buf[i] == '_')) i++;
  ctx->cur += i;
  return str_take(rest, i);
}

bool_ p_str(ctx* ctx, const char* s) {
  str rest = ctx_rest(ctx);
  if (str_prefix(rest, s)) {
    ctx->cur += strlen(s);
    return true;
  }
  return false;
}

bool_ p_tok(ctx* ctx, const char* s) {
  size_t old = ctx->cur;
  p_spc(ctx);
  if (p_str(ctx, s)) return true;
  ctx->cur = old;
  return false;
}

size_t p_until(ctx* ctx, const char* s) {
  str rest = ctx_rest(ctx);
  ptrdiff_t d = str_find(rest, s) + strlen(s);
  if (d >= 0) ctx->cur += d;
  return d;
}

/* common parsers ------------------------------------------- */

struct val;

typedef struct {
  a
} list;

typedef struct {
  enum {
    EXP_VAL,
    EXP_CALL
  } type;
} exp;

exp p_exp(ctx* ctx) {
  exp res;
  /*val val = p_val(ctx);*/
  return res;
}

/* template parsers ----------------------------------------- */

typedef struct {
  enum {
    PT_TEXT,
    PT_IF,  /* {{ if p }} */
    PT_FOR, /* {{ for e, i of a }} */
    PT_END, /* {{ end }} */
    PT_EXP  /* {{ v }} */
  } type;
  union {
    str str;
    exp exp;
    struct {
      str elem, idx;
      bool_ has_idx;
      exp src;
    } for_;
  } data;
} pt_node;

pt_node pt_escape(ctx* ctx) {
  pt_node res;
  size_t cur_old, cur_head;
  str head;

  cur_old = ctx->cur;
  p_spc(ctx);
  cur_head = ctx->cur;
  head = p_sym(ctx);
  p_spc(ctx);

  if (str_same(head, "for")) {
    res.type = PT_FOR;
    res.data.for_.elem = p_sym(ctx);
    if ((res.data.for_.has_idx = p_tok(ctx, ",")))
      res.data.for_.idx = p_sym(ctx);
    p_tok(ctx, "of");
    res.data.for_.src = p_exp(ctx);
  } else if (str_same(head, "end")) {
    res.type = PT_END;
  } else {
    res.type = PT_EXP;
    ctx->cur = cur_head; /* back */
    res.data.exp = p_exp(ctx);
  }

  p_until(ctx, "}}");

  return res;
}

/* markdown parsers ----------------------------------------- */
