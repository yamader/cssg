#ifndef CSSG_FS_H
#define CSSG_FS_H

#include "types.h"

/* path */
size_t path_append_slash(char* path); /* returns new len */
const char* path_ext(const char* name);
char* path_join(const char* dir, const char* base);

/* syscall */
bool_ is_dir(const char* name);
void copy(const char* src, const char* dest);

/* posix */
#include <dirent.h>
#include <linux/limits.h> /* PATH_MAX */
#include <sys/stat.h>     /* mkdir() */
char* realpath(const char* path, char* resolved_path);

#endif
