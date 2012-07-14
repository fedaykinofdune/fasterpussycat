#include <stdlib.h>   
#include <stddef.h> 
#include <sqlite3.h>
#include "debug.h"
#include "uthash.h"
#include "utlist.h"
#include "types.h"
#include "alloc-inl.h"
#include "string-inl.h"
#include "http_client.h"
#include "db.h"
#include "ac.h"
#include "engine.h"
#include "util.h"


#define INSERT_DIR_LINK_SQL "INSERT INTO dir_links (parent_id, child_id, count, parent_success, child_success, parent_child_success) VALUES (?,?,?,?,?,?)"

#define UPDATE_DIR_LINK_SQL "UPDATE dir_links SET parent_id=?, child_id=?, count=?, parent_success=?, child_success=?, parent_child_success=? WHERE id=?"
#define GET_DIR_LINKS_SQL "SELECT id, parent_id, child_id, count, parent_success, child_success, parent_child_success FROM dir_links"
#define GET_TESTS_SQL "SELECT id, url, success, count, flags FROM url_tests"
#define GET_FEATURE_SELECTIONS_SQL "SELECT url_test_id, feature_id FROM feature_selections"
#define GET_TRIGGERS_SQL "SELECT id, trigger, feature_id FROM aho_corasick_feature_triggers"
#define GET_TEST_BY_URL_SQL "SELECT id, url, success, count, flags FROM url_tests WHERE url=? LIMIT 1"
#define GET_FEATURES_SQL "SELECT id, label, count FROM features"
#define GET_FTR_BY_TEST_AND_FEATURE_SQL "SELECT id, url_test_id, feature_id, success, count FROM feature_test_results WHERE url_test_id=? AND feature_id=?"
#define GET_FTR_BY_FEATURE_SQL "SELECT id, url_test_id, feature_id, success, count FROM feature_test_results WHERE feature_id=?"  
#define INSERT_FEATURE_SQL "INSERT INTO features (label) VALUES (?)" 
#define UPDATE_FEATURE_SQL "UPDATE features SET count=? WHERE id=?" 
#define INSERT_FEATURE_TEST_RESULT_SQL "INSERT INTO feature_test_results (url_test_id,feature_id,success,count) VALUES (?,?,?,?)" 
#define INSERT_TRIGGER_SQL "INSERT OR REPLACE INTO aho_corasick_feature_triggers (feature_id,trigger) VALUES (?,?)" 
#define UPDATE_FEATURE_TEST_RESULT_SQL "UPDATE feature_test_results SET success=?,count=? WHERE id =?" 
#define INSERT_URL_TEST_SQL "INSERT INTO url_tests (url,flags) VALUES (?,?)" 
#define UPDATE_URL_TEST_SQL "UPDATE url_tests SET success=?, count=?, flags=? WHERE id=?" 
#define INSERT_FEATURE_SELECTION_SQL "INSERT INTO feature_selections (url_test_id,feature_test_id) VALUES (?,?)"
#define DELETE_FEATURE_SELECTION_SQL "DELETE FROM feature_selections WHERE url_test_id=?"

static AC_STRUCT *aho_corasick;
static struct url_test *test_map=NULL;
static struct feature_test_result *result_map=NULL;
static struct feature *feature_map=NULL;
static struct dir_link *dir_link_map=NULL;
static struct feature *feature_map_by_id=NULL;
static struct trigger *trigger_map=NULL;
static sqlite3 *db;

static sqlite3_stmt *get_ftr_by_feature_stmt;
static sqlite3_stmt *get_tests_stmt;
static sqlite3_stmt *get_test_by_url_stmt;
static sqlite3_stmt *get_features_stmt;
static sqlite3_stmt *get_triggers_stmt;

static sqlite3_stmt *insert_feature_stmt;


static sqlite3_stmt *get_dir_links_stmt;
static sqlite3_stmt *insert_dir_link_stmt;
static sqlite3_stmt *update_dir_link_stmt;

static sqlite3_stmt *insert_trigger_stmt;
static sqlite3_stmt *update_feature_stmt;
static sqlite3_stmt *insert_ftr_stmt;
static sqlite3_stmt *update_ftr_stmt;
static sqlite3_stmt *insert_url_test_stmt;
static sqlite3_stmt *update_url_test_stmt;

static sqlite3_stmt *insert_feature_selection_stmt;

