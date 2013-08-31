#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "simple_buffer.h"

simple_buffer_t *simple_buffer_alloc(size_t size){
  simple_buffer_t *buffer=malloc(sizeof(simple_buffer_t));
  buffer->free=NULL;
  buffer->free_hint=NULL;
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

void simple_buffer_destroy_contents(simple_buffer_t *buffer){
  if(buffer->free) buffer->free(buffer->ptr, buffer->free_hint);
  else{free(buffer->ptr)};
}

void simple_buffer_destroy(simple_buffer_t *buffer){
  simple_buffer_destroy_contents(buffer);
  free(buffer);;
}

void simple_buffer_print(simple_buffer_t *buf){
  int i;
  for(i=0;i<buf->write_pos;i++){
    putc(buf->ptr[i],stdout);
  }
  putc('\n',stdout);
  
}

void simple_buffer_write_packed_string(simple_buffer_t *buffer, const char *str, const size_t size){
  register unsigned int wp=buffer->write_pos;
  register unsigned int nwp=wp+size+1;
  if(nwp>=buffer->size){
    buffer->size=nwp * 2;
    buffer->ptr=realloc(buffer->ptr, buffer->size);
  }
  memcpy(buffer->ptr+wp, str, size);
  buffer->ptr[nwp-1]='\0';
  buffer->write_pos=nwp;
}



inline void simple_buffer_write_packed_string2(simple_buffer_t *buffer, const char *str){
  simple_buffer_write_packed_string(buffer,str,strlen(str));
}


simple_buffer_t *simple_buffer_t(simple_buffer_t *src){
  simple_buffer_t *d=malloc(sizeof(simple_buffer));
  d->read_pos=src->read_pos;
  d->write_pos=src->write_pos;
  d->size=src->size;
  d->ptr=malloc(d->size);
  memcpy(d->ptr,src->ptr,d->size);
  return d;
}

simple_buffer_t *simple_buffer_dup_from_ptr(char *ptr, size_t size){
  if(!size){
    return simple_buffer_alloc(64);
  }
  simple_buffer_t *buf=simple_buffer_alloc(size+1);
  memcpy(buf->ptr,ptr, size);
  buf->size=size;
  buf->write_pos=size;
  buf->read_pos=0;
  return buf;
}

void simple_buffer_set_from_ptr(simple_buffer_t *buf, char *ptr, size_t size){
  buf->read_pos=0;
  buf->write_pos=size;
  buf->size=size;
  buf->ptr=ptr;
}

void simple_buffer_reset(simple_buffer_t *buffer){
  buffer->read_pos=0;
  buffer->write_pos=0;
}


char *simple_buffer_read_line(simple_buffer_t *buffer, int *retsize){
  register int r=buffer->read_pos;
  register char *p=buffer->ptr+r;
  register char *s=p;
  register char *e=buffer->ptr+buffer->write_pos;
  // while(p<e && (*p)!='\n') p++;
 

    p=memchr(p,'\n',e-p);
    if(!p){
      *retsize=0;
      return NULL;
    }
  
  *retsize=p-buffer->ptr-r;
  buffer->read_pos=p-buffer->ptr+1;
  return s;

}


size_t simple_buffer_concat(simple_buffer_t *dst, simple_buffer_t *src){
  return simple_buffer_write(dst, src->ptr, src->write_pos);
}

/* returns number of bytes actually written */

size_t simple_buffer_write(simple_buffer_t *buffer, const char *ptr, size_t size){
  register unsigned int wp=buffer->write_pos;
  register unsigned int nwp=wp+size-1;
  if(nwp>=buffer->size){
    buffer->size=nwp * 2;
    buffer->ptr=realloc(buffer->ptr, buffer->size);
  }
  memcpy(buffer->ptr+wp, ptr, size);
  buffer->write_pos+=size;
  return size;
}

char *simple_buffer_get_string(simple_buffer_t *buffer){
  simple_buffer_write_char(buffer, 0);
  buffer->write_pos--;
  return buffer->data;

}

void simple_buffer_write_char(simple_buffer_t *buffer, unsigned char c){
  simple_buffer_write(buffer, &c, 1);
}



void simple_buffer_write_short(simple_buffer_t *buffer, uint16_t s){
  simple_buffer_write(buffer, &s, 2);
}

size_t simple_buffer_write_int_as_string(simple_buffer_t *buffer, int i){
  static char *s_buf=NULL;
  if(s_buf==NULL) s_buf=malloc(64);
  snprintf(s_buf,63,"%d",i);
  return simple_buffer_write_string(buffer, s_buf);
}

/* returns number of bytes actually written */

size_t simple_buffer_write_string(simple_buffer_t *buffer, const char *string){
  if(string==NULL) return 0;
  return simple_buffer_write(buffer, string, strlen(string));
}



int simple_buffer_write_to_fd(simple_buffer_t *buffer, int fd){
  int ret;
  char *readptr=buffer->ptr+buffer->read_pos;
  char left=buffer->write_pos-buffer->read_pos;
  ret=write(fd, readptr, left);
  if(ret>0){
    buffer->read_pos+=ret;
    left-=ret;
  }
  return left;
}



int simple_buffer_read_from_fd(simple_buffer_t *buffer, int fd, unsigned int size){
  char *buf=malloc(size);
  int ret=read(fd, buf, size);
  if(ret>0){
    simple_buffer_write(buffer, buf, ret);
  }
  free(buf);
  return ret;
}


char *simple_buffer_read(simple_buffer_t *buffer, size_t size, int *retread){
  char *readptr=buffer->ptr+buffer->read_pos;
  char left=buffer->write_pos-buffer->read_pos;
  size_t r=left < size ? left : size;
  *retread=r;
  buffer->read_pos+=r;
  return readptr;
}
