#include "types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* xとyの評価を1回で抑えるためにマクロを避けた */
int max(int x, int y) {
  return x > y ? x : y;
}

int log10i(int v) {
  int res = 0;
  while (v) {
    v /= 10;
    res++;
  }
  return res;
}

/* str ------------------------------------------------------ */

str to_str(const char* s) {
  str res;
  res.buf = s;
  res.len = strlen(s);
  return res;
}

str str_take(str s, size_t n) {
  str res;
  res.buf = s.buf;
  res.len = n;
  return res;
}

str str_drop(str s, size_t n) {
  str res;
  res.buf = s.buf + n;
  res.len = s.len - n;
  return res;
}

str str_span(str s, size_t from, size_t to) {
  str res;
  res.buf = s.buf + from;
  res.len = to - from;
  return res;
}

bool_ str_same(str s, const char* x) {
  size_t tlen = strlen(x);
  return s.len == tlen && !strncmp(s.buf, x, tlen);
}

bool_ str_prefix(str s, const char* x) {
  size_t tlen = strlen(x);
  return s.len >= tlen && !strncmp(s.buf, x, tlen);
}

ptrdiff_t str_find(str s, const char* x) {
  char* cs = cstr(s);
  char* p = strstr(cs, x);
  ptrdiff_t res = p - cs;
  free(cs);
  if (!p) return -1;
  return res;
}

heap_str new_str(size_t n) {
  heap_str res;
  res.buf = malloc(sizeof(char) * n);
  res.len = n;
  return res;
}

void free_str(heap_str* s) {
  free((void*)s->buf);
}

char* cstr(str s) {
  char* res = malloc(sizeof(char) * (s.len + 1));
  res[s.len] = 0;
  return memcpy(res, s.buf, s.len);
}

void prints(str s) {
  char* p = cstr(s);
  printf("%s", p);
  free(p);
}

/* date ----------------------------------------------------- */

heap_str date_str(const date* d, char sep) {
  heap_str res = new_str(11);
  sprintf((char*)res.buf, "%d/%02d/%02d", d->y, d->m, d->d);
  return str_take(res, 10);
}

list new_list(size_t n) {
  list res;
  res.buf = malloc(sizeof(val) * n);
  res.len = 0;
  res.cap = n;
  return res;
}

val* list_at(const list* l, size_t i) {
  if (i >= l->len) return NULL;
  return l->buf + i;
}

val* list_push(list* l, val v) {
  if (l->len >= l->cap) {
    l->cap *= 2;
    l->buf = realloc(l->buf, sizeof(val) * l->cap);
  }
  l->buf[l->len] = v;
  return l->buf + l->len++;
}

val* list_pop(list* l) {
  return l->buf + --l->len;
}

val int_val(int i) {
  val res;
  res.type = VAL_INT;
  res.data.int_ = i;
  return res;
}

val str_val(str s) {
  val res;
  res.type = VAL_STR;
  res.data.str = s;
  return res;
}

val date_val(date d) {
  val res;
  res.type = VAL_DATE;
  res.data.date = d;
  return res;
}

val list_val(list l) {
  val res;
  res.type = VAL_LIST;
  res.data.list = l;
  return res;
}
