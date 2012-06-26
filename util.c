#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void fatal(char *format, ...){
  va_list ap;
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  abort();
}
