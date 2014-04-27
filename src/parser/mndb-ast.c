#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>

#include "parser/mndb-ast.h"
#include "common/mndb-str-buf.h"

static const char *_mndb_ast_type_to_s_table[MNDB_AST_N_TYPES] = {
  "none",
  "token",
  "list",
  "pair",
  "map",
  "bin_op",
  "unary_op",
  "fact",
  "facts",
  "head",
  "rule",
  "root",
  "query",
  "retract",
  "assert",
};

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

void
mndb_ast_set_token(mndb_ast_t *ast, mndb_parser_token_t *token)
{
  ast->token = token;
}

uint32_t
mndb_ast_token_id(mndb_ast_t *ast)
{
  if(ast->token == NULL) return MNDB_AST_NO_TOKEN;

  return ast->token->id;
}

static void
mndb_ast_to_sexp_(mndb_ast_t *ast, mndb_str_buf_t *buf, bool root)
{
  if(!root) mndb_str_buf_append(buf, " ", -1);

  mndb_str_buf_append(buf, "(", -1);
  mndb_str_buf_append(buf, _mndb_ast_type_to_s_table[ast->type], -1);
  mndb_str_buf_append(buf, " ", -1);

  if(ast->token == NULL)
  {
    mndb_str_buf_append(buf, "none", -1);
  }
  else
  {
    char *val_s = mndb_parser_token_value_to_s(ast->token);
    mndb_str_buf_appendf(buf, "(%s %s)", mndb_parser_token_id_to_s(ast->token->id),
                               val_s);
    free(val_s);
  }

  for(mndb_ast_t *child = ast->first_child; child != NULL; child = child->sibling)
  {
    mndb_ast_to_sexp_(child, buf, false);
  }
  mndb_str_buf_append(buf, ")", -1);
}

char *
mndb_ast_to_sexp(mndb_ast_t *ast)
{
  mndb_str_buf_t buf;
  mndb_str_buf_init(&buf, 512);

  mndb_ast_to_sexp_(ast, &buf, true);

  char *str = mndb_str_buf_str(&buf);
  mndb_str_buf_destroy(&buf);

  return str;
}