static sqlite3_stmt *get_feature_selections_stmt;
static sqlite3_stmt *delete_feature_selection_stmt;

int open_database(){
  int rc=sqlite3_open("fasterpussycat.sqlite3", &db);
  if(rc){

    sqlite3_close(db);
    FATAL("bad db :(");
  }



  sqlite3_prepare_v2(db, INSERT_DIR_LINK_SQL, -1, &insert_dir_link_stmt,NULL);
  sqlite3_prepare_v2(db, UPDATE_DIR_LINK_SQL, -1, &update_dir_link_stmt,NULL);
  sqlite3_prepare_v2(db, GET_DIR_LINKS_SQL, -1, &get_dir_links_stmt,NULL);
  sqlite3_prepare_v2(db, INSERT_FEATURE_SELECTION_SQL, -1, &insert_feature_selection_stmt,NULL);
  sqlite3_prepare_v2(db, DELETE_FEATURE_SELECTION_SQL, -1, &delete_feature_selection_stmt,NULL);
  sqlite3_prepare_v2(db, GET_FEATURE_SELECTIONS_SQL, -1, &get_feature_selections_stmt,NULL);
  sqlite3_prepare_v2(db, GET_TESTS_SQL, -1, &get_tests_stmt,NULL);
  sqlite3_prepare_v2(db, GET_TEST_BY_URL_SQL, -1, &get_test_by_url_stmt,NULL);
  sqlite3_prepare_v2(db, GET_FTR_BY_FEATURE_SQL, -1, &get_ftr_by_feature_stmt,NULL);
  sqlite3_prepare_v2(db, GET_FEATURES_SQL, -1, &get_features_stmt,NULL);
  sqlite3_prepare_v2(db, INSERT_FEATURE_SQL, -1, &insert_feature_stmt,NULL);
  sqlite3_prepare_v2(db, INSERT_TRIGGER_SQL, -1, &insert_trigger_stmt,NULL);
  sqlite3_prepare_v2(db, GET_TRIGGERS_SQL, -1,   &get_triggers_stmt,NULL);
  sqlite3_prepare_v2(db, UPDATE_FEATURE_SQL, -1, &update_feature_stmt,NULL);
  sqlite3_prepare_v2(db, INSERT_FEATURE_TEST_RESULT_SQL, -1, &insert_ftr_stmt,NULL);
  sqlite3_prepare_v2(db, UPDATE_FEATURE_TEST_RESULT_SQL, -1, &update_ftr_stmt,NULL);
  sqlite3_prepare_v2(db, INSERT_URL_TEST_SQL, -1, &insert_url_test_stmt,NULL);
  sqlite3_prepare_v2(db, UPDATE_URL_TEST_SQL, -1, &update_url_test_stmt,NULL);
  return 0;
}


void load_feature_selections(){
  info("loading feature selections...");
  sqlite3_reset(get_feature_selections_stmt);
  struct feature_selection *fs;
  struct feature *f;
  struct url_test *t;
  int f_id,t_id;
  while (sqlite3_step(get_feature_selections_stmt) == SQLITE_ROW){
    fs=malloc(sizeof(struct feature_selection));
    t_id=sqlite3_column_int(get_feature_selections_stmt,0);
    f_id=sqlite3_column_int(get_feature_selections_stmt,1);
    HASH_FIND_INT(test_map, &t_id,t);
    HASH_FIND_INT(feature_map_by_id,&f_id,f);
    fs->feature=f;
    fs->next=t->feature_selections;
    t->feature_selections=fs;
  }
}

int test_size_sort(struct url_test *a, struct url_test *b){
  if(strlen(a->url)>strlen(b->url)){
    return -1;
  }
  if(strlen(a->url)==strlen(b->url)){
    return 0;
  }
  return 1;
}

void create_dir_links(){
  load_tests();
  HASH_SORT(test_map,test_size_sort);
  struct url_test *parent;
  struct url_test *child;
  struct dir_link *link=calloc(sizeof(struct dir_link),1);
  for(parent=test_map;parent!=NULL;parent=parent->hh.next){
    if(!(parent->flags & F_DIRECTORY)) continue;
    for(child=parent->hh.prev;child!=NULL;child=child->hh.prev){
        if(child->parent || strstr(child->url,parent->url)!=child->url) continue;
        link->id=-1;
        link->parent_id=parent->id;
        link->child_id=child->id;
        child->parent=link;
        save_dir_link(link);
      }
  }

}

