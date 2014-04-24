#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include "parser/mndb-parser.h"

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
