#include "ast.h"

struct ast_node *new_ast_node(){
  return (struct ast_node *) malloc(sizeof(struct ast_node));
}
