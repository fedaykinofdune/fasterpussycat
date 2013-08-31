tcp_endpoint_t *tcp_endpoint_alloc(struct sockaddr_storage addr, int use_ssl){
  tcp_endpoint_t *t=malloc(sizeof(tcp_endpoint_t));
  t->addr=addr;
  t->use_ssl=use_ssl
  return t;
}

void tcp_endpoint_destroy(tcp_endpoint_t *t){
  free(t);
}