void update_feature_selection(struct url_test *test){
  struct feature_selection *fs;

  sqlite3_exec(db, "BEGIN", 0, 0, 0);
  sqlite3_reset(delete_feature_selection_stmt);
  sqlite3_bind_int(delete_feature_selection_stmt,1,test->id);
  
  if(sqlite3_step(delete_feature_selection_stmt)!=SQLITE_DONE){
    fatal("SOME KIND OF FAIL IN FEATURE SELECTION DELETE '%s' %s\n", test->url, sqlite3_errmsg(db));
  }
  for(fs=test->feature_selections;fs!=NULL;fs=fs->next){
    sqlite3_reset(insert_feature_selection_stmt);
    sqlite3_bind_int(insert_feature_selection_stmt,1,test->id);
    sqlite3_bind_int(insert_feature_selection_stmt,2,fs->feature->id);
    if(sqlite3_step(delete_feature_selection_stmt)!=SQLITE_DONE){
      fatal("SOME KIND OF FAIL IN FEATURE SELECTION INSERTION '%s' '%s' %s\n", test->url, fs->feature->label, sqlite3_errmsg(db));
    }
  }

  sqlite3_exec(db, "COMMIT", 0, 0, 0);
  
}

void load_aho_corasick_triggers(){
  aho_corasick=ac_alloc();  
  sqlite3_reset(get_triggers_stmt);
  while (sqlite3_step(get_triggers_stmt) == SQLITE_ROW){
    struct trigger *t=ck_alloc(sizeof(struct trigger));
    t->id=sqlite3_column_int(get_triggers_stmt,0);
    t->trigger=(char *) strdup((const char *) sqlite3_column_text(get_triggers_stmt,1));
    t->feature_id=sqlite3_column_int(get_triggers_stmt,2);
    HASH_ADD_INT( trigger_map, id, t );
    ac_add_string(aho_corasick, t->trigger,strlen(t->trigger),t->id);
  }
  ac_prep(aho_corasick);
}


struct url_test *get_tests(){
  return test_map;
}

struct feature *get_features(){
  return feature_map;
}

void add_features_from_triggers(struct http_response *rep, struct target *t){
  int meh;
  int id;
  struct trigger *trig;
  struct feature *f;
  ac_search_init(aho_corasick,(char *) rep->payload,rep->pay_len);
  while(ac_search(aho_corasick, &meh,&id)){
    HASH_FIND_INT(trigger_map, &id, trig);
    HASH_FIND_INT(feature_map_by_id, &trig->feature_id,f);
    if(f==NULL){
      fatal("feature is null, feature_id=%d\n",trig->feature_id);
    }
    add_feature_to_target(f,t);
  }
}

void add_aho_corasick_trigger(char *trigger, char *feature){
  struct feature *f;
  open_database();
  
  sqlite3_exec(db, "BEGIN", 0, 0, 0);
  load_features();

  f=find_or_create_feature_by_label(feature);
  if(f->id==-1){
    save_feature(f);
  }
  sqlite3_reset(insert_trigger_stmt);
  sqlite3_bind_int(insert_trigger_stmt,1,f->id);
  sqlite3_bind_text(insert_trigger_stmt,2,trigger, -1, SQLITE_TRANSIENT);

  if(sqlite3_step(insert_trigger_stmt)!=SQLITE_DONE){
      fatal("SOME KIND OF FAIL IN TRIGGER INSERT: %s\n",sqlite3_errmsg(db));
  }

  sqlite3_exec(db, "COMMIT", 0, 0, 0);
  info("saved trigger!\n");
}

struct feature *find_or_create_feature_by_label(const char *label){
  struct feature *result;
  int i;
  char *l_label=strdup(label);
  for (i = 0; l_label[i]; i++){
    l_label[i]=tolower(l_label[i]);
  }
  HASH_FIND(hhl, feature_map, l_label, strlen(l_label), result);
  if(result!=NULL){
    free(l_label);
    return result;
  }

  result=ck_alloc(sizeof(struct feature));
  result->id=-1;
  result->label=l_label;
  result->dirty=1;
  save_feature(result);
  HASH_ADD_KEYPTR( hhl, feature_map, result->label, strlen(result->label), result);

