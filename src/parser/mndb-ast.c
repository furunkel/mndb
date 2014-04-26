#include <assert.h>

#include "parser/mndb-ast.h"

mndb_ast_t *
mndb_ast_new(mndb_ast_type_t type)
{
  mndb_ast_t *ast = (mndb_ast_t *) malloc(sizeof(mndb_ast_t));

  ast->type = type;
  ast->token = NULL;
  ast->first_child = NULL;
  ast->last_child = NULL;
  ast->sibling = NULL;
  return ast;
}

mndb_ast_t *
mndb_ast_add_child(mndb_ast_t *ast, mndb_ast_t *child)
{
  if(ast->last_child == NULL)
  {
    ast->last_child = child;
    ast->first_child = child;
  }
  else
  {
    ast->last_child->sibling = child;
    ast->last_child = child;
  }
  return ast;
}

void
mndb_ast_set_type(mndb_ast_t *ast, mndb_ast_type_t type)
{
  ast->type = type;
}
