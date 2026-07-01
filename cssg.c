#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs.h"
#include "parser.h"

char* strdup(const char* __s);

struct {
  const char* src;
  const char* dest; /* must cstr for opendir */
  char* ignore;
} conf;

void loadconf(int argc, char* argv[]) {
  conf.src = argc >= 2 ? argv[1] : ".";
  conf.dest = argc >= 3 ? argv[2] : "dist";
  conf.ignore = NULL;

  { /* destがsrcの中にあるときignoreを追加する */
    char realsrc[PATH_MAX], realdest[PATH_MAX];
    size_t rslen, rdlen;
    realpath(conf.src, realsrc);
    realpath(conf.dest, realdest);
    rslen = path_append_slash(realsrc);
    rdlen = path_append_slash(realdest);
    if (rslen <= rdlen && !strncmp(realsrc, realdest, rslen))
      conf.ignore = strdup(realdest + rslen);
  }
}

void render_md(const char* src, const char* dest) {
}

void render_html(const char* src, const char* dest) {
}

void put_file(const char* src, const char* dest) {
  copy(src, dest);
}

void walk(DIR* dir, char* base) {
  struct dirent* ent;

  /* ignore . and .. */
  readdir(dir), readdir(dir);

  while ((ent = readdir(dir))) {
    char* name = base ? path_join(base, ent->d_name) : ent->d_name;
    char* src = path_join(conf.src, name);
    char* dest = path_join(conf.dest, name);

    /* 隠しファイルは除去 */
    if (*ent->d_name == '.') goto next;

    /* ignoreをスキップ */
    if (conf.ignore) {
      char* name_slash = strdup(name);
      size_t name_len = path_append_slash(name_slash);
      bool_ ignored = !strncmp(name_slash, conf.ignore, name_len);
      free(name_slash);
      if (ignored) goto next;
    }

    if (is_dir(name)) {
      DIR* nextdir = opendir(name);
      mkdir(dest, 0755);
      walk(nextdir, name);
      closedir(nextdir);
    } else if (!strcmp(path_ext(name), ".md")) {
      render_md(src, dest);
    } else if (!strcmp(path_ext(name), ".html")) {
      render_html(src, dest);
    } else {
      put_file(src, dest);
    }

  next:
    free(dest);
    free(src);
    if (base) free(name);
  }
}

int main(int argc, char* argv[]) {
  loadconf(argc, argv);

  {
    DIR* root = opendir(conf.src);
    mkdir(conf.dest, 0755);
    walk(root, NULL);
    closedir(root);
  }

  /*
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
  */

  md_list li = parse_md(to_str(
      "# hello\n"
      "[a](a)\n"
      "*a*a*\n"));
  int i;
  md_node e;
  for (i = 0; i < li.len; i++) {
    e = li.buf[i];
    printf("%d: %d", i, e.type);
    puts("");
  }

  return 0;
}
