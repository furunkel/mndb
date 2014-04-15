#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MNDB_MEM_ALIGN ((size_t)8)

typedef enum {
  MNDB_MEM_FLAGS_NONE    = 0,
  MNDB_MEM_FLAGS_SHRINK  = (1 << 2)
} mndb_mem_flags_t;


struct mndb_mem_s {
  size_t cur;
  size_t cur2;
  mndb_mem_flags_t flags;
  size_t size;
  uint8_t *data;
  uint8_t *data2;
};


#define mndb_mem_get_flags(mem) (mem->flags)
#define mndb_mem_get_size(mem) (mem->size)
#define mndb_mem_used(mem) (mem->cur)

typedef struct mndb_mem_s mndb_mem_t;

typedef void (*mndb_mem_copy_func_t)(mndb_mem_t *mem, uint8_t *data);

typedef struct {
  size_t size;
  mndb_mem_copy_func_t copy_func;
  uint8_t *fwd_ptr;
  uint8_t data[];
} mndb_mem_header_t;

typedef bool (*mndb_mem_each_header_func_t)(mndb_mem_header_t *header,
                                            void *user_data);

mndb_mem_header_t *
mndb_mem_header(uint8_t *ptr);

uint8_t *
mndb_mem_alloc(mndb_mem_t *mem, size_t size,
               mndb_mem_copy_func_t copy_func,
               uint8_t *roots[], size_t roots_len)
               __attribute__((malloc)) __attribute__((alloc_size(2)));

void
mndb_mem_init(mndb_mem_t *mem, size_t size, mndb_mem_flags_t flags);

void
mndb_mem_destroy(mndb_mem_t *mem);

uint8_t *
mndb_mem_header_data(mndb_mem_header_t *header);

bool
mndb_mem_each_header(mndb_mem_t *mem, mndb_mem_each_header_func_t cb, void *user_data);

uint8_t *
mndb_mem_copy(mndb_mem_t *mem, uint8_t *ptr);

bool
mndb_mem_gc(mndb_mem_t *mem, uint8_t *roots[], size_t len);
