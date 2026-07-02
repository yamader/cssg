#ifndef CSSG_MD_H
#define CSSG_MD_H

#include "types.h"

typedef struct {
  enum {
    MDI_TEXT,
    MDI_CODE,
    MDI_EMPH,
    MDI_STRONG,
    MDI_LINK,
    MDI_IMAGE
  } type;
  struct {
    str text, attr;
  } data;
} md_inline;

typedef struct {
  enum {
    MD_LINE,
    MD_HEAD,
    MD_CODE,
    MD_HTML,
    MD_PAR,
    MD_TABLE,
    MD_QUOTE,
    MD_LIST
  } type;
  union {
    md_inline content;
    str html;
    /* todo: table */
  } data;
} md_node;

typedef struct {
  md_node* buf;
  size_t len, cap;
} md_list;

md_list new_md_list();
void free_md_list(md_list* list);
void md_list_push(md_list* list, md_node node);

heap_str eval_md(md_list list);

#endif
