%{

#include <stddef.h>
#include "http_client.h"
#include "ast.h"
struct ast_node *oper(int oper, struct ast_node *lhs, struct ast_node *rhs);
struct ast_node *string_value(char *val);
struct ast_node *number_value(int val);
struct ast_node *id(int id);
struct ast_node *parsed_ast;
void yyerror(char *s, ...);
%}

%start ROOT
%union 
{
  int number;
  char *string;
  struct ast_node *node;
}

%token EQ
%token NE
%token GT
%token LT
%token GE
%token LE
%token NOT
%token OR
%token AND
%token LPAREN
%token RPAREN
%token CONTAINS
%token ENDSWITH
%token PATH
%token BODY
%token CODE
%token SIZE
%token HTMLMIME
%token STRING_LITERAL
%token NUMBER_LITERAL
%token METHOD
%type <string> STRING_LITERAL
%type <number> NUMBER_LITERAL
%type <node> ROOT primary_expr expr expr2 string number
%%

ROOT:
  primary_expr { $$=$1; parsed_ast=$1; }
;

primary_expr:
  expr { $$ = $1; }
  | LPAREN expr RPAREN { $$ = $2; }
;

expr:
  expr2 { $$ = $1; }
  | primary_expr OR primary_expr { $$ = oper(AND, $1, $3); }
  | primary_expr AND primary_expr { $$ = oper(OR, $1, $3); }
  | NOT primary_expr { $$ = oper(NOT,$2,NULL); }
;

expr2:
  string CONTAINS string { $$ = oper(CONTAINS,$1,$3);}
  | string ENDSWITH string { $$ = oper(ENDSWITH,$1,$3);}
  | number EQ number { $$ = oper(EQ,$1,$3);}
  | string EQ string { $$ = oper(EQ,$1,$3); }
  | number NE number { $$ = oper(NE,$1,$3);}
  | string NE string { $$ = oper(NE,$1,$3); }
  | number LT number { $$ = oper(LT,$1,$3); }
  | number LE number { $$ = oper(LE,$1,$3); }
  | number GT number { $$ = oper(GT,$1,$3); }
  | number GE number { $$ = oper(GE,$1,$3); }
;

string:
  BODY { $$ = id(BODY); }
  | PATH { $$ = id(PATH); }
  | METHOD { $$ = id(METHOD); }
  | HTMLMIME { $$ = id(HTMLMIME); }
  | STRING_LITERAL { $$=string_value($1); }
;

number:
  CODE { $$ = id(CODE); }
  | SIZE { $$ = id(SIZE); }
  | NUMBER_LITERAL { $$=number_value($1); }
;

%%





struct ast_node *oper(int oper, struct ast_node *lhs, struct ast_node *rhs){
  struct ast_node *node=new_ast_node();
  node->lhs=lhs;
  node->rhs=rhs;
  node->oper=oper;
  return node;
}



struct ast_node *string_value(char *val){
  struct ast_node *node=new_ast_node();
  node->val.text=strdup(val);
  node->val_type=STRING_LITERAL;
  return node;
}



struct ast_node *number_value(int val){
  struct ast_node *node=new_ast_node();
  node->val.number=val;
  node->val_type=NUMBER_LITERAL;
  return node;
}

struct ast_node *id(int id){
  struct ast_node *node=new_ast_node();
  node->id=id;
  return node;
}



char *string_eval(struct ast_node *node, struct http_request *req, struct http_response *res){
  if(node->id){
    switch(node->id){
      case BODY:
        return res->payload;
        break;
      case PATH:
        return serialize_path(req,0,0);
        break;
      case METHOD:
        return req->method;
        break;
      case HTMLMIME:
        return ((res->header_mime!=NULL) ? res->header_mime : "");
        break;
    }
  }
  if(node->val_type==STRING_LITERAL) return node->val.text;
}



int number_eval(struct ast_node *node, struct http_request *req, struct http_response *res){
  if(node->id){
    switch(node->id){
      case CODE:
        return res->code;
        break;
      case SIZE:
        return res->pay_len;
        break;
     }
  }
  if(node->val_type==NUMBER_LITERAL) return node->val.number;
  return -1;
}


int ast_eval(struct ast_node *node, struct http_request *req, struct http_response *res ){
  char *l, *r;
  char *end_l, *end_r;
  switch(node->oper){
    case AND:
      return ast_eval(node->lhs, req, res) && ast_eval(node->rhs, req, res);
      break;
    case OR:
      return ast_eval(node->lhs, req, res) || ast_eval(node->rhs, req, res);
      break;
    case NOT:
      return !ast_eval(node->lhs, req, res);
      break;
    case CONTAINS:
      return strstr(string_eval(node->lhs, req, res),string_eval(node->rhs, req, res))!=NULL;
    case EQ:
      if(node->lhs->val_type==NUMBER_LITERAL) return number_eval(node->lhs, req, res)==number_eval(node->rhs, req, res);
      else return (strcmp(string_eval(node->lhs, req, res),string_eval(node->rhs, req, res))==0);
      break;
    case NE:
      if(node->lhs->val_type==NUMBER_LITERAL) return number_eval(node->lhs, req, res)!=number_eval(node->rhs, req, res);
      else return (strcmp(string_eval(node->lhs, req, res),string_eval(node->rhs, req, res))!=0);
      break;
    case ENDSWITH:
      l=string_eval(node->lhs,req,res);
      r=string_eval(node->rhs,req,res);
      if(strlen(r)==0) return 0; 
      if(strlen(r)>strlen(l)) return 0;
      end_l=l+strlen(l)-1;
      end_r=r+strlen(r)-1;
      while(end_l>=l){
        if(*end_l!=*end_r) return 0;
        end_l--;
        end_r--;
      }
      return 1;
      break;
    case GT:
      return number_eval(node->lhs,req,res) > number_eval(node->rhs,req,res);
      break;
    case LT:
      return number_eval(node->lhs,req,res) < number_eval(node->rhs,req,res);
      break;
    case GE:
      return number_eval(node->lhs,req,res) >= number_eval(node->rhs,req,res);
      break;
    case LE:
      return number_eval(node->lhs,req,res) <= number_eval(node->rhs,req,res);
      break;
  }
  return 0;
}

void yyerror(char *s, ...)
{
  va_list ap;
  va_start(ap, s);

  vfprintf(stderr, s, ap);
  fprintf(stderr, "\n");
}

