#include "parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  str _input;
  size_t cur;
} ctx;

DEFINE_OPT(opt_ctx, ctx);

ctx ctx_from(str s) {
  ctx res;
  res._input = s;
  res.cur = 0;
  return res;
}

str ctx_rest(ctx* ctx) {
  str res;
  res.buf = ctx->_input.buf + ctx->cur;
  res.len = ctx->_input.len - ctx->cur;
  return res;
}

bool_ ctx_eof(ctx* ctx) {
  return ctx->cur >= ctx->_input.len;
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
    return true;
  }
  ctx->cur = old;
  return false;
}

bool_ p_tok_spc(ctx* ctx, const char* s) {
  if (p_tok(ctx, s)) {
    p_spc(ctx);
    return true;
  }
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

opt_ctx p_line(ctx* g_ctx) {
  if (!ctx_eof(g_ctx)) {
    opt_str line = p_until(g_ctx, "\n");
    return to_opt_ctx(ctx_from(line.ok ? line.v : p_rest(g_ctx)));
  }
  return null_opt_ctx();
}

/* common parsers ------------------------------------------- */

val p_val(ctx* ctx) {
  str rest = ctx_rest(ctx);
  val v;
  return v;
}

expr p_expr(ctx* ctx) {
  str rest = ctx_rest(ctx);
  expr e;

  /* todo: val */

  p_spc(ctx);
  e.data.sym = p_sym(ctx);
  if (p_tok_spc(ctx, "(")) {
    e.type = EXP_CALL;
    expr_args_init(&e);
    do expr_args_push(&e, p_expr(ctx));
    while (p_tok_spc(ctx, ","));
    p_tok_spc(ctx, ")");
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
    /* for sym [, sym] of expr */
    p_spc(ctx);
    res.data.for_.elem = p_sym(ctx);
    if ((res.data.for_.has_idx = p_tok_spc(ctx, ",")))
      res.data.for_.idx = p_sym(ctx);
    p_tok_spc(ctx, "of");
    res.data.for_.src = p_expr(ctx);
  } else if (str_same(head, "end")) {
    res.type = TMPL_END;
  } else {
    res.type = TMPL_EXP;
    ctx->cur = old; /* back */
    p_spc(ctx);
    res.data.expr = p_expr(ctx);
  }

  if (!p_until(ctx, "}}").ok) {
    ctx->cur = old;
    return null_opt_tmpl_node();
  }

  return to_opt_tmpl_node(res);
}

tmpl_node tmpl_text_node(str text) {
  tmpl_node res;
  res.type = TMPL_TEXT;
  res.data.text = text;
  return res;
}

tmpl_list pt_all(ctx* ctx) {
  tmpl_list res = new_tmpl_list();

  for (;;) {
    opt_str text;
    opt_tmpl_node escape;

    text = p_until(ctx, "{{");
    if (!text.ok) {
      tmpl_list_push(&res, tmpl_text_node(p_rest(ctx)));
      break;
    }
    tmpl_list_push(&res, tmpl_text_node(text.v));

    escape = pt_escape(ctx);
    if (!escape.ok) {
      /* todo: パニックすべき? */
      tmpl_list_push(&res, tmpl_text_node(to_str("{{")));
      tmpl_list_push(&res, tmpl_text_node(p_rest(ctx))); /* todo: 挙動の確認; EOFまで読んじゃってる? */
      break;
    }
    tmpl_list_push(&res, escape.v);
  }

  return res;
}

tmpl_list parse_tmpl(const str s) {
  ctx ctx = ctx_from(s);
  return pt_all(&ctx);
}

/* markdown parsers ----------------------------------------- */

md_list pm_all(ctx* ctx) {
  md_list res = new_md_list();

  for (;;) {
    md_node p = {MD_PAR, {MDI_TEXT, {p_rest(ctx), to_str("")}}};
    md_list_push(&res, p);
    break;
  }

  return res;
}

md_list parse_md(const str s) {
  ctx ctx = ctx_from(s);
  return pm_all(&ctx);
}

/* frontmatter parsers -------------------------------------- */

fm_list new_fm_list() {
  fm_list list;
  list.len = 0;
  list.cap = 4;
  list.buf = malloc(sizeof(fm_item) * list.cap);
  return list;
}

fm_item* fm_push(fm_list* d, str key, str val) {
  if (d->len >= d->cap) {
    d->cap *= 2;
    d->buf = realloc(d->buf, sizeof(fm_item) * d->cap);
  }
  d->buf[d->len].key = key;
  d->buf[d->len].val = val;
  return d->buf + d->len++;
}

page parse_fm(const str s) {
  ctx ctx = ctx_from(s);
  page page;
  page.fm = new_fm_list();

  if (p_tok(&ctx, "---")) {
    opt_ctx line_ctx = p_line(&ctx);

    /* 最初の---が無効なとき */
    if (!line_ctx.ok || !(p_spc(&line_ctx.v), ctx_eof(&line_ctx.v))) {
      ctx.cur = 0;
      goto content;
    }

    while ((line_ctx = p_line(&ctx)).ok) {
      if (p_tok_spc(&line_ctx.v, "---"))
        goto content;
      else {
        str key = p_sym(&line_ctx.v);
        if (!p_tok_spc(&line_ctx.v, ":")) continue; /* 無効な行は無視 */
        fm_push(&page.fm, key, str_trimr(p_rest(&line_ctx.v)));
      }
    }

    /* 閉じ---が無いとき */
    ctx.cur = 0;
    page.fm.len = page.fm.cap = 0;
    free(page.fm.buf);
  }

content:
  page.content = p_rest(&ctx);
  return page;
}
