#pragma once

#include <alloca.h>
#include <stdint.h>

typedef struct {
  size_t capa;
  size_t cur;
  uint8_t *data[];
} mndb_ptr_stack_t;

mndb_ptr_stack_t *
mndb_ptr_stack_new(size_t size);


void
mndb_ptr_stack_free(mndb_ptr_stack_t *stack);

static inline void
mndb_ptr_stack_push(mndb_ptr_stack_t *stack, uint8_t *data)
{
  stack->data[stack->cur++] = data;
}

static inline uint8_t *
mndb_ptr_stack_pop(mndb_ptr_stack_t *stack)
{
  return stack->data[--stack->cur];
}

static inline uint8_t **
mndb_ptr_stack_data(mndb_ptr_stack_t *stack)
{
  return stack->data;
}

static inline size_t
mndb_ptr_stack_size(mndb_ptr_stack_t *stack)
{
  return stack->capa * sizeof(uint8_t *);
}

static inline size_t
mndb_ptr_stack_capa(mndb_ptr_stack_t *stack)
{
  return stack->capa;
}

static inline size_t
mndb_ptr_stack_cur(mndb_ptr_stack_t *stack)
{
  return stack->cur;
}

static inline uint8_t *
mndb_ptr_stack_at(mndb_ptr_stack_t *stack, size_t index)
{
  return stack->data[index];
}
