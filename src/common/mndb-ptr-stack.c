#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "common/mndb-ptr-stack.h"

mndb_ptr_stack_t *
mndb_ptr_stack_new(size_t capa)
{
  mndb_ptr_stack_t *stack = (mndb_ptr_stack_t *) malloc(capa * sizeof(uint8_t *)
                                                        + sizeof(mndb_ptr_stack_t));
  stack->capa = capa;
  stack->cur = 0;

  return stack;
}

mndb_ptr_stack_t *
mndb_ptr_stack_alloca(size_t capa)
{
  mndb_ptr_stack_t *stack = (mndb_ptr_stack_t *) alloca(capa * sizeof(uint8_t *)
                                                        + sizeof(mndb_ptr_stack_t));
  stack->capa = capa;
  stack->cur = 0;

  return stack;
}

void
mndb_ptr_stack_free(mndb_ptr_stack_t *stack)
{
  free(stack);
}

