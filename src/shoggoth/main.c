#include <signal.h>
#include <getopt.h>
#include "global_options.h"
#include "zeromq.h"
#include "connection_pool.h"

void parse_opts(int argc, char *argv[]){
  int c;
  while(1){
    static struct option long_options[] = {
      {"connections",  required_argument, 0, 'c'},
      {"hosts",  required_argument, 0, 'h'},
      {0,0,0,0}
    };
    int option_index = 0;
    c = getopt_long (argc, argv, "c:h:", long_options, &option_index);
    if(c==-1) break;
    switch(c){
      case 0:
        /* If this option set a flag, do nothing else now. */
        if (long_options[option_index].flag != 0)
          break;
        if (optarg)
        break;
      case 'h':
        opt.max_endpoints=atoi(optarg);
        break;
      case 'c':
        opt.max_conn_per_endpoint=atoi(optarg);
        break;
     }
      

    }
  if (optind < argc) opt.bind=argv[optind];
  
}



void initialize_server(){
  signal(SIGPIPE, SIG_IGN);
  messaging_init();
  connection_init();
  connection_pool_initialize();
  
}

int main(int argc, char *argv[]){
  parse_opts(argc, argv);
  initialize();
  connection_pool_main_loop();
}
