#pragma once

typedef struct mndb_ast_s mndb_ast_t;

#include "parser/mndb-parser.h"

typedef enum {
  MNDB_AST_TYPE_INVALID,
  MNDB_AST_TYPE_TOKEN,
  MNDB_AST_TYPE_LIST,
  MNDB_AST_TYPE_PAIR,
  MNDB_AST_TYPE_MAP,
  MNDB_AST_TYPE_BIN_OP,
  MNDB_AST_TYPE_UNARY_OP,
  MNDB_AST_TYPE_UNIFY,
  MNDB_AST_TYPE_FACT,
} mndb_ast_type_t;

struct mndb_ast_s {
  mndb_ast_type_t type;
  mndb_parser_token_t *token;
  struct mndb_ast_s *first_child;
  struct mndb_ast_s *last_child;
  struct mndb_ast_s *sibling;
};

mndb_ast_t *
mndb_ast_new(mndb_ast_type_t type);

mndb_ast_t *
mndb_ast_add_child(mndb_ast_t *ast, mndb_ast_t *child);

void
mndb_ast_set_type(mndb_ast_t *ast, mndb_ast_type_t type);
