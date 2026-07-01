#ifndef CSSG_PARSER_H
#define CSSG_PARSER_H

#include "template.h"
#include "types.h"

/* common parsers ------------------------------------------- */

/* template parsers ----------------------------------------- */

tmpl_list parse_tmpl(const str s);

/* markdown parsers ----------------------------------------- */

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
    /* table */
  } data;
} md_node;

typedef struct {
  md_node* buf;
  size_t len, cap;
} md_list;

md_list parse_md(const str s);

/* frontmatter parsers -------------------------------------- */

typedef struct {
  dict frontmatter;
  str content;
} page;

page parse_fm(const str s);

#endif
