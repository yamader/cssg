#ifndef CSSG_PARSER_H
#define CSSG_PARSER_H

#include "expr.h"
#include "md.h"
#include "tmpl.h"
#include "types.h"

/* common parsers ------------------------------------------- */

/* template parsers ----------------------------------------- */

tmpl_list parse_tmpl(const str s);

/* markdown parsers ----------------------------------------- */

md_list parse_md(const str s);

/* frontmatter parsers -------------------------------------- */

typedef struct {
  str key, val;
} fm_item;

typedef struct {
  fm_item* buf;
  size_t len, cap;
} fm_list;

typedef struct {
  fm_list fm;
  str content;
} page;

page parse_fm(const str s);

#endif
