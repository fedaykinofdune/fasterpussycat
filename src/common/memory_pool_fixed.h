struct memory_pool_fixed_chunk;
struct memory_pool_fixed_item;

typedef struct {
  size_t m_item_size;
  size_t item_size;
  int capacity;
  struct memory_pool_fixed_chunk *chunk_list;
  struct memory_pool_fixed_item *free_list;
} memory_pool_fixed_t;


/* memory_pool_fixed.c */
memory_pool_fixed_t *memory_pool_fixed_new(size_t item_size, int start_capacity);
void memory_pool_fixed_free(memory_pool_fixed_t *pool, void *mem);
void *memory_pool_fixed_alloc(memory_pool_fixed_t *pool);
void memory_pool_fixed_destroy(memory_pool_fixed_t *pool);
