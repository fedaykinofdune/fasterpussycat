
#include "shoggoth/dns.h"
#include "common/memory_pool_fixed.h"

struct dns_query_t;

typedef struct dns_query_t {
  uint32_t id;
  char *host;
  dns_callback_t callback;
  time_t time_sent;
  int attempts;
  char *qtype;
  void *data;
  UT_hash_handle *hh
}
dns_query_t;

#define DNS_TIMEOUT 4

static dns_query_t *outstanding_queries;
static uint16_t next_free=-1;
static uint16_t current_id=0;
static size_t capacity=0;
static memory_pool_fixed_t *query_pool;
static memory_pool_fixed_t *result_pool;
static simple_buffer_t *packet;
static int family=0;
static sockaddr_storage nameserver_addr;
static int sockfd;

/* returns socket */

int dns_init(){
  int i;
  int flags
  capacity=64;
  dns_get_nameserver();
  query_pool=memory_pool_fixed_new(sizeof(dns_query_t), 64);
  result_pool=memory_pool_fixed_new(sizeof(dns_result_t), 64);

  packet=simple_buffer_alloc(128);
  
  switch(nameserver_addr->family){
    case AF_INET:
      sockfd=socket(AF_INET, SOCK_DGRAM, 0);
      connect(sockfd, (sockaddr *) nameserver_addr, sizeof(sockaddr_in));
      break;
    case AF_INET6;
      sockfd=socket(AF_INET6, SOCK_DGRAM, 0);
      connect(sockfd, (sockaddr *) nameserver_addr, sizeof(sockaddr_in6));
    break;
  }

  int flags;
  flags = fcntl(sockfd, F_GETFL, 0);
  fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
  return sockfd;
}

void dns_free_result(dns_result_t *r){
  memory_pool_fixed_free(result_pool, r);
}

void dns_get_nameserver(){
  FILE  *fp;
  int size;
  char  line[512];
  char *ptr;
  char *addr[16];
  struct *sockaddr_in addr4;
  struct *sockaddr6_in addr6;
  if ((fp = fopen("/etc/resolv.conf", "r")) == NULL) {
    errx(1,"Couldn't open /etc/resolv.conf");
  } else {
    /* Try to figure out what DNS server to use */
    for (ret--; fgets(line, sizeof(line), fp) != NULL; ) {
      if (!strcmpn(line, "nameserver", 10)){
        for(ptr=line+10;ptr;ptr++){
          if(*ptr>0x2f && *ptr<0x67) break;
        }
        if(!*ptr) errx(1,"Couldn't parse nameserver");
        break;
      }
    }
    if(inet_pton(AF_INET, ptr, addr)){
        family=AF_INET;
    }
    else if (inet_pton(AF_INET6, ptr, addr)){
        family=AF_INET6;
    }
    if(family==0){
      errx(1,"Couldn't parse nameserver");
    }
    switch(family){
      case AF_INET:
        addr4=(sockaddr_in *) &nameserver_addr;
        addr4->sin_family=family;
        addr4->sin_port=htons(53);
        memcpy( &(addr4->sin_addr.s_addr), addr, 4);
        break;
      case AF_INET6:
        addr6=(sockaddr_in6 *) &nameserver_addr;
        addr6->sin6_family=family;
        addr6->sin6_port=htons(53);
        memcpy( &(addr6->sin6_addr.s6_addr), addr, 16);
        break;
    }

}
    fclose(fp);
}



dns_result_t *dns_result_alloc(){
  dns_result_t *r=memory_pool_fixed_alloc(result_pool);
  return r;
}

dns_query_t *dns_query_alloc(){
  dns_query_t *q=memory_pool_fixed_alloc(query_pool);
  return q;
}

void dns_query_free(dns_query_t *q){
  HASH_DEL(outstanding_queries, q);
  memory_pool_fixed_free(memory_pool, q);
}

void dns_query(char *qtype, char *host, dns_callback_t callback, void *data){
  dns_query_t *q=dns_query_alloc();
  q->host=host;
  q->data=data;
  q->id=++current_id;
  q->call_back=callback;
  q->attempts=0;
  q->qtype=0;
  dns_send_query(q);
}

void dns_send_query(dns_query_t *q){
  q->time_sent=time(NULL);
  if(q->attempts>0) HASH_DEL(outstanding_queries, q);
  q->attempts++;
  HASH_ADD_INT( outstanding_queries, q, id);
  dns_make_query_packet(q->qtype, q->id, q->host, packet);
  send(sockfd, packet->ptr,simple_buffer_size(packet));
}

void dns_process(){
  int BUFFER_SIZE=128;
  char recv_buffer[BUFFER_SIZE];
  uint16_t *recv_u16;
  char len;
  char *end;
  uint16_t answers;
  uint32_t tid;
  dns_query_t *q;
  dns_result *result;
  char *ptr;
  int type;
  while((len=recv(sockfd, recv_buffer, BUFFER_SIZE))>0){
    recv_u16=(uint16_t *) recv_buffer; 
    if(len<12){
      warnx("Undersized DNS reply received !?");
      continue;
    }
    end=recv_buffer+len;
    tid = recv_u16[0];
    HASH_FIND_INT(outstanding_queries, &id, q);
    if(!q){
      warnx("Couldn't find query!");
      continue;
    }
    result=dns_result_alloc();
    result.family=query.family;
      
    answers = htons(recv_u16[3]);
    
    if(!answers){
      result.status=DNS_NX;
      dns_send_result(q, result);
      continue;
    }

    for(ptr=recv_buffer+12; ptr<end && *ptr; ptr++);
    ptr+=5;

    while(ptr<(end-12)){

      /* skip cname */

      if(*ptr!=0xc0){
        for(;ptr<(end-12) && *ptr;ptr++);
        ptr--;
      }
      
      ptr++;
      type=htons(*( (uint16_t *) ptr));
      


      type=
      
    }




    

  }

}


void dns_make_query_packet(char *qtype, uint16_t id, char *host, simple_buffer_t *out);
  uint16_t word;
  int host_size=strlen(host);
  int label_size;
  char *label, *ptr, *end;
  static char *standard = "\x01\x00\x00\x01\x00\x00\x00\x00\x00\x00";
  simple_buffer_reset(out);
  simple_buffer_write_short(out, (uint16_t) id);
  simple_buffer_write(out,standard,10);
  label=host;
  end=label+host_size;
  label_size=0;

  /* make the QNAME */

  for(ptr=label;ptr<end;ptr++){
    if (*ptr=='.' && label_size>0){
      simple_buffer_write_char(out, label_size);
      simple_buffer_write(out, label, label_size);
      label_size= 0;
      label=ptr+1;
      continue;
    }
    label_size++;
  }
  
  simple_buffer_write_char(out, 0); /* end of QNAME */
  simple_buffer_write(out, qtype, 2);
  simple_buffer_write(out, CLASS_IN, 2);

}

