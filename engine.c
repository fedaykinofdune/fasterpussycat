#include <stdio.h>
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
#include "post.h"

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

void output_result(struct http_request *req, struct http_response *res){
  struct annotation *a;
  char line[80];
  int size=0; 
  if(GET_HDR((unsigned char *) "content-length", &res->hdr)) size=atoi((char *) GET_HDR((unsigned char *) "content-length", &res->hdr));
  else size=res->pay_len;


  snprintf(line, 80, "[%d] http://%s%s",res->code,req->t->host,req->test->url);
  printf("%-75s %6d %-16s",line, size, res->header_mime);
  for(a=res->annotations;a;a=a->next){
    printf(" [%s",a->key);
    if(a->value) printf("=%s", a->value);
    printf("]");
  }
  printf("\n");
  fflush(stdout);

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

void process_features(struct http_response *res, struct target *t){
  unsigned char *server=GET_HDR((unsigned char *) "server",&res->hdr);
  unsigned char *powered=GET_HDR((unsigned char *) "x-powered-by",&res->hdr);
  char *label;
  char *f_str;
  int i, size, server_l;
  if(!server){
    return;
  }
  server_l=strlen((char *) server);
  size=server_l+1;
  if(powered) size+=strlen((char *) powered)+1;
  f_str=ck_alloc(size);
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
  ck_free(f_str);
}


void dir_update_score(struct req_pointer *pointer, double f){
  struct dir_link_res *as_parent;
  struct http_request *r=pointer->req;
  struct req_pointer *ptr2;
  if(f==1.0) r->score=calculate_new_posterior(r->score,r->link->child_success,r->link->count,r->link->parent_child_success,r->link->parent_success,1);
  else if(f==0.0) r->score=calculate_new_posterior(r->score,r->link->child_success,r->link->count,r->link->parent_child_success,r->link->parent_success,0);
  else {
    double s1=f*calculate_new_posterior(r->score,r->link->child_success,r->link->count,r->link->parent_child_success,r->link->parent_success,1);
    double s2=(1-f)*calculate_new_posterior(r->score,r->link->child_success,r->link->count,r->link->parent_child_success,r->link->parent_success,0);
    r->score=s1+s2;
    if(r->test->parent){
      HASH_FIND_INT(r->t->link_map,&r->test->id,as_parent);    
      if(r->test->children){      
        for(ptr2=as_parent->children;ptr2!=NULL;ptr2=ptr2->next){
          dir_update_score(ptr2, r->score);
        }
      }
    }
  }
}

u8 process_test_result(struct http_request *req, struct http_response *res){
  struct url_test *test=req->test;;
  struct feature_node *f;
  struct feature_test_result *ftr;
  struct dir_link_res *as_parent=NULL;
  struct req_pointer *pointer=NULL;
  int success=0;
  int rc=0;
  rc=is_404(req->t->detect_404,req,res);
  if(rc==DETECT_UNKNOWN){
    if(req->pointer) req->pointer->done=1;
    return 0;
  }
  if(!test) fatal("We should never be here without a test");
  success=(rc==DETECT_SUCCESS);

  /* parent dir */

  if(test->children){
    HASH_FIND_INT(req->t->link_map,&test->id,as_parent);
    as_parent->tested=1;
    if(success) as_parent->success=1;
    
    for(pointer=as_parent->children;pointer!=NULL;pointer=pointer->next){
      maybe_update_dir_link(pointer);
      if(!train && !pointer->done){
        dir_update_score(pointer,(double) success);
      }
    }
    if(!train) sort_host_queue(req->t->host);
  }

  /* child */

  if(test->parent){
    pointer=req->pointer;
    pointer->tested=1;
    pointer->success=success;
    if(pointer==NULL) fatal("req pointer is null!");
    maybe_update_dir_link(pointer);
    pointer->done=1;
  }


  test->count++;
  test->dirty=1;
  
  if(success){
      test->success++;
      run_post_rules(req,res);
      output_result(req, res);
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
  t->detect_404=ck_alloc(sizeof(struct detect_404_info));
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


unsigned char process_first_page(struct http_request *req, struct http_response *res){
  info("process first");
  if(res->state==STATE_DNSERR || res->code==0){

    /* hambone detected fail immediately */

    return 0;
  }
  process_features(res,req->t);
  add_features_from_triggers(res,req->t);
  launch_404_probes(req->t);
  return 0;
}

int score_sort(struct test_score *lhs, struct test_score *rhs){
  if (lhs->score > rhs->score) return -1;
  else if (lhs->score < rhs->score) return 1;
  return 0;
}


void maybe_update_dir_link(struct req_pointer *pointer){
  if(!(pointer->tested && pointer->parent->tested)) return;
  pointer->link->dirty=1;
  pointer->link->count++;
  if(pointer->success) pointer->link->child_success++;
  if(pointer->parent->success) pointer->link->parent_success++;
  if(pointer->parent->success && pointer->success) pointer->link->parent_child_success++;
}

void enqueue_tests(struct target *t){
  struct url_test *test;
  struct test_score *score;
  struct http_request *request;
  struct dir_link_res *dir_res;
  
  struct dir_link_res *parent_res;
  unsigned char *url_cpy;
  unsigned int queued=0;
  unsigned char *method;
  for(test=get_tests();test!=NULL;test=test->hh.next){
    
    if(train && max_train_count && test->count>max_train_count){
      continue;
    }
    if(test->children){
      dir_res=ck_alloc(sizeof(struct dir_link_res));
      dir_res->parent_id=test->id;
      HASH_ADD_INT(t->link_map, parent_id, dir_res );    
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
    request->score=score->score;
    parent_res=NULL;
    if(score->test->parent){
      struct req_pointer *pointer=ck_alloc(sizeof(struct req_pointer));
      pointer->req=request;
      request->pointer=pointer;
      pointer->link=score->test->parent;
      request->link=score->test->parent;
      HASH_FIND_INT(t->link_map, &score->test->parent->parent_id, parent_res);
      LL_APPEND(parent_res->children,pointer);
      pointer->parent=parent_res;
    }
    async_request(request);
    queued++;
  }
}



