#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

inline void maybe_newline(FILE *f, char *format){
  if(format[strlen(format)-1]!='\n'){
    fprintf(f,"\n");
  }
}

void info(char *format, ...){
  va_list ap;
  va_start(ap, format);
  fprintf(stdout, " :: ");
  vfprintf(stdout, format, ap);
  va_end(ap);
  maybe_newline(stdout,format);
  fflush(stdout);
}



void fatal(char *format, ...){
  va_list ap;
  va_start(ap, format);
  fprintf(stderr, "FATAL ERROR: ");
  vfprintf(stderr, format, ap);
  va_end(ap);
  maybe_newline(stderr,format);
  fflush(stderr);
  abort();
}

void warn(char *format, ...){
  va_list ap;
  va_start(ap, format);
  fprintf(stdout, " :! ");
  vfprintf(stdout, format, ap);
  va_end(ap);
  maybe_newline(stdout,format);
  fflush(stdout);
}