  HASH_ADD_INT( feature_map_by_id, id, result);
  return result; 
}

void save_feature(struct feature *f){
  if(f->id==-1){
    sqlite3_reset(insert_feature_stmt);
    sqlite3_bind_text(insert_feature_stmt,1,f->label, -1, SQLITE_TRANSIENT);
    if(sqlite3_step(insert_feature_stmt)!=SQLITE_DONE){
      fatal("SOME KIND OF FAIL IN FEATURE INSERT '%s' %s\n", f->label, sqlite3_errmsg(db));
    }
    f->id=sqlite3_last_insert_rowid(db);
    f->dirty=0;
  }
  else{
    sqlite3_reset(update_feature_stmt);
    sqlite3_bind_int(update_feature_stmt,1,f->count);
    sqlite3_bind_int(update_feature_stmt,2,f->id);
    if(sqlite3_step(update_feature_stmt)!=SQLITE_DONE){
      fatal("SOME KIND OF FAIL IN FEATURE UPDATE %s\n",sqlite3_errmsg(db));
    }
    f->dirty=0;
  }
  
}

void save_all(){
  struct feature *feature_iter; 
  struct url_test *url_test_iter;
  struct feature_test_result *ftr_iter;
  struct dir_link *link_iter;
  info("saving results\n");
  sqlite3_exec(db, "BEGIN", 0, 0, 0);

  for(feature_iter=feature_map; feature_iter!=NULL; feature_iter=feature_iter->hh.next){
    if(feature_iter->dirty){
      save_feature(feature_iter);
    }
  }

  for(url_test_iter=test_map; url_test_iter!=NULL; url_test_iter=url_test_iter->hh.next){
    if(url_test_iter->dirty){
      save_url_test(url_test_iter);
    }
  }

  for(ftr_iter=result_map; ftr_iter!=NULL; ftr_iter=ftr_iter->hh.next){
    if(ftr_iter->dirty){
      save_ftr(ftr_iter);
    }
  }

  for(link_iter=dir_link_map; link_iter!=NULL; link_iter=link_iter->hh.next){
    if(link_iter->dirty){
      save_dir_link(link_iter);
    }
  }
 sqlite3_exec(db, "COMMIT", 0, 0, 0);
}



void save_ftr(struct feature_test_result *f){
  if(f->id==-1){
    sqlite3_reset(insert_ftr_stmt);
    sqlite3_bind_int(insert_ftr_stmt,1,f->url_test_id);
    sqlite3_bind_int(insert_ftr_stmt,2,f->feature_id);
    sqlite3_bind_int(insert_ftr_stmt,3,f->success);
    sqlite3_bind_int(insert_ftr_stmt,4,f->count);
    if(sqlite3_step(insert_ftr_stmt)!=SQLITE_DONE){
      fatal("SOME KIND OF FAIL IN FTR INSERT %s\n",sqlite3_errmsg(db));
    }
    f->id=sqlite3_last_insert_rowid(db);
    f->dirty=0;
  }
  else{
    sqlite3_reset(update_ftr_stmt);

    sqlite3_bind_int(update_ftr_stmt,1,f->success);
    sqlite3_bind_int(update_ftr_stmt,2,f->count);
    sqlite3_bind_int(update_ftr_stmt,3,f->id);
    if(sqlite3_step(update_ftr_stmt)!=SQLITE_DONE){
      fatal("SOME KIND OF FAIL IN FTR UPDATE %s\n",sqlite3_errmsg(db));
    }
    f->dirty=0;
  }
  
}



