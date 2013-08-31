#include <sys/socket.h>

/* out must be alloced to at least 19 bytes */

void sockaddr_serialize(struct sockaddr *addr, unsigned char *out){
  struct sockaddr_in *addr4;
  struct sockaddr_in6 *addr6;
  uint32_t a32;
  uint16_t b16;
  switch(addr->sa_family){
    case AF_INET:
      out[0]=SHOG_AF_INET;
      addr4=(struct sockaddr_in *) addr;
      a32=addr4->sin_addr.s_addr;
      b16=addr4->sin_port;
      memcpy(out+1, &b16, 2);
      memcpy(out+3, &a32, 4);
      break;
    case AF_INET6:
      out[0]=SHOG_AF_INET6;
      addr6=(struct sockaddr_in6 *) addr;
      b16=addr6->sin_port;
      memcpy(out+1, &b16, 2);
      memcpy(out+3, &(addr6->sin6_addr.s6_addr), 16);
      break;
    default:
      errx(1, "other protocols not supported");
  }

}

void sockaddr_unserialize(unsigned char *in, struct sockaddr_storage *out){
  struct sockaddr_in *addr4;
  struct sockaddr_in6 *addr6;
  memset(out,0, sizeof(sockaddr_storage));
  switch(in[0]){
    case SHOG_AF_INET:
      addr4=(sockaddr_in *) out;
      addr4->sin_family=AF_INET;
      addr4->sin_port=*((uint16_t *) in+1);
      addr4->sin_addr.s_addr=*((uint32_t *) in+3);
    case SHOG_AF_INET6:
      addr6=(sockaddr_in6 *) out;
      addr6->sin6_family=AF_INET6;
      addr6->sin6_port=*((uint16_t *) in+1);
      memcpy(&(addr6->sin6_addr.s6_addr), in+3, 16);
    default:
      errx(1, "Type %d not supported", in[0]);
  }

}
