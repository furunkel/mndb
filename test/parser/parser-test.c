#include <stdio.h>
#include <stdlib.h>

#include "mndb.h"
#include "../test-helper.h"


static const char *const _mndb_log_tag = "parser-test";

int
main(int argc, const char *argv[])
{
  mndb_init(&argc, &argv);

  mndb_parser_t *parser = mndb_parser_new(MNDB_PARSER_MODE_PARSE);

  int status;

  mndb_ast_t *ast;

  const size_t b_len = 1024;
  char b[b_len];

  mndb_str_buf_t buf;
  mndb_str_buf_init(&buf, b_len);

  while(true)
  {
    size_t n = fread(b, sizeof(char), b_len, stdin);

    if(!mndb_str_buf_append(&buf, b, (ssize_t) n))
    {
      mndb_error("appending to buffer failed");
      status = 1;
      goto end;
    }

    if(n < b_len) break;
  }

  if(!feof(stdin))
  {
    mndb_error("eof not reached");
    status = 1;
    goto end;
  }

  size_t len;
  char *src = mndb_str_buf_str(&buf, &len);

  ast = mndb_parser_parse(parser, src, len);
  char *ast_sexp = mndb_ast_to_sexp(ast, NULL);
  puts(ast_sexp);
  free(ast_sexp);
  free(src);

end:
  mndb_parser_free(parser);
  mndb_str_buf_destroy(&buf);

  return status;
}
