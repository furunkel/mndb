#pragma once

#include <stdint.h>
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
  MNDB_PARSER_TOKEN_ID_INVALID,
  MNDB_PARSER_TOKEN_ID_DBLQ_STR,
  MNDB_PARSER_TOKEN_ID_SNGLQ_STR,
  MNDB_PARSER_TOKEN_ID_INT,
  MNDB_PARSER_TOKEN_ID_FLOAT,
  MNDB_PARSER_TOKEN_ID_SL_COMMENT,
  MNDB_PARSER_TOKEN_ID_ML_COMMENT,
  MNDB_PARSER_TOKEN_ID_ATOM,
  MNDB_PARSER_TOKEN_ID_REGEXP,
  MNDB_PARSER_TOKEN_ID_IMPLI,
  MNDB_PARSER_TOKEN_ID_QUERY,
  MNDB_PARSER_TOKEN_ID_ASSERT,
  MNDB_PARSER_TOKEN_ID_RETRACT,
  MNDB_PARSER_TOKEN_ID_VAR,
  MNDB_PARSER_TOKEN_ID_LPAREN,
  MNDB_PARSER_TOKEN_ID_RPAREN,
  MNDB_PARSER_TOKEN_ID_LBRACK,
  MNDB_PARSER_TOKEN_ID_RBRACK,
  MNDB_PARSER_TOKEN_ID_COMMA,
  MNDB_PARSER_TOKEN_ID_SEMIC,
  MNDB_PARSER_TOKEN_ID_COLON,
  MNDB_PARSER_TOKEN_ID_BAR,
  MNDB_PARSER_TOKEN_ID_UNIFY,
  MNDB_PARSER_TOKEN_ID_IS,
  MNDB_PARSER_TOKEN_ID_MOD,
  MNDB_PARSER_TOKEN_ID_DIV,
  MNDB_PARSER_TOKEN_ID_MUL,
  MNDB_PARSER_TOKEN_ID_MINUS,
  MNDB_PARSER_TOKEN_ID_PLUS,
} mndb_parser_token_id_t;

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

  mndb_parser_token_id_t id;
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

mndb_parser_token_t *
mndb_parser_token_new(size_t size);

void
mndb_parser_token_set_value(mndb_parser_token_t *token, mndb_parser_token_type_t type, ...);

void
mndb_parser_token_free(mndb_parser_token_t *token);


