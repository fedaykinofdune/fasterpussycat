


#ifndef FASTERPUSSYCAT_AST_H
#define FASTERPUSSYCAT_AST_H



union ast_value {
  int number;
  char *text;
};

struct ast_node {
  int oper;
  struct ast_node *lhs;
  struct ast_node *rhs;
  union ast_value val;
  int val_type;
  int id;
};

struct ast_node *new_ast_node();

#endif
