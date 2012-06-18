#include "utlist.h"
#include <gmp.h>
#include "http_client.h"
#include "db.h"
#include "engine.h"

static mpq_t feat_given_succ_ratio;
static mpq_t feat_given_fail_ratio; 
static mpq_t neg_posterior; 
static mpq_t evidence;
static mpq_t one;
static mpq_t zero;
static mpq_t posterior;

void setup_bayes(){
  mpq_init(evidence);
  mpq_init(neg_posterior);
  mpq_init(one);
  mpq_init(zero);
  mpq_init(posterior);
  mpq_init(feat_given_fail_ratio);
  mpq_init(feat_given_succ_ratio);
  mpq_set_ui(one,1,1);

}



unsigned int get_test_successes(struct url_test *test, unsigned int codes){
  unsigned int successes=0;
  if(codes & CODE_200) successes+=test->code_200;
  if(codes & CODE_301) successes+=test->code_301;
  if(codes & CODE_302) successes+=test->code_302;
  if(codes & CODE_401) successes+=test->code_401;
  if(codes & CODE_403) successes+=test->code_403;
  if(codes & CODE_500) successes+=test->code_500;
  if(codes & CODE_OTHER) successes+=test->code_other;
  return successes;
}

unsigned int get_codes_for_test(struct url_test *test){
  unsigned int codes=CODE_200;
  if(test->flags & F_DIRECTORY){
    codes=codes | CODE_401;
    codes=codes | CODE_403;
  }
  return codes;

}

unsigned int get_ftr_successes(struct feature_test_result *test, unsigned int codes){
  unsigned int successes=0;
  if(codes & CODE_200) successes+=test->code_200;
  if(codes & CODE_301) successes+=test->code_301;
  if(codes & CODE_302) successes+=test->code_302;
  if(codes & CODE_401) successes+=test->code_401;
  if(codes & CODE_403) successes+=test->code_403;
  if(codes & CODE_500) successes+=test->code_500;
  if(codes & CODE_OTHER) successes+=test->code_other;
  return successes;
}

void calculate_new_posterior(mpq_t *posterior, struct url_test *test, struct feature_test_result *ftr,unsigned int codes){
  if(ftr->count==0) return;
  mpq_sub(neg_posterior,one,*posterior);
  unsigned int url_test_success=get_test_successes(test,codes);
  unsigned int ftr_success=get_ftr_successes(ftr,codes);
  unsigned int url_test_fail=test->count-url_test_success;
  unsigned int ftr_fail=ftr->count-ftr_success;

  if(url_test_success!=0){
    mpq_set_ui(feat_given_succ_ratio, ftr_success, url_test_success);
    mpq_canonicalize(feat_given_succ_ratio);
    mpq_mul(feat_given_succ_ratio,*posterior, feat_given_succ_ratio);
  }
  else{
    mpq_set_ui(feat_given_succ_ratio,0,1);
  }
  if(url_test_fail!=0){
    mpq_set_ui(feat_given_fail_ratio, ftr_fail, url_test_fail);
    mpq_canonicalize(feat_given_fail_ratio);
    mpq_mul(feat_given_fail_ratio,neg_posterior, feat_given_fail_ratio);
  }
  else{
    mpq_set_ui(feat_given_fail_ratio,0,1);
  }
  mpq_add(evidence,feat_given_fail_ratio,feat_given_succ_ratio);
  mpq_div(*posterior,feat_given_succ_ratio,evidence);
}

double get_test_probability(struct url_test *test, struct target *t){
  int codes=get_codes_for_test(test);
  struct feature_node *fn;
  struct feature *f;
  struct feature_test_result *ftr;
  mpq_set_ui(posterior,get_test_successes(test,codes),test->count);
  mpq_canonicalize(posterior);
  DL_FOREACH(t->features,fn){
    f=fn->data;
    ftr=find_or_create_ftr(test->id,f->id);
    calculate_new_posterior(&posterior,test,ftr,codes);
  }
  return mpq_get_d(posterior);
}





