#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

typedef struct connection connection;


enum connection_state { NOTINIT, CONNECTING, READY, WRITING, READING }; 

struct connection {
  unsigned char use_ssl;
  connection_state state;
  unsigned char done_ssl_handshake;
  unsigned char retry;
  unsigned char reused;
  http_response response;
  simple_buffer *read_buffer;
  simple_buffer *aux_buffer;
  prepared_request *request;
  unsigned int fd;
  unsigned int index;
  time_t lastrw;
  struct sockaddr *addr;
  SSL *sslHandle;
  SSL_CTX *sslContext;
  connection *next_conn;
  connection *next_idle;

};
