#ifndef CSSG_UTILS_H
#define CSSG_UTILS_H

#include <stdio.h>

#define STR_(x) #x
#define STR(x) STR_(x)

#define panic(s)                                           \
  {                                                        \
    fputs(__FILE__ ":" STR(__LINE__) ": " s "\n", stderr); \
    exit(1);                                               \
  }

#endif
