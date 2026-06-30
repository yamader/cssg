#include "parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

typedef struct {
  str _input;
  size_t cur;
} ctx;

str ctx_rest(ctx* ctx) {
  str res;
  res.buf = ctx->_input.buf + ctx->cur;
  res.len = ctx->_input.len - ctx->cur;
  return res;
}

/*
#define unwrap_assign(T, y, x) \
  {                            \
    T _x = x;                  \
    if (_x.ok)                 \
      y = _x.v;                \
    else                       \
      goto abort;              \
  }
*/

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
  size_t i = 1;
  if (!(rest.len && (isalpha(rest.buf[0]) || rest.buf[0] == '_'))) return to_str("");
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
  if (p_str(ctx, s)) {
    p_spc(ctx);
    return true;
  }
  ctx->cur = old;
  return false;
}

opt_str p_until(ctx* ctx, const char* s) {
  str rest = ctx_rest(ctx);
  ptrdiff_t d = str_find(rest, s);
  if (d < 0) return null_opt_str();
  ctx->cur += d + strlen(s);
  return to_opt_str(str_take(rest, d));
}

str p_rest(ctx* ctx) {
  str rest = ctx_rest(ctx);
  ctx->cur += rest.len;
  return rest;
}

/* common parsers ------------------------------------------- */

val p_val(ctx* ctx) {
  str rest = ctx_rest(ctx);
  val v;
  return v;
}

void args_push(exp* e, exp arg) {
  if (e->data.args_len >= e->data.args_cap) {
    e->data.args_cap *= 2;
    e->data.args = realloc(e->data.args, sizeof(exp) * e->data.args_cap);
  }
  e->data.args[e->data.args_len++] = arg;
}

exp p_exp(ctx* ctx) {
  str rest = ctx_rest(ctx);
  exp e;

  /* todo: val */

  p_spc(ctx);
  e.data.sym = p_sym(ctx);
  if (p_tok(ctx, "(")) {
    e.type = EXP_CALL;
    e.data.args_len = 0;
    e.data.args_cap = 2;
    e.data.args = malloc(sizeof(exp) * e.data.args_cap);
    do args_push(&e, p_exp(ctx));
    while (p_tok(ctx, ","));
    p_tok(ctx, ")");
  } else {
    e.type = EXP_SYM;
  }

  return e;
}

/* template parsers ----------------------------------------- */

DEFINE_OPT(opt_tmpl_node, tmpl_node)

opt_tmpl_node pt_escape(ctx* ctx) {
  tmpl_node res;
  size_t old = ctx->cur;
  str head;

  p_spc(ctx);
  head = p_sym(ctx);

  if (str_same(head, "for")) {
    res.type = TMPL_FOR;
    /* for sym [, sym] of exp */
    p_spc(ctx);
    res.data.for_.elem = p_sym(ctx);
    if ((res.data.for_.has_idx = p_tok(ctx, ",")))
      res.data.for_.idx = p_sym(ctx);
    p_tok(ctx, "of");
    res.data.for_.src = p_exp(ctx);
  } else if (str_same(head, "end")) {
    res.type = TMPL_END;
  } else {
    res.type = TMPL_EXP;
    ctx->cur = old; /* back */
    p_spc(ctx);
    res.data.exp = p_exp(ctx);
  }

  if (!p_until(ctx, "}}").ok) {
    ctx->cur = old;
    return null_opt_tmpl_node();
  }

  return to_opt_tmpl_node(res);
}

void node_push(tmpl_list* list, tmpl_node node) {
  if (list->len >= list->cap) {
    list->cap *= 2;
    list->buf = realloc(list->buf, sizeof(tmpl_node) * list->cap);
  }
  list->buf[list->len++] = node;
}

tmpl_node text_node(str text) {
  tmpl_node res;
  res.type = TMPL_TEXT;
  res.data.text = text;
  return res;
}

tmpl_list pt_all(ctx* ctx) {
  tmpl_list res;
  res.len = 0;
  res.cap = 2;
  res.buf = malloc(sizeof(tmpl_node) * res.cap);

  for (;;) {
    opt_str text;
    opt_tmpl_node escape;

    text = p_until(ctx, "{{");
    if (!text.ok) {
      node_push(&res, text_node(p_rest(ctx)));
      break;
    }
    node_push(&res, text_node(text.v));

    escape = pt_escape(ctx);
    if (!escape.ok) {
      node_push(&res, text_node(to_str("{{")));
      node_push(&res, text_node(p_rest(ctx)));
      break;
    }
    node_push(&res, escape.v);
  }

  return res;
}

tmpl_list parse_tmpl(const str s) {
  ctx ctx;
  ctx._input = s;
  ctx.cur = 0;
  return pt_all(&ctx);
}

/* markdown parsers ----------------------------------------- */

/* frontmatter parsers -------------------------------------- */

