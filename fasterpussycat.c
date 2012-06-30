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
#include "util.h"
#include "db.h"
#include "engine.h"
#include "bayes.h"
#include "id3.h"


#ifdef DEBUG_ALLOCATOR
struct __AD_trk_obj* __AD_trk[ALLOC_BUCKETS];
u32 __AD_trk_cnt[ALLOC_BUCKETS];
#endif /* DEBUG_ALLOCATOR */

#define MODE_ATTACK 1
#define MODE_ADD_URL 2
#define MODE_ADD_TRIGGER 3
#define MODE_HELP 4
#define TRIGGER 5
#define FEATURE 6
#define URL 7
#define FLAGS 8
#define STATISTICS 9
#define MODE_ANALYZE 10

void usage(){
printf(
"Usage:  fasterpussycat [OPTIONS] HOST1 HOST2 HOST3 ...\n"
"        fasterpussycat --add-url --url URL [--flags flags]\n"
"        fasterpussycat --add-trigger --trigger TRIGGER --feature FEATURE\n"
"\n"
"Options:\n"
"\n"
"Attack mode:\n"
"\n"
"\n"
"  -n, --max-hosts=N             maximum simultaneous hosts to scan\n"
"  -c, --max-conn=N              maximum connections per a host\n"
"  -r, --max-requests=N          check only the top N requests per a host (0 to\n"
"                                   scan all urls in the database)\n"
"  -P  --progress=N              print scan progress every N seconds, (0 to\n"
"                                   disable)\n"
"  -B  --browser={metal|minimal|firefox|explorer|iphone}\n"     
"                                emulate browser headers. The following\n"
"                                   options are supported: metal (bare metal\n"
"                                   request), minimal, firefox, explorer, iphone.\n"
"                                   NOTE: Use \"metal\" or \"minimal\" when\n"
"                                   upload bandwith is constrained\n"
"  -T  --train[=M]               enable training of all tests, or only tests with\n"
"                                   a sample size of at most M\n"
"      --skip-dir-check          skips check to see if we can identify dirs\n"
"      --skip-cgi-bin-check      skips check for cgi-bin dir\n"
"      --force-save              save results even though training mode is off\n"
"\n"
"Database:\n"
"\n"
"      --add-trigger             adds a trigger to the database, requires the\n"
"                                   feature and trigger options\n"
"      --add-url                 adds or updates a url test in the database,\n"
"                                   requires the url and optional flag option.\n"
"      --url=PATH                the url path to manipulate NOTE: no host or\n"
"                                  protocol sections i.e. \"/index.php\"\n"
"      --statistics              print statistics\n"
"      --analyze                 build decision tree\n"
"\n"
"Other:\n"
"  -h  --help                    this help\n"
);
}

/* Ctrl-C handler... */

static u8 stop_soon;
static u32 progress=10;
static void ctrlc_handler(int sig) {
  stop_soon = 1;
}

u8 print_body(struct http_request *req, struct http_response *rep){
  printf("%s",rep->payload);
  return 0;
}

void do_scan(){
  load_features();
  load_tests();
  load_aho_corasick_triggers();
  struct timeval tv;
  gettimeofday(&tv, NULL);
  u64 st_time;
  u32 last_req=0;
  u64 last_time;
  st_time = tv.tv_sec * 1000LL + tv.tv_usec / 1000;
  last_time=st_time;
  while ((next_from_queue() && !stop_soon)) {


    u64 end_time;
    u64 run_time;
    struct timeval tv_tmp;

    gettimeofday(&tv_tmp, NULL);
    end_time = tv_tmp.tv_sec * 1000LL + tv_tmp.tv_usec / 1000;

    run_time = end_time - st_time;
    if(progress && ((end_time-last_time)>(1000*progress))){
      req_sec = ((req_count - queue_cur)-last_req) * 1000.0 / ((end_time-last_time) + 1);
      last_time=end_time;
      last_req=(req_count - queue_cur);
      http_stats(st_time);
    }
  }
  if(train || force_save) save_all();


}



