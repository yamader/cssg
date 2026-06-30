#ifndef CSSG_TYPES_H
#define CSSG_TYPES_H

#include <stddef.h>

/* scalar --------------------------------------------------- */

typedef unsigned char ubyte;

typedef ubyte bool_;
#define true 1
#define false 0

/* str ------------------------------------------------------ */

typedef struct {
  const char* buf;
  size_t len;
} str;

str to_str(const char* s);
str str_take(str s, size_t n);
str str_drop(str s, size_t n);
str str_span(str s, size_t from, size_t to);
bool_ str_same(str s, const char* x);
bool_ str_prefix(str s, const char* x);
ptrdiff_t str_find(str s, const char* x);

typedef str heap_str;

heap_str new_str(size_t n);
void free_str(heap_str* s);
char* cstr(str s);

void prints(str s);

/* date ----------------------------------------------------- */

typedef struct {
  short y;
  ubyte m;
  ubyte d;
} date;

heap_str date_str(const date* d, char sep);

/* list ----------------------------------------------------- */

typedef struct val val;

typedef struct {
  val* buf;
  size_t len, cap;
} list;

list new_list(size_t n);
val* list_at(const list* l, size_t i);
val* list_push(list* l, val v);
val* list_pop(list* l);

/* val ------------------------------------------------------ */

struct val {
  enum {
    VAL_INT,
    VAL_STR,
    VAL_DATE,
    VAL_LIST
  } type;
  union {
    int int_;
    str str;
    date date;
    list list;
  } data;
};

val int_val(int i);
val str_val(str s);
val date_val(date d);
val list_val(list l);

/* optional ------------------------------------------------- */

#define DEFINE_OPT(T, V) \
  typedef struct {       \
    V v;                 \
    bool_ ok;            \
  } T;                   \
  static T to_##T(V v) { \
    T res;               \
    res.v = v;           \
    res.ok = true;       \
    return res;          \
  }                      \
  static T null_##T() {  \
    T res;               \
    res.ok = false;      \
    return res;          \
  }

DEFINE_OPT(opt_int, int)
DEFINE_OPT(opt_size, size_t)
DEFINE_OPT(opt_str, str)

#endif
