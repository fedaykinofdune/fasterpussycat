#ifndef SHOGGOTH_SIMPLE_BUFFER_H
#define SHOGGOTH_SIMPLE_BUFFER_H
#include <stdlib.h>

#define simple_buffer_size(_buf) _buf->write_pos

typedef struct {
  size_t size;
  unsigned int read_pos;
  unsigned int write_pos;
  char *ptr;
  void (*free) ((void *), (void *));
  void *free_hint;
} simple_buffer;


/* simple_buffer.c */



simple_buffer *alloc_simple_buffer(size_t size);
void destroy_simple_buffer(simple_buffer *buffer);
void write_packed_string_to_simple_buffer(simple_buffer *buf, const char *str, size_t size);
void write_packed_string_to_simple_buffer2(simple_buffer *buf, const char *str);
simple_buffer *dup_simple_buffer(simple_buffer *src);
void set_simple_buffer_from_ptr(simple_buffer *buf, char *ptr, size_t size);
void reset_simple_buffer(simple_buffer *buffer);
void print_simple_buffer(simple_buffer *buffer);
char *read_line_from_simple_buffer(simple_buffer *buffer, int *retsize);
size_t concat_simple_buffer(simple_buffer *dst, simple_buffer *src);
size_t write_to_simple_buffer(simple_buffer *buffer, const char *ptr, size_t size);
size_t write_int_to_simple_buffer(simple_buffer *buffer, int i);
size_t write_string_to_simple_buffer(simple_buffer *buffer, const char *string);
char *read_from_simple_buffer(simple_buffer *buffer, size_t size, int *retread);

#endif
