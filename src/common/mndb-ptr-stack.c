#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "common/mndb-ptr-stack.h"

mndb_ptr_stack_t *
mndb_ptr_stack_new(size_t size)
{
  mndb_ptr_stack_t *stack = (mndb_ptr_stack_t *) malloc(size + sizeof(mndb_ptr_stack_t));
  stack->size = size;
  stack->cur = 0;

  return stack;
}

mndb_ptr_stack_t *
mndb_ptr_stack_alloca(size_t size)
{
  mndb_ptr_stack_t *stack = (mndb_ptr_stack_t *) alloca(size + sizeof(mndb_ptr_stack_t));
  stack->size = size;
  stack->cur = 0;

  return stack;
}

void
mndb_ptr_stack_free(mndb_ptr_stack_t *stack)
{
  free(stack);
}

