#include <stdio.h>
#include "parser.h"
#include <string.h>

int main(int argc, char* argv[]) {
  tmpl_list li = parse_tmpl(to_str(
      "hello\n"
      "world\n"
      "{{ a 1\n"
      "}}"
      "\n"));

  int i;
  tmpl_node e;
  for (i = 0; i < li.len; i++) {
    e = li.buf[i];
    printf("#%d: %d :: ", i, e.type);
    if (e.type == TMPL_TEXT) prints(e.data.text);
    puts("");
  }

  return 0;
}
