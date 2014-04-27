#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#include "mndb.h"
#include "../test-helper.h"


int
main(int argc, const char *argv[])
{
  mndb_init(&argc, &argv);

  mndb_parser_t *parser = mndb_parser_new(MNDB_PARSER_MODE_PARSE);

  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  mndb_ast_t *ast;

  while((read = getline(&line, &len, stdin)) != -1)
  {
    ast = mndb_parser_parse(parser, line, (size_t)read);
    char *ast_sexp = mndb_ast_to_sexp(ast);
    puts(ast_sexp);
    free(ast_sexp);
  }

  mndb_parser_free(parser);
}
