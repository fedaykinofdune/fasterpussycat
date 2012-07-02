#include <gmp.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>
#include "uthash.h"
#include "utlist.h"
#include "http_client.h"
#include "db.h"
#include "engine.h"
#include "bayes.h"
#include "util.h"
#include "match_rule.h"
#include "detect_404.h"

static struct target *targets=NULL;
int check_dir=1;
int check_cgi_bin=1;
int check_404_size=0;
int check_200_size=1;
unsigned int max_requests=0;
int train=0;
int force_save=0;
unsigned int max_train_count=0;
struct target *target_by_host(u8 *host){
  struct target *t;
  HASH_FIND_STR(targets, (char *) host, t);
  return t;
}

void add_feature_label_to_target(const char *label, struct target *t){
  struct feature_node *fn;
  DL_FOREACH(t->features, fn){
    if(!strcasecmp(fn->data->label, label)){
      return;
    }
  }
  fn=(struct feature_node *) ck_alloc(sizeof(struct feature_node));
  fn->data=find_or_create_feature_by_label(label);
  fn->data->count++;
  fn->data->dirty=1;
  DL_APPEND(t->features,fn);
}




void add_feature_to_target(struct feature *f, struct target *t){
  struct feature_node *fn;
  DL_FOREACH(t->features, fn){
    if(!strcasecmp(fn->data->label, f->label)){
      return;
    }
  }
  fn=(struct feature_node *) ck_alloc(sizeof(struct feature_node));
  fn->data=f;
  fn->data->count++;
  fn->data->dirty=1;
  DL_APPEND(t->features,fn);
}

void process_features(struct http_response *rep, struct target *t){
  unsigned char *server=GET_HDR((unsigned char *) "server",&rep->hdr);
  unsigned char *powered=GET_HDR((unsigned char *) "x-powered-by",&rep->hdr);
  char *label;
  char *f_str;
  int i, size, server_l;
  if(!server){
    return;
  }
  server_l=strlen((char *) server);
  size=server_l+1;
  if(powered) size+=strlen((char *) powered)+1;
  f_str=malloc(size);
  strcpy(f_str,(char *) server);
  if(powered){
     f_str[server_l]=' ';
     strcpy(&f_str[server_l+1],(char *) powered);
  }
  for(i=0;f_str[i];i++){
    if(f_str[i]<'A' || f_str[i]>'z'){
      f_str[i]=' ';
    }
  }
  label=strtok((char *) f_str," ");
  while(label!=NULL){
    if(strlen((char *) label)>2){
      add_feature_label_to_target(label, t);
    }
    label=strtok(NULL," ");
  }
  free(f_str);
}

u8 process_test_result(struct http_request *req, struct http_response *rep){
  int code=rep->code;
  struct url_test *test;
  struct feature_node *f;
  struct feature_test_result *ftr;
  int success=0;
  int rc=0;
  rc=is_404(req->t->detect_404,req,rep);
  if(rc==DETECT_UNKNOWN){
    return 0;
  }
  success=(rc==DETECT_SUCCESS);
  test=req->test;
  test->count++;
  test->dirty=1;

  if(success){
      test->success++;
      printf("CODE: %d http://%s%s\n",code,req->t->host,test->url);
  }
  DL_FOREACH(req->t->features,f) {
    ftr=find_or_create_ftr(test->id,f->data->id);
    ftr->count++;
    ftr->dirty=1;
    if(success) ftr->success++;
  }
  return 0;
}

void add_target(u8 *host){
  u32 cur_pos;
  u8 *url;
  struct target *t=(struct target *) ck_alloc(sizeof(struct target));
  struct http_request *first=(struct http_request *) ck_alloc(sizeof(struct http_request));
  t->host=host;
  t->features=NULL;
  t->test_scores=NULL;
  HASH_ADD_KEYPTR( hh, targets, t->host, strlen((char *) t->host), t );
  NEW_STR(url,cur_pos);
  ADD_STR_DATA(url,cur_pos,(unsigned char *) "http://");
  ADD_STR_DATA(url,cur_pos,host);
  ADD_STR_DATA(url,cur_pos,"/");
  parse_url(url,first,NULL);
  t->prototype_request=req_copy(first,0);
  t->prototype_request->method=ck_alloc(5);
  memcpy(t->prototype_request->method,"HEAD",5);
  t->detect_404=calloc(sizeof(struct detect_404_info),1);
  add_default_rules(t->detect_404);
  first->callback=process_first_page;
  first->t=t;
  async_request(first);
}

struct http_request *new_request(struct target *t){
  struct http_request *r=req_copy(t->prototype_request,0);
  r->t=t;
  r->method=ck_strdup(t->prototype_request->method);
  return r;
}



struct http_request *new_request_with_method(struct target *t, unsigned char *method){
  struct http_request *r=req_copy(t->prototype_request,0);
  r->t=t;
  r->method=ck_strdup(method);
  return r;
}


struct http_request *new_request_with_method_and_path(struct target *t, unsigned char *method,unsigned char *path){
  struct http_request *r=req_copy(t->prototype_request,0);
  r->t=t;
  r->method=ck_strdup(method);
  tokenize_path(path,r,0);
  return r;
}


unsigned char process_first_page(struct http_request *req, struct http_response *rep){
  if(rep->state==STATE_DNSERR || rep->code==0){

    /* hambone detected fail immediately */

    return 0;
  }
  process_features(rep,req->t);
  add_features_from_triggers(rep,req->t);
  launch_404_probes(req->t);
  return 0;
}

int score_sort(struct test_score *lhs, struct test_score *rhs){
  if (lhs->score > rhs->score) return -1;
  else if (lhs->score < rhs->score) return 1;
  return 0;
}

void enqueue_tests(struct target *t){
  struct url_test *test;
  struct test_score *score;
  struct http_request *request;
  unsigned char *url_cpy;
  unsigned int queued=0;
  unsigned char *method;
  for(test=get_tests();test!=NULL;test=test->hh.next){
    
    if(train && max_train_count && test->count>max_train_count){
      continue;
    }
    
    
    score=ck_alloc(sizeof(struct test_score));
    score->test=test;
  
    if(train) score->score=rand(); /* if training randomize order to prevent bias */
    else score->score=get_test_probability(test,t);
    LL_PREPEND(t->test_scores,score); 
  }
  LL_SORT(t->test_scores,score_sort);
  LL_FOREACH(t->test_scores,score) {
     
     method=(unsigned char*) recommend_method(t->detect_404, score->test);
     if(method==RECOMMEND_SKIP) continue;
     request=new_request(t);
     url_cpy=ck_alloc(strlen(score->test->url)+1);
     memcpy(url_cpy,score->test->url,strlen(score->test->url)+1);
     
     tokenize_path(url_cpy, request, 0);
     request->test=score->test;
     request->callback=process_test_result;
     request->method=ck_strdup(method);

     async_request(request);
     queued++;
     if(max_requests && queued>=max_requests) break;
  }
}



