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

/* markdownは行指向のフラットなノード列としてパースする:
   - 見出し/引用/リスト行/表の行と段落は、インライン要素ごとの
     同typeノードの並び + 終端ノードとして積まれる
   - 終端ノードは text == "" で、attrにマーカーを持つ
     (見出しの"##"(lenがレベル)、リストの"-"や"1."、引用の">"など)
   - コードブロック(textが中身、attrが言語名)、HTML、水平線(MD_LINE)は
     単一ノード
   evalは同typeのノードを終端ノードまで集めて1ブロックにすればよい */

DEFINE_OPT(opt_md_inline, md_inline)

md_node md_inline_node(int type, md_inline content) {
  md_node res;
  res.type = type;
  res.data.content = content;
  return res;
}

md_node md_text_node(int type, str text) {
  md_node res;
  res.type = type;
  res.data.content.type = MDI_TEXT;
  res.data.content.data.text = text;
  res.data.content.data.attr = to_str("");
  return res;
}

/* ブロックの終端ノード */
md_node md_end_node(int type, str marker) {
  md_node res = md_text_node(type, to_str(""));
  res.data.content.data.attr = marker;
  return res;
}

/* `コード` */
opt_md_inline pm_code(ctx* ctx) {
  size_t old = ctx->cur;
  md_inline res;
  opt_str body;

  if (!p_str(ctx, "`")) return null_opt_md_inline();
  body = p_until(ctx, "`");
  if (!body.ok) {
    ctx->cur = old;
    return null_opt_md_inline();
  }

  res.type = MDI_CODE;
  res.data.text = body.v;
  res.data.attr = to_str("");
  return to_opt_md_inline(res);
}

/* *強調*と**強い強調** */
opt_md_inline pm_emph(ctx* ctx) {
  size_t old = ctx->cur;
  bool_ strong = p_str(ctx, "**");
  md_inline res;
  opt_str body;

  if (!strong && !p_str(ctx, "*")) return null_opt_md_inline();
  body = p_until(ctx, strong ? "**" : "*");
  if (!body.ok || !body.v.len) { /* 空の強調は不可 */
    ctx->cur = old;
    return null_opt_md_inline();
  }

  res.type = strong ? MDI_STRONG : MDI_EMPH;
  res.data.text = body.v;
  res.data.attr = to_str("");
  return to_opt_md_inline(res);
}

/* [text](url)と![alt](url) */
opt_md_inline pm_link(ctx* ctx) {
  size_t old = ctx->cur;
  bool_ img = p_str(ctx, "!");
  md_inline res;
  opt_str text, url;

  if (!p_str(ctx, "[")) {
    ctx->cur = old;
    return null_opt_md_inline();
  }
  text = p_until(ctx, "]");
  if (!text.ok || !p_str(ctx, "(") || !(url = p_until(ctx, ")")).ok) {
    ctx->cur = old;
    return null_opt_md_inline();
  }

  res.type = img ? MDI_IMAGE : MDI_LINK;
  res.data.text = text.v;
  res.data.attr = url.v;
  return to_opt_md_inline(res);
}

/* インライン要素1つ */
opt_md_inline pm_inline(ctx* ctx) {
  switch (ctx_rest(ctx).buf[0]) {
    case '`':
      return pm_code(ctx);
    case '*':
      return pm_emph(ctx);
    case '!':
    case '[':
      return pm_link(ctx);
  }
  return null_opt_md_inline();
}

/* eofまでをインライン要素に分解してtypeのノードとして積む */
void pm_inlines(ctx* ctx, md_list* res, int type) {
  str all = ctx_rest(ctx);
  size_t base = ctx->cur, start = 0;

  while (!ctx_eof(ctx)) {
    size_t pos = ctx->cur - base;
    opt_md_inline inl = pm_inline(ctx);

    if (!inl.ok) { /* ただの文字 */
      ctx->cur++;
      continue;
    }
    if (start < pos)
      md_list_push(res, md_text_node(type, str_span(all, start, pos)));
    md_list_push(res, md_inline_node(type, inl.v));
    start = ctx->cur - base;
  }

  if (start < all.len)
    md_list_push(res, md_text_node(type, str_drop(all, start)));
}

/* 1行をインライン要素に分解してtypeのノードとして積む */
void pm_inline_line(ctx* g_ctx, md_list* res, int type) {
  opt_ctx line = p_line(g_ctx);
  ctx body;

  if (!line.ok) return;
  p_spc(&line.v);
  body = ctx_from(str_trimr(p_rest(&line.v)));
  pm_inlines(&body, res, type);
}

