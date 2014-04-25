#pragma once

typedef struct mndb_ast_s mndb_ast_t;

#include "parser/mndb-parser.h"

typedef enum {
  MNDB_AST_TYPE_INVALID,
  MNDB_AST_TYPE_TOKEN,
} mndb_ast_type_t;

struct mndb_ast_s {
  mndb_ast_type_t type;
  mndb_parser_token_t *token;

  size_t children_len;
  size_t children_capa;
  struct mndb_ast_s *children[];
};

mndb_ast_t *
mndb_ast_new(size_t capa);


