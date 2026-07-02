#include "md.h"

#include <ctype.h>
#include <stdlib.h>

md_list new_md_list() {
  md_list list;
  list.len = 0;
  list.cap = 2;
  list.buf = malloc(sizeof(md_node) * list.cap);
  return list;
}

void free_md_list(md_list* list) {
  free(list->buf);
}

void md_list_push(md_list* list, md_node node) {
  if (list->len >= list->cap) {
    list->cap *= 2;
    list->buf = realloc(list->buf, sizeof(md_node) * list->cap);
  }
  list->buf[list->len++] = node;
}

/* 左側の空白を落とす(str_trimrの左版。types.cの共有関数は増やさずmd.c内に留める) */
static str triml(str s) {
  while (s.len && isspace((unsigned char)s.buf[0])) {
    s.buf++;
    s.len--;
  }
  return s;
}

static void eval_inline(heap_str* res, md_inline* mi) {
  switch (mi->type) {
    case MDI_TEXT:
      str_cat(res, mi->data.text);
      break;
    case MDI_CODE:
      str_cat(res, to_str("<code>"));
      str_cat(res, mi->data.text);
      str_cat(res, to_str("</code>"));
      break;
    case MDI_EMPH:
      str_cat(res, to_str("<em>"));
      str_cat(res, mi->data.text);
      str_cat(res, to_str("</em>"));
      break;
    case MDI_STRONG:
      str_cat(res, to_str("<strong>"));
      str_cat(res, mi->data.text);
      str_cat(res, to_str("</strong>"));
      break;
    case MDI_LINK:
      str_cat(res, to_str("<a href=\""));
      str_cat(res, mi->data.attr);
      str_cat(res, to_str("\">"));
      str_cat(res, mi->data.text);
      str_cat(res, to_str("</a>"));
      break;
    case MDI_IMAGE:
      str_cat(res, to_str("<img src=\""));
      str_cat(res, mi->data.attr);
      str_cat(res, to_str("\" alt=\""));
      str_cat(res, mi->data.text);
      str_cat(res, to_str("\">"));
      break;
  }
}

/* インライン要素の並びを終端ノード(text=="")まで評価してresに積む。
   終端ノードのattr(marker)を*markerに返し、終端の次のインデックスを返す */
static size_t eval_run(md_list list, size_t i, heap_str* res, str* marker) {
  for (;;) {
    md_inline* c = &list.buf[i++].data.content;
    if (c->type == MDI_TEXT && c->data.text.len == 0) {
      *marker = c->data.attr;
      return i;
    }
    eval_inline(res, c);
  }
}

/* iから始まるリスト項目が番号付きか(終端ノードのmarkerだけを覗き見る) */
static bool_ list_item_ordered(md_list list, size_t i) {
  for (;;) {
    md_inline* c = &list.buf[i++].data.content;
    if (c->type == MDI_TEXT && c->data.text.len == 0)
      return c->data.attr.len && isdigit((unsigned char)c->data.attr.buf[0]);
  }
}

/* "---"や":--:"だけの表の区切り行か */
static bool_ is_sep_row(str s) {
  size_t i;
  bool_ has_dash = false;
  for (i = 0; i < s.len; i++) {
    char c = s.buf[i];
    if (c == '-') {
      has_dash = true;
    } else if (c != ':' && c != '|' && !isspace((unsigned char)c)) {
      return false;
    }
  }
  return has_dash;
}

