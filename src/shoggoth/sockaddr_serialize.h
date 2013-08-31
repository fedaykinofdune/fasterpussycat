#define SHOG_AF_INET 0
#define SHOG_AF_INET6 1
#define SHOG_SOCKADDRSIZE 19

/* sockaddr_serialize.c */
void sockaddr_serialize(struct sockaddr *addr, unsigned char *out);
void sockaddr_unserialize(unsigned char *in, struct sockaddr_storage *out);