void save_dir_link(struct dir_link *link){
  if(link->id==-1){
    sqlite3_reset(insert_dir_link_stmt);
    sqlite3_bind_int(insert_dir_link_stmt,1,link->parent_id);
    sqlite3_bind_int(insert_dir_link_stmt,2,link->child_id);
    sqlite3_bind_int(insert_dir_link_stmt,3,link->count);
    sqlite3_bind_int(insert_dir_link_stmt,4,link->parent_success);
    sqlite3_bind_int(insert_dir_link_stmt,5,link->child_success);
    sqlite3_bind_int(insert_dir_link_stmt,6,link->parent_child_success);
    if(sqlite3_step(insert_dir_link_stmt)!=SQLITE_DONE){
       fatal("SOME KIND OF FAIL IN DIR LINK INSERT %s\n",sqlite3_errmsg(db));
    }
    link->id=sqlite3_last_insert_rowid(db);
    link->dirty=0;
  }
  else{

    sqlite3_reset(update_dir_link_stmt);
    sqlite3_bind_int(update_dir_link_stmt,1,link->parent_id);
    sqlite3_bind_int(update_dir_link_stmt,2,link->child_id);
    sqlite3_bind_int(update_dir_link_stmt,3,link->count);
    sqlite3_bind_int(update_dir_link_stmt,4,link->parent_success);
    sqlite3_bind_int(update_dir_link_stmt,5,link->child_success);
    sqlite3_bind_int(update_dir_link_stmt,6,link->parent_child_success);
    sqlite3_bind_int(update_dir_link_stmt,7,link->id);
    if(sqlite3_step(update_dir_link_stmt)!=SQLITE_DONE){
       fatal("SOME KIND OF FAIL IN DIR LINK UPDATE %s\n",sqlite3_errmsg(db));
    }
    link->id=sqlite3_last_insert_rowid(db);
    link->dirty=0;
  }
}


void save_url_test(struct url_test *f){
  int rc;
  if(f->id==-1){
    sqlite3_reset(insert_url_test_stmt);
    sqlite3_bind_text(insert_url_test_stmt,1,f->url,-1,SQLITE_STATIC);
    sqlite3_bind_int(insert_url_test_stmt,2,f->flags);
    if(sqlite3_step(insert_url_test_stmt)!=SQLITE_DONE){
       fatal("SOME KIND OF FAIL IN FTR UPDATE %s\n",sqlite3_errmsg(db));
    }
    f->id=sqlite3_last_insert_rowid(db);
    f->dirty=0;
  }
  else{
    sqlite3_reset(update_url_test_stmt);

    sqlite3_bind_int(update_url_test_stmt,1,f->success);
    sqlite3_bind_int(update_url_test_stmt,2,f->count);
    sqlite3_bind_int(update_url_test_stmt,3, f->flags);
    sqlite3_bind_int(update_url_test_stmt,4,f->id);
    
    if((rc=sqlite3_step(update_url_test_stmt))!=SQLITE_DONE){
      fatal("SOME KIND OF FAIL IN URL TEST UPDATE %s\n",sqlite3_errmsg(db));
    }
    f->dirty=0;
  }
  
}


struct feature_test_result *find_or_create_ftr(int url_test_id, int feature_id){
  struct feature_test_result *result;
  int keylen = offsetof(struct feature_test_result, feature_id) + sizeof(int) - offsetof(struct feature_test_result, url_test_id);
  struct ftr_lookup_key *lookup_key=ck_alloc(sizeof(struct ftr_lookup_key));
  lookup_key->url_test_id=url_test_id;
  lookup_key->feature_id=feature_id;
  HASH_FIND(hh, result_map, &lookup_key->url_test_id, keylen, result);
  ck_free(lookup_key);
  if(result!=NULL){
    return result;
  }
  result=ck_alloc(sizeof(struct feature_test_result));
  result->url_test_id=url_test_id;
  result->feature_id=feature_id;
  result->id=-1;
  result->dirty=1;
  HASH_ADD( hh, result_map, url_test_id, keylen, result );
  return result; 
}

int load_ftr_by_feature_id(int feature_id){
  sqlite3_reset(get_ftr_by_feature_stmt);
  sqlite3_bind_int(get_ftr_by_feature_stmt,1,feature_id);
  while (sqlite3_step(get_ftr_by_feature_stmt) == SQLITE_ROW){
    load_ftr();
  }
  return 0;
}



int load_features(){
  info("loading features...\n");
  sqlite3_reset(get_features_stmt);
  while (sqlite3_step(get_features_stmt) == SQLITE_ROW){
    load_feature();
  }
  return 0;
}


int load_feature(){
  struct feature *feat;
  feat=ck_alloc(sizeof(struct feature));
  feat->id=sqlite3_column_int(get_features_stmt,0);
  feat->label=(char *) strdup((const char *) sqlite3_column_text(get_features_stmt,1));
  feat->count=sqlite3_column_int(get_features_stmt,2);
  HASH_ADD_KEYPTR( hhl, feature_map, feat->label, strlen(feat->label), feat );
  HASH_ADD_INT( feature_map_by_id, id, feat );
  load_ftr_by_feature_id(feat->id);
  return 0;
}