void parse_opts(int argc, char** argv){
  int longIndex;
  int mode=MODE_ATTACK;
  int i;
  unsigned int f;
  int t=0;
  char *url=NULL;
  char *trigger=NULL;
  char *feature=NULL;
  char *description="";
  unsigned int flags=0;
  struct option long_options[] = {        /* long options array. Items are all caSe SensiTivE! */
    { "add-url", no_argument, &mode, MODE_ADD_URL   }, 
    { "add-trigger", no_argument, &mode, MODE_ADD_TRIGGER},
    { "trigger", required_argument,NULL, TRIGGER},
    { "max-hosts", required_argument,NULL, 'n'},
    { "max-connections", required_argument,NULL, 'c'},
    { "max-requests", required_argument,NULL, 'r'},
    { "progress", required_argument, NULL, 'P' },
    { "statistics", no_argument, NULL, STATISTICS },
    { "browser", required_argument, NULL, 'B' },
    { "train",  optional_argument, NULL, 'T' },
    { "feature", required_argument,NULL, FEATURE},
    { "url", required_argument, NULL, URL },
    { "flags", required_argument, NULL, FLAGS },
    { "help", required_argument, NULL, 'h' },
    { "skip-dir-check", no_argument, &check_dir, 0},
    { "skip-cgi-bin-check", no_argument, &check_cgi_bin, 0},
    { "force-save", no_argument, &force_save, 1},
    { "analyze", no_argument, &mode, MODE_ANALYZE},
    { 0,    0,    0,    0   }       /* terminating -0 item */
  };
  int opt;
  while((opt=getopt_long( argc, argv, "-n:P:c:B:h:r:T::", long_options, &longIndex ))!=-1){
    switch(opt){
      case 1:
        add_target((unsigned char *) optarg);
        t++;
        break;
      case 'B':
        if (!strcasecmp("metal",optarg)) browser_type=BROWSER_METAL;
        else if (!strcasecmp("minimal",optarg)) browser_type=BROWSER_FAST;
        else if (!strcasecmp("firefox",optarg)) browser_type=BROWSER_FFOX;
        else if (!strcasecmp("explorer",optarg)) browser_type=BROWSER_MSIE;
        else if (!strcasecmp("iphone",optarg)) browser_type=BROWSER_PHONE;
        else {
          printf("ERROR: Browser type '%s' is not supported.\n",optarg);
          exit(1);
        }
        break;
      case 'h':
        usage();
        exit(0);
      case URL:
        url=optarg;
        break;
      case 'c':
        max_conn_host=atoi(optarg);
        break;
      case 'r':
        max_requests=atoi(optarg);
        break;
      case 'P':
        progress=atoi(optarg);
        break;
      case STATISTICS:
        load_tests();
        load_features();
        show_feature_predictive_values();
        exit(0);
        break;
      case 'T':
        train=1;
        if(optarg) max_train_count=atoi(optarg);
        break;
      case 'n':
        max_hosts=atoi(optarg);
        break;
      case FEATURE:
        feature=optarg;
        break;
      case TRIGGER:
        trigger=optarg;
        break;
      case FLAGS:
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
        break;
    }
  }

  if(train){
    max_requests=0;
    check_dir=1;
    check_cgi_bin=1;
  }

  switch(mode){
    case MODE_ADD_URL:
      add_or_update_url(url, description, flags);
      exit(0);
      break;
    case MODE_ATTACK:
      if(!t){
        usage();
        exit(0);
      }
      max_connections=max_hosts * max_conn_host;
      do_scan();
      exit(0);
      break;
    case MODE_ADD_TRIGGER:
      add_aho_corasick_trigger(trigger, feature);
      exit(0);
      break;
    case MODE_ANALYZE:
      load_tests();
      load_features();
      info("building id3 decision tree...");
      build_tree(NULL);
      exit(0);
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
  setup_bayes();
  parse_opts(argc, argv);
  fflush(0);

  EVP_cleanup();
  CRYPTO_cleanup_all_ex_data();

  return 0;

}
