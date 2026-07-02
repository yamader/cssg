#include "md.h"

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

heap_str eval_md(md_list list) {
}
