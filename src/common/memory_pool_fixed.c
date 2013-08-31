#include <stdlib.h>
#include "memory_pool_fixed.h"

typedef struct memory_pool_fixed_chunk {
  struct memory_pool_fixed_chunk *next;
  char data[1];
} memory_pool_fixed_chunk_t;


typedef struct memory_pool_fixed_item {
  struct memory_pool_fixed_item *next_free;
  char data[1];
} memory_pool_fixed_item_t;

void memory_pool_fixed_new_chunk(memory_pool_fixed_t *pool, int n_items);

memory_pool_fixed_t *memory_pool_fixed_new(size_t item_size, int start_capacity){
  memory_pool_fixed_t *pool=calloc(sizeof(memory_pool_fixed_t));
  pool->item_size=item_size;
  pool->mp_item_size=item_size+offsetof(memory_pool_fixed_t, data);
  pool->free_list=NULL;
  pool->chunk_list=NULL;
  pool->capacity=0;
  memory_pool_fixed_new_chunk(pool, start_capacity);
}

void memory_pool_fixed_free(memory_pool_fixed_t *pool, void *mem){
  memory_pool_fixed_item_t *item=(memory_pool_fixed_item_t *) ( mem - offsetof(memory_pool_fixed_item_t, data) );
  item->next_free=pool->free_list;
  pool->free_list=item;
}

void *memory_pool_fixed_alloc(memory_pool_fixed_t *pool){
  if(!pool->free_list) memory_pool_fixed_new_chunk(pool, capacity);
  memory_pool_fixed_chunk_t *item=pool->free_list;
  pool->free_list=item->next_free;
  return &(item->data);
}

void memory_pool_fixed_destroy(memory_pool_fixed_t *pool){
  memory_pool_fixed_chunk_t *current, *next;
  current=pool->chunk_list;
  while(current){
      next=current->next_chunk;
      free(current);
      current=next;
  }
  free(pool);
}

void memory_pool_fixed_new_chunk(memory_pool_fixed_t *pool, int n_items){
  memory_pool_fixed_chunk_t *chunk;
  int i;
  memory_pool_fixed_item_t *item
  size_t data_size=n_items * pool->mp_item_size;
  unsigned int data_offset=offsetof(memory_pool_fixed_chunk_t, data);
  chunk=calloc(data_size+data_offset,1);
  chunk->next=pool->chunk_list;
  pool->chunk_list=chunk;
  pool->capacity=+n_items;
  for(i=0;i<n_items;i++){
    item=(memory_pool_fixed_item_t *) chunk->data+(pool->mp_item_size*i);
    memory_pool_fixed_free( pool, &(item->data) );
  }
}


