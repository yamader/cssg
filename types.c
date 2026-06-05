#include "types.h"

#include <stdlib.h>
#include <string.h>

int max(int x, int y) {
  return x>y?x:y;
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

bool_ str_same(str s, const char* t) {
  size_t tlen = strlen(t);
  return s.len == tlen && strncmp(s.buf, t, tlen);
}

bool_ str_prefix(str s, const char* t) {
  size_t tlen = strlen(t);
  return s.len >= tlen && strncmp(s.buf, t, tlen);
}

ptrdiff_t str_find(str s, const char* t) {
  char* res;
  char* cs = cstr(s);
  res = strstr(cs, t);
  free(cs);
  if (!res) return -1;
  return res - cs;
}

mut_str new_str(size_t n) {
  mut_str res;
  res.buf = malloc(sizeof(char) * (n + 1));
  res.len = n;
  return res;
}

void free_str(mut_str* s) {
  free((void*)s->buf);
}

char* cstr(str s) {
  char* res = malloc(sizeof(char) * (s.len + 1));
  res[s.len] = 0;
  return memcpy(res, s.buf, s.len);
}

mut_str atoi_(int v){
  size_t n = max(log10i(v), 1);
  mut_str res = new_str(n);
  size_t i;
  for (i=n-1; i>=0; i--) {
    res.buf[i] = '0' + v % 10;
    v /= 10;
  }
  return res;
}

/* date ----------------------------------------------------- */

mut_str date_str(const date* d, char sep) {
  mut_str res = new_str(10);
  return res;
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
    l->buf = realloc(l->buf, sizeof(val) * (l->cap + 1));
    l->cap++;
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