heap_str eval_md(md_list list) {
  heap_str res = new_str(0);
  size_t i = 0;

  while (i < list.len) {
    md_node* e = list.buf + i;

    switch (e->type) {
      case MD_LINE:
        str_cat(&res, to_str("<hr>\n"));
        i++;
        break;

      case MD_HTML:
        str_cat(&res, e->data.html);
        str_cat(&res, to_str("\n"));
        i++;
        break;

      case MD_CODE:
        str_cat(&res, to_str("<pre><code"));
        if (e->data.content.data.attr.len) {
          str_cat(&res, to_str(" class=\"language-"));
          str_cat(&res, e->data.content.data.attr);
          str_cat(&res, to_str("\""));
        }
        str_cat(&res, to_str(">"));
        str_cat(&res, e->data.content.data.text);
        str_cat(&res, to_str("</code></pre>\n"));
        i++;
        break;

      case MD_HEAD: {
        heap_str inner = new_str(0);
        str marker;
        char tag[3];
        i = eval_run(list, i, &inner, &marker);
        tag[0] = 'h';
        tag[1] = (char)('0' + marker.len);
        tag[2] = 0;
        str_cat(&res, to_str("<"));
        str_cat(&res, to_str(tag));
        str_cat(&res, to_str(">"));
        str_cat(&res, inner);
        str_cat(&res, to_str("</"));
        str_cat(&res, to_str(tag));
        str_cat(&res, to_str(">\n"));
        free_str(&inner);
        break;
      }

      case MD_PAR: {
        heap_str inner = new_str(0);
        str marker;
        i = eval_run(list, i, &inner, &marker);
        str_cat(&res, to_str("<p>"));
        str_cat(&res, inner);
        str_cat(&res, to_str("</p>\n"));
        free_str(&inner);
        break;
      }

      case MD_QUOTE: {
        str_cat(&res, to_str("<blockquote>\n"));
        while (i < list.len && list.buf[i].type == MD_QUOTE) {
          heap_str inner = new_str(0);
          str marker;
          i = eval_run(list, i, &inner, &marker);
          str_cat(&res, to_str("<p>"));
          str_cat(&res, inner);
          str_cat(&res, to_str("</p>\n"));
          free_str(&inner);
        }
        str_cat(&res, to_str("</blockquote>\n"));
        break;
      }

      case MD_LIST: {
        heap_str inner = new_str(0);
        str marker;
        bool_ ordered;
        i = eval_run(list, i, &inner, &marker);
        ordered = marker.len && isdigit((unsigned char)marker.buf[0]);
        str_cat(&res, ordered ? to_str("<ol>\n") : to_str("<ul>\n"));
        str_cat(&res, to_str("<li>"));
        str_cat(&res, inner);
        str_cat(&res, to_str("</li>\n"));
        free_str(&inner);
        while (i < list.len && list.buf[i].type == MD_LIST &&
               list_item_ordered(list, i) == ordered) {
          heap_str item = new_str(0);
          i = eval_run(list, i, &item, &marker);
          str_cat(&res, to_str("<li>"));
          str_cat(&res, item);
          str_cat(&res, to_str("</li>\n"));
          free_str(&item);
        }
        str_cat(&res, ordered ? to_str("</ol>\n") : to_str("</ul>\n"));
        break;
      }

      case MD_TABLE: {
        bool_ first_row = true;
        str_cat(&res, to_str("<table>\n"));
        while (i < list.len && list.buf[i].type == MD_TABLE) {
          heap_str inner = new_str(0);
          str marker;
          i = eval_run(list, i, &inner, &marker);

          if (is_sep_row(inner)) {
            free_str(&inner);
            continue;
          }

          {
            str rest = inner;
            str open = first_row ? to_str("<th>") : to_str("<td>");
            str close = first_row ? to_str("</th>") : to_str("</td>");
            str_cat(&res, to_str("<tr>"));
            for (;;) {
              ptrdiff_t d = str_find(rest, "|");
              str cell = triml(str_trimr(d < 0 ? rest : str_take(rest, d)));
              if (cell.len) {
                str_cat(&res, open);
                str_cat(&res, cell);
                str_cat(&res, close);
              }
              if (d < 0) break;
              rest = str_drop(rest, d + 1);
            }
            str_cat(&res, to_str("</tr>\n"));
          }

          free_str(&inner);
          first_row = false;
        }
        str_cat(&res, to_str("</table>\n"));
        break;
      }
    }
  }

  return res;
}
