

struct http_header_t;


typedef struct {
  char *key;
  char *value;
  int no_free;
  struct http_header_t *next;
} http_header_t;

