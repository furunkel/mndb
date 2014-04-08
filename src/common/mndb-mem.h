#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MNDB_IDA_TABLE_PAGE_SIZE 512
#define MNDB_INITIAL_IDA_TABLE_SIZE (sizeof(uint8_t *) * 512)
#define MNDB_IDA_TABLE_GROWTH_FACTOR 3


typedef enum {
  MNDB_MEM_FLAGS_NONE   = 0,
  MNDB_MEM_FLAGS_MARK   = (1 << 0),
  MNDB_MEM_FLAGS_COPY   = (1 << 1),
  MNDB_MEM_FLAGS_SHRINK = (1 << 2)
} mndb_mem_flags_t;



typedef struct mndb_ida_table_s {
  size_t size;
  uintptr_t free;
  uintptr_t cur;
  struct mndb_mem_s *mem;
  struct mndb_ida_table_s *next;
  uint8_t data[];
} mndb_ida_table_t;


struct mndb_mem_s {
  uintptr_t cur;
  uintptr_t cur2;
  mndb_ida_table_t *ida_table;
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
                   mndb_mem_fin_func_t fin_func);

uint8_t *
mndb_mem_alloc_ida(mndb_mem_t *mem, size_t size,
                   mndb_mem_fwd_func_t fwd_func,
                   mndb_mem_mark_or_copy_func_t mark_or_copy_func,
                   mndb_mem_fin_func_t fin_func);


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
mndb_ref_ida(uint8_t *ptr);

void
mndb_free_ida(uint8_t *ptr);

void
mndb_unref_ida(uint8_t *ptr);

void
mndb_free(uint8_t *ptr);

void
mndb_ref(uint8_t *ptr);

void
mndb_unref(uint8_t *ptr);

uint8_t *
mndb_translate_ida(uint8_t *entry);

uint8_t *
mndb_ida_table_find(mndb_ida_table_t *table, uint8_t *val);


uint8_t *
mndb_mem_header_data(mndb_mem_header_t *header);

void
mndb_mem_compact(mndb_mem_t *mem);

void
mndb_mem_each_header(mndb_mem_t *mem, mndb_mem_each_header_func_t cb);

void
mndb_mem_each_ida_header(mndb_mem_t *mem, mndb_mem_each_header_func_t cb);

/* Exposed for testing purposes */
bool
_mndb_mem_compact_step_shrink(mndb_mem_t *mem, uintptr_t free);

void
_mndb_mem_compact_step_move(mndb_mem_t *mem, uintptr_t free);

void
_mndb_mem_compact_step_fwd(mndb_mem_t *mem);

void
_mndb_mem_compact_step_compute_fwd_addrs(mndb_mem_t *mem, uintptr_t *free);

uint8_t *
mndb_mem_copy(mndb_mem_t *mem, uint8_t *ptr);

void
mndb_mem_mark(uint8_t *ptr);
