/* util.c */

extern int show_debug;

void print_mem(void *mem, int len);

void info(char *format, ...);
void fatal(char *format, ...);
void warn(char *format, ...);
void debug(char *format, ...);
char *trim(char *str);
