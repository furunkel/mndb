#include "parser/mndb-ast.h"

mndb_ast_t *
mndb_ast_new(size_t capa)
{
  mndb_ast_t *ast = (mndb_ast_t *) malloc(sizeof(mndb_ast_t) + capa * sizeof(mndb_ast_t *));

  ast->type = MNDB_AST_TYPE_INVALID;
  ast->token = NULL;
  ast->children_capa = capa;
  ast->children_len = 0;

  return ast;
}