int load_ftr(){
  struct feature_test_result *result;
  int keylen;
  result=ck_alloc(sizeof(struct feature_test_result));
  
  result->id=sqlite3_column_int(get_ftr_by_feature_stmt,0);
  result->url_test_id=sqlite3_column_int(get_ftr_by_feature_stmt,1);
  result->feature_id=sqlite3_column_int(get_ftr_by_feature_stmt,2);
  result->success=sqlite3_column_int(get_ftr_by_feature_stmt,3);
  result->count=sqlite3_column_int(get_ftr_by_feature_stmt,4);
  keylen =   offsetof(struct feature_test_result, feature_id) + sizeof(int) - offsetof(struct feature_test_result, url_test_id);
  HASH_ADD( hh, result_map, url_test_id, keylen, result);
  return 0;
}


struct url_test *load_test(sqlite3_stmt *stmt){
    struct url_test *test;
    test=ck_alloc(sizeof(struct url_test));
    test->id=sqlite3_column_int(stmt,0);
    test->url=(char *) strdup((const char *) sqlite3_column_text(stmt,1));
    test->success=sqlite3_column_int(stmt,2);
    test->count=sqlite3_column_int(stmt,3);
    test->parent=NULL;
    test->children=NULL;
    test->flags=(unsigned int) sqlite3_column_int(stmt,4);
    HASH_ADD_INT( test_map, id, test );
    return test;
}

void add_or_update_url(char *url, char *description, unsigned int flags){
  struct url_test *test,*tmp,*lng=NULL;
  int new=0;
  if(url[strlen(url)-1]=='/') flags|=F_DIRECTORY;
  if(strstr(url, "/cgi-bin/")) flags|=F_CGI;
  if(url[0]!='/'){
    char *new=malloc(strlen(url)+2);
    strcpy(&new[1],url);
    new[0]='/';
    url=new;
  }
  sqlite3_reset(get_test_by_url_stmt);
  sqlite3_bind_text(get_test_by_url_stmt,1,url,-1,SQLITE_STATIC);
  if(sqlite3_step(get_test_by_url_stmt) == SQLITE_ROW){
    test=load_test(get_test_by_url_stmt);
  }
  else{
    test=ck_alloc(sizeof(struct url_test));
    test->dirty=1;
    test->id=-1;
    new=1;
  }
  test->url=url;
  test->description=description;
  test->flags=flags;
  save_url_test(test);
  info("saved url!\n");
  if(new){
    load_tests();
    if(!(flags & F_DIRECTORY)){
      int longest=0;
      for(tmp=get_tests();tmp;tmp=tmp->hh.next){
        if((test->flags & F_DIRECTORY) && strstr(test->url,tmp->url)==test->url && strlen(tmp->url)>longest){
          lng=tmp;
          longest=strlen(tmp->url);
        }
      }
      if(longest>0){
        struct dir_link *link=calloc(sizeof(struct dir_link),1);
        link->child_id=test->id;
        link->parent_id=lng->id;
        link->id=-1;
        link->dirty=-1;
        save_dir_link(link);
      }

    }
  }
}

int load_tests(){
  info("loading tests...");
  while (sqlite3_step(get_tests_stmt) == SQLITE_ROW){
    load_test(get_tests_stmt);
  }
  info("loading dir links...");
  while (sqlite3_step(get_dir_links_stmt) == SQLITE_ROW){
     struct dir_link *link=calloc(sizeof(struct dir_link),1);
     link->id=sqlite3_column_int(get_dir_links_stmt,0);
     link->parent_id=sqlite3_column_int(get_dir_links_stmt,1);
     link->child_id=sqlite3_column_int(get_dir_links_stmt,2);
     link->count=sqlite3_column_int(get_dir_links_stmt,3);
     link->parent_success=sqlite3_column_int(get_dir_links_stmt,4);
     link->child_success=sqlite3_column_int(get_dir_links_stmt,5);
     link->parent_child_success=sqlite3_column_int(get_dir_links_stmt,6);
     
     HASH_ADD_INT( dir_link_map, id, link );
     
     HASH_FIND_INT( test_map, &link->parent_id, link->parent );
     HASH_FIND_INT( test_map, &link->child_id, link->child );
     link->child->parent=link;
     LL_APPEND( link->parent->children, link );
  }
  return 0;
}


