#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "simple_buffer.h"

simple_buffer *alloc_simple_buffer(size_t size){
  simple_buffer *buffer=malloc(sizeof(simple_buffer));
  if(size>0){
    buffer->ptr=malloc(size);
  }
  else{
    buffer->ptr=NULL;
    buffer->size=0;
  }
  buffer->write_pos=0;
  buffer->read_pos=0;
  buffer->size=size;
  return buffer;
}


void destroy_simple_buffer(simple_buffer *buffer){
  free(buffer->ptr);
  free(buffer);;
}

void write_packed_string_to_simple_buffer(simple_buffer *buf, const char *str, size_t size){
  char z='\0';
  write_to_simple_buffer(buf,str,size);
  write_to_simple_buffer(buf,&z,1);
}


void write_packed_string_to_simple_buffer2(simple_buffer *buf, const char *str){
  char z='\0';
  write_to_simple_buffer(buf,str,strlen(str));
  write_to_simple_buffer(buf,&z,1);
}

simple_buffer *dup_simple_buffer(simple_buffer *src){
  simple_buffer *d=malloc(sizeof(simple_buffer));
  d->read_pos=src->read_pos;
  d->write_pos=src->write_pos;
  d->size=src->size;
  d->ptr=malloc(d->size);
  memcpy(d->ptr,src->ptr,d->size);
  return d;
}

void set_simple_buffer_from_ptr(simple_buffer *buf, char *ptr, size_t size){
  buf->read_pos=0;
  buf->write_pos=size;
  buf->size=size;
  buf->ptr=ptr;
}

void reset_simple_buffer(simple_buffer *buffer){
  buffer->read_pos=0;
  buffer->write_pos=0;
}


char *read_line_from_simple_buffer(simple_buffer *buffer, int *retsize){
  int r=buffer->read_pos;
  int s=buffer->read_pos;
  char *p;
  while(r<buffer->write_pos && buffer->ptr[r]!='\n') r++;
  if(r>=buffer->write_pos){
    *retsize=0;
    return NULL;
  }
  p=buffer->ptr+buffer->read_pos;
  *retsize=r-s;
  buffer->read_pos=r+1;
  return p;

}


size_t concat_simple_buffer(simple_buffer *dst, simple_buffer *src){
  return write_to_simple_buffer(dst, src->ptr, src->write_pos);
}

/* returns number of bytes actually written */

size_t write_to_simple_buffer(simple_buffer *buffer, const char *ptr, size_t size){
  if((buffer->write_pos+size-1)>=buffer->size){
    buffer->size=(buffer->write_pos+size-1) * 2;
    buffer->ptr=realloc(buffer->ptr, buffer->size);
  }
  memcpy(buffer->ptr+buffer->write_pos, ptr, size);
  buffer->write_pos+=size;
  return size;
}

size_t write_int_to_simple_buffer(simple_buffer *buffer, int i){
  static char *s_buf=NULL;
  if(s_buf==NULL) s_buf=malloc(64);
  snprintf(s_buf,63,"%d",i);
  return write_string_to_simple_buffer(buffer, s_buf);
}

/* returns number of bytes actually written */

size_t write_string_to_simple_buffer(simple_buffer *buffer, const char *string){
  if(string==NULL) return 0;
  return write_to_simple_buffer(buffer, string, strlen(string));
}



char *read_from_simple_buffer(simple_buffer *buffer, size_t size, int *retread){
  char *readptr=buffer->ptr+buffer->read_pos;
  char left=buffer->write_pos-buffer->read_pos;
  size_t r=left < size ? left : size;
  *retread=r;
  buffer->read_pos+=r;
  return readptr;
}
