#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

int show_debug=0;

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



void debug(char *format, ...){
  if(!show_debug) return;
  va_list ap;
  va_start(ap, format);
  fprintf(stdout, " :? ");
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


void print_mem(void *mem, int len){
  int i;
  for(i=0;i<len;i++){
    printf("%2.2x",((unsigned char*) mem)[i]);
  }
}

char *trim(char *str)
{
  char *end;

  // Trim leading space
  while(isspace(*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}
