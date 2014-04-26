#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include "parser/mndb-parser.h"
#include "common/mndb-util.h"

#include "gen/parser.h"
#include "gen/lexer.h"

#define MNDB_PARSER_AST_ROOT_MAX_CHILDREN 512

void *ParseAlloc(void *(*mallocProc)(size_t));
void ParseFree(void *p, void (*freeProc)(void*));
void Parse(void *, int, mndb_parser_token_t *, _mndb_parser_ctx_t *ctx);
void ParseTrace(FILE*, const char*);

extern mndb_log_level_t _mndb_log_level;

mndb_parser_token_t *
mndb_parser_token_new(size_t size)
{
  mndb_parser_token_t *token = malloc(sizeof(mndb_parser_token_t) + size);
  token->begin = 0;
  token->end = 0;
  token->line = 0;
  token->col = 0;
  token->id = 0;
  token->refc = 1;
  token->type = MNDB_PARSER_TOKEN_TYPE_NO_VALUE;
  token->int_val = INT64_MIN;
  token->float_val = NAN;
  token->str_len = size;

  return token;
}

void
mndb_parser_token_set_value(mndb_parser_token_t *token, mndb_parser_token_type_t type, ...)
{
  va_list vl;
  va_start(vl, type);

  switch(type)
  {
    case MNDB_PARSER_TOKEN_TYPE_NO_VALUE:
      token->type = MNDB_PARSER_TOKEN_TYPE_NO_VALUE;
      token->int_val = INT64_MIN;
      token->float_val = NAN;
      break;
    case MNDB_PARSER_TOKEN_TYPE_INT:
      token->type = MNDB_PARSER_TOKEN_TYPE_INT;
      token->int_val = va_arg(vl, int64_t);
      token->float_val = NAN;
      break;
    case MNDB_PARSER_TOKEN_TYPE_FLOAT:
      token->type = MNDB_PARSER_TOKEN_TYPE_FLOAT;
      token->float_val = va_arg(vl, double);
      token->int_val = INT64_MIN;
      break;
    case MNDB_PARSER_TOKEN_TYPE_STR:
    {
      token->type = MNDB_PARSER_TOKEN_TYPE_STR;
      char *str = va_arg(vl, char *);
      memcpy(token->str_val, str, token->str_len);
      token->float_val = NAN;
      token->int_val = INT64_MIN;
      break;
    }
    default:
      assert(0);
  }

  va_arg(vl, int);
  va_end(vl);
}

void
mndb_parser_token_free(mndb_parser_token_t *token)
{
  free(token);
}

mndb_parser_t *
mndb_parser_new(mndb_parser_mode_t mode)
{
  mndb_parser_t *parser = (mndb_parser_t *) malloc(sizeof(mndb_parser_t));

  yylex_init(&parser->scanner);

  if(mode == MNDB_PARSER_MODE_PARSE)
  {
    parser->parser = ParseAlloc(malloc);
    if(_mndb_log_level <= MNDB_LOG_LEVEL_DEBUG)
    {
      ParseTrace(stderr, "mndb:DEBUG:parser: ");
    }
  }
  else
  {
    parser->parser = NULL;
  }

  parser->mode = mode;

  return parser;
}


mndb_ast_t *
mndb_parser_parse(mndb_parser_t *parser, const char *buf, size_t len)
{
  YY_BUFFER_STATE buffer = NULL;
  buffer = yy_scan_bytes(buf, len, parser->scanner);

  _mndb_parser_ctx_t ctx = {.parser = parser, .root = NULL};

  yylex_init_extra((void *)&ctx, &parser->scanner);

  while(yylex(parser->scanner) != 0) {}

  if(parser->mode == MNDB_PARSER_MODE_PARSE)
  {
    Parse(parser->parser, 0 /* eof */, NULL, &ctx);
  }

  yy_delete_buffer(buffer, parser->scanner);

  return ctx.root;
}


void
mndb_parser_free(mndb_parser_t *parser)
{
  yylex_destroy(parser->scanner);
  ParseFree(parser->parser, free);
  free(parser);
}
