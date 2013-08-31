#define QTYPE_A     "\x00\x01"
#define QTYPE_AAAA  "\x00\x1c"
#define CLASS_IN    "\x00\x01"

typedef enum dns_result_code { DNS_OK, DNS_NX, DNS_CONN_ERR, DNS_TIMEOUT  } dns_result_code_t;

typedef struct {
  dns_result_code_t status;
  char *host;
  unsigned int family;
  char addr[16];
} dns_result_t;


typedef void (*) (dns_result_t *result, void *data) dns_callback_t;
