#include "uthash.h"
#include "utlist.h"
#include "http_client.h"
#include "db.h"
#include "engine.h"
#include "bayes.h"
#include "id3.h"


int in_chain(struct feature *f, struct id3_node *root){
  struct id3_node *i;
  for(i=root;i!=NULL;i=i->parent){
    if(i->feature==f) return 1;
  }
  return 0;
}

int depth(struct id3_node *root){
  int depth=0;
  struct id3_node *i;
  for(i=root;i!=NULL;i=i->parent){
    depth++;
  }
  return depth;
}

void print_path(struct id3_node *node){
  if(node->parent) print_path(node->parent);
  switch(node->type){
    case ID3_ROOT:
      printf("[R]");
      break;
    case ID3_EVD:
      printf("[+]");
      break;
    case ID3_NO_EVD:
      printf("[-]");
      break;
  }
  if(node->parent){
    printf("%s ",node->parent->feature->label);
  }
}


struct feature *pick_feature(struct id3_node *root){
  struct feature *f;
  struct feature *top=NULL;
  struct test_score *score;
  double highest=0;
  if(depth(root)>9){
    return NULL;
  }
  for(f=get_features();f!=NULL;f=f->hh.next){
    double gain=0;
    if(f->count<50) continue;
    if(in_chain(f,root)) continue;
    LL_FOREACH(root->scores,score){
      gain+=expected_change(score,f);
    }
    if(gain>highest){
      top=f;
      highest=gain;
    }
  }
  
  return top;
}

struct id3_node *spawn_child(struct id3_node *parent, int evidence){
  struct id3_node *child=calloc(sizeof(struct id3_node),1);
  struct feature_test_result *ftr;
  struct test_score *score;
  struct test_score *copy;
  child->parent=parent;
  child->type=evidence;
  LL_FOREACH(parent->scores,score){
    ftr=find_or_create_ftr(score->test->id,parent->feature->id);
    copy=calloc(sizeof(struct test_score),1);
    copy->score=calculate_new_posterior(score->score,score->test->success, score->test->count, ftr->success, ftr->count, evidence);
   // if(score->score!=copy->score) printf("%s old %f new %f\n",score->test->url, score->score,copy->score);
    copy->test=score->test;
    LL_PREPEND(child->scores,copy); /* reverses the test order but doesn't matter */
  }
  return child;
}


struct id3_node *build_tree(struct id3_node *root){
  struct feature *picked;
  /* if the root node is null, construct a new root node */

  if(root==NULL){
    struct url_test *test;
    struct test_score *score;
    root=calloc(sizeof(struct id3_node),1);
    root->type=ID3_ROOT;
    for(test=get_tests();test!=NULL;test=test->hh.next){
      if(test->success<10) continue;
      score=calloc(sizeof(struct test_score),1);
      score->test=test;
      score->score=(double) test->success / (double) test->count;
      LL_PREPEND(root->scores,score);
    }
    return build_tree(root);
  }
  

  /* otherwise continue */

  picked=pick_feature(root);
  if(picked==NULL){
  //print_path(root->parent);
  //printf("[STOP]\n");
  return root; /* end of tree */
  }
  root->feature=picked;
  print_path(root);
  printf("\n");
  root->child_evidence=spawn_child(root,1);
  root->child_no_evidence=spawn_child(root,0);
  build_tree(root->child_evidence);
  build_tree(root->child_no_evidence);
  return root;




  
}




