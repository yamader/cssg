#include "tmpl.h"

#include <stdlib.h>

tmpl_list new_tmpl_list() {
  tmpl_list list;
  list.len = 0;
  list.cap = 2;
  list.buf = malloc(sizeof(tmpl_node) * list.cap);
  return list;
}

void free_tmpl_list(tmpl_list* list) {
  free(list->buf);
}

void tmpl_list_push(tmpl_list* list, tmpl_node node) {
  if (list->len >= list->cap) {
    list->cap *= 2;
    list->buf = realloc(list->buf, sizeof(tmpl_node) * list->cap);
  }
  list->buf[list->len++] = node;
}

heap_str eval_tmpl(tmpl_list list) {
}
