/*
   skipfish - main entry point
   ---------------------------

   Author: Michal Zalewski <lcamtuf@google.com>

   Copyright 2009, 2010, 2011 by Google Inc. All Rights Reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>
#include <gmp.h>
#include "types.h"
#include "alloc-inl.h"
#include "string-inl.h"

#include "http_client.h"
#include "db.h"
#include "engine.h"
#include "bayes.h"

#ifdef DEBUG_ALLOCATOR
struct __AD_trk_obj* __AD_trk[ALLOC_BUCKETS];
u32 __AD_trk_cnt[ALLOC_BUCKETS];
#endif /* DEBUG_ALLOCATOR */

#define MODE_DB_ADD 1
#define MODE_ATTACK 2
#define MODE_TRIGGER_ADD 3
#define MODE_HELP 4

/* Ctrl-C handler... */

static u8 stop_soon;

static void ctrlc_handler(int sig) {
  stop_soon = 1;
}

u8 print_body(struct http_request *req, struct http_response *rep){
  printf("%s",rep->payload);
  return 0;
}
/* Main entry point */

void do_scan(){
  load_features();
  load_tests();
  load_aho_corasick_triggers();
  setup_bayes();

  struct timeval tv;
  gettimeofday(&tv, NULL);
  u64 st_time;
  long c=0; 
  st_time = tv.tv_sec * 1000LL + tv.tv_usec / 1000;
  while ((next_from_queue() && !stop_soon)) {


    u64 end_time;
    u64 run_time;
    struct timeval tv_tmp;

    gettimeofday(&tv_tmp, NULL);
    end_time = tv_tmp.tv_sec * 1000LL + tv_tmp.tv_usec / 1000;

    run_time = end_time - st_time;

    req_sec = (req_count - queue_cur / 1.15) * 1000 / (run_time + 1);
    if((c % 256)==0){ 
      http_stats(st_time);
    }
    c++;
  }
  save_all();


}

void parse_opts(int argc, char** argv){
  int longIndex;
  int mode=MODE_ATTACK;
  int i;
  unsigned int f;

  char *url=NULL;
  char *trigger=NULL;
  char *feature=NULL;
  char *description="";
  unsigned int flags=0;
  struct option long_options[] = {        /* long options array. Items are all caSe SensiTivE! */
    { "add-url", no_argument, NULL, 'A'   }, 
    { "add-trigger", no_argument, NULL, 'T'},
    { "trigger", required_argument,NULL, 't'},
    { "max-hosts", required_argument,NULL, 'n'},
    { "max-connections", required_argument,NULL, 'c'},
    { "feature", required_argument,NULL, 'e'},
    { "url", required_argument, NULL, 'u' },
    { "flags", required_argument, NULL, 'f' },
    { 0,    0,    0,    0   }       /* terminating -0 item */
  };
  int opt;
  while((opt=getopt_long( argc, argv, "-ATu:f:n:s:c:", long_options, &longIndex ))!=-1){
    switch(opt){
      case 1:
        add_target((unsigned char *) optarg);
        break;
      case 'A':
        mode=MODE_DB_ADD;
        break;
      case 'T':
        mode=MODE_TRIGGER_ADD;
        printf("trigger add\n");
        break;
      case 'u':
        url=optarg;
        break;
      case 'd':
        description=optarg;
        break;
      case 'c':
        max_conn_host=atoi(optarg);
        break;
      case 'h':
        mode=MODE_HELP;
        break;
      case 'n':
        max_hosts=atoi(optarg);
        break;
      case 'e':
        feature=optarg;
        break;
      case 't':
        trigger=optarg;
        break;
      case 'f':
        for(i=0; i<strlen(optarg); i++){
          f=tolower(optarg[i]);
          switch(f){
            case 'c':
              flags|=F_CRITICAL;
              break;
            case 'i':
              flags|=F_INFO;
              break;
            case 'd':
              flags|=F_DIRECTORY;
              break;
            case 'g':
              flags|=F_CGI;
            default:
              fprintf (stderr, "Unknown flag: '%c'",f ); 
              exit(1);
          }
        }
        printf("flags %d\n",flags);
        break;
    }
  }

  switch(mode){
    case MODE_DB_ADD:
      add_or_update_url(url, description, flags);
      exit(0);
      break;
    case MODE_ATTACK:
      max_connections=max_hosts * max_conn_host;
      do_scan();
      exit(0);
      break;
    case MODE_TRIGGER_ADD:
      add_aho_corasick_trigger(trigger, feature);
      exit(0);
      break;
  }

}

int main(int argc, char** argv) {
  u32 seed;
  struct timeval tv;
  signal(SIGINT, ctrlc_handler);
  signal(SIGPIPE, SIG_IGN);
  SSL_library_init();

  gettimeofday(&tv, NULL);
  seed = tv.tv_usec ^ (tv.tv_sec << 16) ^ getpid();

  srandom(seed);

  if (resp_tmout < rw_tmout) 
    resp_tmout = rw_tmout;

  if (max_connections < max_conn_host)
    max_connections = max_conn_host;
  open_database();
  parse_opts(argc, argv);
  fflush(0);

  EVP_cleanup();
  CRYPTO_cleanup_all_ex_data();

  return 0;

}
