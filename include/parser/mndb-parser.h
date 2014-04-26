#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>


typedef enum {
  MNDB_PARSER_MODE_SCAN,
  MNDB_PARSER_MODE_PARSE
} mndb_parser_mode_t;

typedef enum {
  MNDB_PARSER_TOKEN_TYPE_NO_VALUE,
  MNDB_PARSER_TOKEN_TYPE_INT,
  MNDB_PARSER_TOKEN_TYPE_FLOAT,
  MNDB_PARSER_TOKEN_TYPE_STR
} mndb_parser_token_type_t;

typedef enum {
  MNDB_PARSER_TOKEN_FLAGS_NONE,
  MNDB_PARSER_TOKEN_FLAGS_SQ_STR,
  MNDB_PARSER_TOKEN_FLAGS_DQ_STR,
  MNDB_PARSER_TOKEN_FLAGS
} mndb_parser_token_flags_t;

typedef struct {
  uint32_t begin;
  uint32_t end;
  uint32_t line;
  uint32_t col;
  uint32_t refc;

  uint32_t id;
  mndb_parser_token_type_t type;

  int64_t int_val;
  double  float_val;
  size_t str_len;
  char  str_val[];

} mndb_parser_token_t;

typedef struct mndb_parser_s mndb_parser_t;

typedef void (*mndb_parser_token_func_t)(mndb_parser_t *parser, mndb_parser_token_t *token);

struct mndb_parser_s {
  uint32_t col;
  uint32_t last_col;
  uint32_t line;
  uint32_t last_line;
  uint32_t pos;
  uint32_t last_pos;

  mndb_parser_mode_t mode;
  mndb_parser_token_t *last_token;
  mndb_parser_token_t *pending_token;
  mndb_parser_token_func_t token_func;

  void *parser;
  void *scanner;

  char *src;

  bool after_comment;
};

#include "parser/mndb-ast.h"

typedef struct  {
  mndb_parser_t *parser;
  mndb_ast_t *root;
} _mndb_parser_ctx_t;

mndb_parser_token_t *
mndb_parser_token_new(size_t size);

void
mndb_parser_token_set_value(mndb_parser_token_t *token, mndb_parser_token_type_t type, ...);

void
mndb_parser_token_free(mndb_parser_token_t *token);

mndb_parser_t *
mndb_parser_new(mndb_parser_mode_t mode);

mndb_ast_t *
mndb_parser_parse(mndb_parser_t *parser, const char *buf, size_t len);

void
mndb_parser_free(mndb_parser_t *parser);
