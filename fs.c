#include "fs.h"

#include <fcntl.h>
#include <linux/fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* path ----------------------------------------------------- */

size_t path_append_slash(char* path) {
  size_t l = strlen(path);
  path[l++] = '/';
  path[l] = '\0';
  return l;
}

const char* path_ext(const char* name) {
  char* p = strrchr(name, '.');
  return p ? p : "";
}

char* path_join(const char* dir, const char* base) {
  size_t dlen = strlen(dir);
  size_t blen = strlen(base);
  char* buf = malloc(dlen + 1 + blen + 1);
  memcpy(buf, dir, dlen);
  buf[dlen] = '/';
  memcpy(buf + dlen + 1, base, blen + 1);
  return buf;
}

/* syscall -------------------------------------------------- */

bool_ is_dir(const char* path) {
  struct stat sb;
  stat(path, &sb);
  return S_ISDIR(sb.st_mode);
}

void copy(const char* src, const char* dest) {
  int fd_src = open(src, O_RDONLY);
  int fd_dest = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);

  if (ioctl(fd_dest, FICLONE, fd_src)) {
    ubyte buf[4096];
    ssize_t n;
    while ((n = read(fd_src, buf, sizeof(buf))) > 0)
      write(fd_dest, buf, n);
  }

  close(fd_src);
  close(fd_dest);
}

heap_str read_all(const char* path) {
  FILE* f = fopen(path, "rb");
  heap_str buf;
  size_t size;

  fseek(f, 0, SEEK_END);
  size = ftell(f);
  rewind(f);
  buf = new_str(size);
  fread((char*)buf.buf, 1, buf.len, f);

  fclose(f);
  return buf;
}

void write_all(const char* path, str s) {
  FILE* f = fopen(path, "wb");
  fwrite(s.buf, 1, s.len, f);
  fclose(f);
}
