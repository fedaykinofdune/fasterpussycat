#ifndef FASTERPUSSYCAT_SIMPLE_BUFFER_H
#define FASTERPUSSYCAT_SIMPLE_BUFFER_H

typedef struct {
  size_t size;
  unsigned int read_pos;
  unsigned int write_pos;
  char *ptr;
} simple_buffer;


/* simple_buffer.c */
simple_buffer *alloc_simple_buffer(size_t size);
void free_simple_buffer(simple_buffer *buffer);
void reset_simple_buffer(simple_buffer *buffer);
size_t write_to_simple_buffer(simple_buffer *buffer, char *ptr, size_t size);
size_t write_string_to_simple_buffer(simple_buffer *buffer, char *string);
