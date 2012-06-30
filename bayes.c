#include <math.h>
#include <stdio.h>
#include "utlist.h"
#include <gmp.h>
#include "http_client.h"
#include "db.h"
#include "engine.h"
#include "bayes.h"

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

double positive_predictive_value(struct url_test *test, struct feature *f){
  struct feature_test_result *ftr=find_or_create_ftr(test->id,f->id);
  return (double) ftr->success / ftr->count;
  // return ((double) ftr->success / test->success) / (((double) ftr->count / test->count) + ((ftr->count - ftr->success) / (test->count - test->success)));
}



double negative_predictive_value(struct url_test *test, struct feature *f){
  struct feature_test_result *ftr=find_or_create_ftr(test->id,f->id);
  return ((double) (test->count-test->success)-(ftr->count-ftr->success))/(test->count-ftr->count);
  // return (double) (1-((ftr->count - ftr->success) / (test->count - test->success))) / ( (1-((ftr->count - ftr->success) / (test->count - test->success))) + (1-((double) ftr->success / test->success)));
}




double information(double prob){
  double p1;
  double p2;
  if(prob==0.0 || prob==1.0){
    return 0;
  } 
  return (prob*(-log2(prob)))+((1-prob)*(-log2(1-prob)));
}

double information_gain(struct test_score *score, struct feature *f){
  struct url_test *test=score->test;
  struct feature_test_result *ftr=find_or_create_ftr(test->id,f->id);
  if(test->count==0) return 0.0;
  
  double pF=(((double) ftr->success/(double) test->success)*score->score)+((((double) ftr->count-ftr->success)/((double) test->count-test->success))*(1-score->score));
  double base_info=information(score->score);
  double not_f_count=test->count-ftr->count;
  double false_count=(test->count-test->success)-(ftr->count-ftr->success);
  double entropy_evidence;
  if(ftr->count!=0) entropy_evidence=information(ftr->success/ftr->count);
  else entropy_evidence=0.0;
  double entropy_not_evidence=information(false_count/not_f_count);
  double after_entropy=(pF*entropy_evidence)+((1-pF)*entropy_not_evidence);;
  //return base_info-after;
  // printf("%f %f %f\n",pa,pb,log(pa/pb));
  return base_info-after_entropy;

}



double calculate_new_posterior(double posterior, unsigned int hypothesis_true, unsigned int sample_size, unsigned int evidence_and_hypothesis_true, unsigned int evidence_count, int evidence_occurred ){
  if(evidence_count==0) return posterior;

  unsigned int hypothesis_false=sample_size-hypothesis_true;
  unsigned int evidence_and_hypothesis_false=evidence_count-evidence_and_hypothesis_true;
  double prob_evidence_given_hypothesis_true;
  double prob_evidence_given_hypothesis_false;
  double evidence;

  if(hypothesis_true!=0){
    prob_evidence_given_hypothesis_true = (double) evidence_and_hypothesis_true / (double) hypothesis_true;
    if(!evidence_occurred){
      prob_evidence_given_hypothesis_true=1-prob_evidence_given_hypothesis_true;
    }
  }
  else{
    prob_evidence_given_hypothesis_true=0;  
  }
  if(hypothesis_false!=0){
    prob_evidence_given_hypothesis_false = (double) evidence_and_hypothesis_false / (double) hypothesis_false;
    if(!evidence_occurred){
      prob_evidence_given_hypothesis_false=1-prob_evidence_given_hypothesis_false;
    }
  }
  else{
    prob_evidence_given_hypothesis_false=0;
  }
  evidence=(posterior*prob_evidence_given_hypothesis_true)+((1-posterior) * prob_evidence_given_hypothesis_false);
  return (posterior*prob_evidence_given_hypothesis_true)/evidence;
}

void show_feature_predictive_values(){
  struct feature *f;
  struct url_test *test;
  double ppv, npv, m, m2;
  struct test_score score;
  for(f=get_features();f!=NULL;f=f->hh.next){
    if(f->count<50) continue;
    ppv=0;
    npv=0;
    m=0;
    m2=0;
    int count=0;

    for(test=get_tests();test!=NULL;test=test->hh.next){
      if(test->success<10) continue;
      score.score=(double) test->success/test->count;
      score.test=test;
      m2=information_gain(&score,f);
    //  if(isnan(m2)){
    //    m2=0;
    //  }
      m+=m2;
      ppv+=positive_predictive_value(test,f);
      npv+=negative_predictive_value(test,f);
      count++;
    }
    printf("%s ppv %f npv %f merit %f\n", f->label, ppv/count, npv/count, m);
  }
}

inline int feature_seen(struct feature *f, struct target *t){
  struct feature *f2;
  struct feature_node *fn;
  DL_FOREACH(t->features,fn){
    f2=fn->data;
    if(f2==f){
      return 1;
    }
  }
  return 0;
}



double expected_change(struct test_score *score, struct feature *f){
  struct url_test *test=score->test;
  struct feature_test_result *ftr=find_or_create_ftr(test->id,f->id);
  
  if(test->count==0) return 0.0;
  
  double pF=(((double) ftr->success/(double) test->success)*score->score)+((((double) ftr->count-ftr->success)/((double) test->count-test->success))*(1-score->score));
  double change_y=fabs(score->score-calculate_new_posterior(score->score,test->success,test->count,ftr->success,ftr->count,1));
  double change_n=fabs(score->score-calculate_new_posterior(score->score,test->success,test->count,ftr->success,ftr->count,0));
  return ((pF*change_y)+((1-pF)*change_n));
  
  
}



double expected_change_ratio(struct test_score *score, struct feature *f){
  struct url_test *test=score->test;
  struct feature_test_result *ftr=find_or_create_ftr(test->id,f->id);
  
  if(test->count==0 || score->score==0) return 0.0;
   
  double pF=(((double) ftr->success/(double) test->success)*score->score)+((((double) ftr->count-ftr->success)/((double) test->count-test->success))*(1-score->score));
  double change_y=fabs(score->score-calculate_new_posterior(score->score,test->success,test->count,ftr->success,ftr->count,1))/score->score;
  double change_n=fabs(score->score-calculate_new_posterior(score->score,test->success,test->count,ftr->success,ftr->count,0))/score->score;
  return ((pF*change_y)+((1-pF)*change_n));
  
  
}

double get_test_probability(struct url_test *test, struct target *t){
  struct feature *f;
  struct feature_test_result *ftr;
  double posterior=(double) test->success / (double) test->count;
  for(f=get_features();f!=NULL;f=f->hh.next){
    if(f->count<200) continue;
    ftr=find_or_create_ftr(test->id,f->id);
    posterior=calculate_new_posterior(posterior,test->success,test->count, ftr->success,ftr->count, feature_seen(f,t));
  }
  return posterior;
}





