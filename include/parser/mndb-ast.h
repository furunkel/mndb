#pragma once

typedef struct mndb_ast_s mndb_ast_t;

#include "parser/mndb-parser.h"

#define MNDB_AST_NO_TOKEN ((uint32_t) -1)

typedef enum {
  MNDB_AST_TYPE_NONE,
  MNDB_AST_TYPE_TOKEN,
  MNDB_AST_TYPE_LIST,
  MNDB_AST_TYPE_PAIR,
  MNDB_AST_TYPE_MAP,
  MNDB_AST_TYPE_BIN_OP,
  MNDB_AST_TYPE_UNARY_OP,
  MNDB_AST_TYPE_FACT,
  MNDB_AST_TYPE_FACTS,
  MNDB_AST_TYPE_HEAD,
  MNDB_AST_TYPE_RULE,
  MNDB_AST_TYPE_ROOT,
  MNDB_AST_TYPE_QUERY,
  MNDB_AST_TYPE_RETRACT,
  MNDB_AST_TYPE_ASSERT,
  MNDB_AST_N_TYPES
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

void
mndb_ast_set_token(mndb_ast_t *ast, mndb_parser_token_t *token);

uint32_t
mndb_ast_token_id(mndb_ast_t *ast);

char *
mndb_ast_to_sexp(mndb_ast_t *ast, size_t *len);