/* ```言語名 中身 ``` */
bool_ pm_codeblock(ctx* g_ctx, md_list* res) {
  opt_ctx line;
  opt_str body;
  md_node node;

  if (!p_str(g_ctx, "```")) return false;

  node.type = MD_CODE;
  node.data.content.type = MDI_CODE;
  node.data.content.data.attr = to_str("");

  if ((line = p_line(g_ctx)).ok) { /* 行の残りは言語名 */
    p_spc(&line.v);
    node.data.content.data.attr = str_trimr(p_rest(&line.v));
  }

  body = p_until(g_ctx, "```");
  node.data.content.data.text = str_trimr(body.ok ? body.v : p_rest(g_ctx));
  if (body.ok) p_line(g_ctx); /* 閉じ```の行の残りを捨てる */

  md_list_push(res, node);
  return true;
}

/* '<'で始まる行から空行までは生のHTML */
bool_ pm_html(ctx* ctx, md_list* res) {
  opt_str text;
  md_node node;

  if (!str_prefix(ctx_rest(ctx), "<")) return false;

  text = p_until(ctx, "\n\n");
  node.type = MD_HTML;
  node.data.html = str_trimr(text.ok ? text.v : p_rest(ctx));
  md_list_push(res, node);
  return true;
}

/* # 見出し */
bool_ pm_head(ctx* g_ctx, md_list* res) {
  str marker = ctx_rest(g_ctx);
  size_t level = 0;

  while (p_str(g_ctx, "#")) level++;
  if (!level) return false;

  pm_inline_line(g_ctx, res, MD_HEAD);
  md_list_push(res, md_end_node(MD_HEAD, str_take(marker, level)));
  return true;
}

/* > 引用 */
bool_ pm_quote(ctx* g_ctx, md_list* res) {
  if (!p_str(g_ctx, ">")) return false;

  pm_inline_line(g_ctx, res, MD_QUOTE);
  md_list_push(res, md_end_node(MD_QUOTE, to_str(">")));
  return true;
}

/* | 表の行(セルの分解はevalに任せる) */
bool_ pm_table(ctx* g_ctx, md_list* res) {
  if (!str_prefix(ctx_rest(g_ctx), "|")) return false;

  pm_inline_line(g_ctx, res, MD_TABLE);
  md_list_push(res, md_end_node(MD_TABLE, to_str("|")));
  return true;
}

/* -か*だけ(3つ以上)の行は水平線 */
bool_ pm_hr(ctx* g_ctx, md_list* res) {
  str rest = ctx_rest(g_ctx);
  size_t i, n = 0;
  char c = 0;

  for (i = 0; i < rest.len && rest.buf[i] != '\n'; i++) {
    if (rest.buf[i] == ' ') continue;
    if ((rest.buf[i] != '-' && rest.buf[i] != '*') || (c && rest.buf[i] != c))
      return false;
    c = rest.buf[i];
    n++;
  }
  if (n < 3) return false;

  p_line(g_ctx);
  md_list_push(res, md_end_node(MD_LINE, str_trimr(str_take(rest, i))));
  return true;
}

/* "- "か"1. "で始まる行はリストの項目 */
bool_ pm_item(ctx* g_ctx, md_list* res) {
  str rest = ctx_rest(g_ctx);
  size_t n = 0;

  if (rest.len >= 2 && (rest.buf[0] == '-' || rest.buf[0] == '*') &&
      rest.buf[1] == ' ') {
    n = 2;
  } else {
    while (n < rest.len && isdigit(rest.buf[n])) n++;
    if (!n || n + 1 >= rest.len || rest.buf[n] != '.' || rest.buf[n + 1] != ' ')
      return false;
    n += 2;
  }

  g_ctx->cur += n;
  pm_inline_line(g_ctx, res, MD_LIST);
  md_list_push(res, md_end_node(MD_LIST, str_trimr(str_take(rest, n))));
  return true;
}

/* 段落(空行まで) */
void pm_par(ctx* g_ctx, md_list* res) {
  opt_str text = p_until(g_ctx, "\n\n");
  ctx body = ctx_from(str_trimr(text.ok ? text.v : p_rest(g_ctx)));

  if (ctx_eof(&body)) return;
  pm_inlines(&body, res, MD_PAR);
  md_list_push(res, md_end_node(MD_PAR, to_str("")));
}

md_list pm_all(ctx* ctx) {
  md_list res = new_md_list();

  while (!ctx_eof(ctx)) {
    if (p_str(ctx, "\n")) continue; /* 空行 */
    if (pm_codeblock(ctx, &res)) continue;
    if (pm_html(ctx, &res)) continue;
    if (pm_head(ctx, &res)) continue;
    if (pm_quote(ctx, &res)) continue;
    if (pm_table(ctx, &res)) continue;
    if (pm_hr(ctx, &res)) continue; /* "- - -"はリストより水平線を優先 */
    if (pm_item(ctx, &res)) continue;
    pm_par(ctx, &res);
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
