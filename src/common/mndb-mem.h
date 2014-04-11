#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


typedef enum {
  MNDB_MEM_FLAGS_NONE   = 0,
  MNDB_MEM_FLAGS_MARK   = (1 << 0),
  MNDB_MEM_FLAGS_COPY   = (1 << 1),
  MNDB_MEM_FLAGS_SHRINK = (1 << 2)
} mndb_mem_flags_t;


struct mndb_mem_s {
  uintptr_t cur;
  uintptr_t cur2;
  mndb_mem_flags_t flags;
  size_t size;
  uint8_t *data;
  uint8_t *data2;
};

typedef struct mndb_mem_s mndb_mem_t;

typedef void (*mndb_mem_copy_func_t)(mndb_mem_t *mem, uint8_t *data);
typedef void (*mndb_mem_fwd_func_t)(uint8_t *data);
typedef void (*mndb_mem_mark_func_t)(uint8_t *data);
typedef void (*mndb_mem_fin_func_t)(uint8_t *data);

typedef union mndb_mem_mark_or_copy_func_u {
  mndb_mem_mark_func_t mark_func;
  mndb_mem_copy_func_t copy_func;

} mndb_mem_mark_or_copy_func_t;

typedef struct {
  size_t size;
  uint16_t refc;
  mndb_mem_fwd_func_t  fwd_func;
  mndb_mem_mark_or_copy_func_t mark_or_copy_func;
  mndb_mem_fin_func_t fin_func;
  uint8_t *fwd_ptr;
} mndb_mem_header_t;

typedef void (*mndb_mem_each_header_func_t)(mndb_mem_header_t *header,
                                            mndb_mem_t *mem, uint8_t *addr);

mndb_mem_header_t *
mndb_mem_header(uint8_t *ptr);

uint8_t *
mndb_mem_alloc(mndb_mem_t *mem, size_t size,
               mndb_mem_fwd_func_t fwd_func,
               mndb_mem_mark_or_copy_func_t mark_or_copy_func,
               mndb_mem_fin_func_t fin_func) __attribute__((malloc)) __attribute__((alloc_size(2)));

void
mndb_mem_init(mndb_mem_t *mem, size_t size, mndb_mem_flags_t flags);

void
mndb_mem_destroy(mndb_mem_t *mem);

void
mndb_mem_header_unref(mndb_mem_header_t *header);

void
mndb_mem_header_ref(mndb_mem_header_t *header);

void
mndb_mem_header_free(mndb_mem_header_t *header);

void
mndb_free(uint8_t *ptr);

void
mndb_ref(uint8_t *ptr);

void
mndb_unref(uint8_t *ptr);

uint8_t *
mndb_mem_header_data(mndb_mem_header_t *header);

void
mndb_mem_each_header(mndb_mem_t *mem, mndb_mem_each_header_func_t cb);

uint8_t *
mndb_mem_copy(mndb_mem_t *mem, uint8_t *ptr);

void
mndb_mem_mark(uint8_t *ptr);
