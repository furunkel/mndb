#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>

#define MNDB_YGEN_ALIGN ((size_t)sizeof(uintptr_t))

typedef enum {
  MNDB_YGEN_FLAGS_NONE = 0,
} mndb_ygen_flags_t;


struct mndb_ygen_s {
  size_t cur;
  size_t cur2;
  mndb_ygen_flags_t flags;
  size_t size;
  uint8_t *data;
  uint8_t *data2;
};


#define mndb_ygen_get_flags(mem) ((mem)->flags)
#define mndb_ygen_get_size(mem) ((mem)->size)
#define mndb_ygen_used(mem) ((mem)->cur)

typedef struct mndb_ygen_s mndb_ygen_t;

typedef void (*mndb_ygen_copy_func_t)(mndb_ygen_t *mem, uint8_t *data);

typedef struct {
  mndb_ygen_copy_func_t copy_func;
  uint8_t *fwd_ptr;
  size_t size;
  uint8_t data[];
} mndb_ygen_header_t;

static_assert(sizeof(mndb_ygen_header_t) % MNDB_YGEN_ALIGN == 0,
              "sizeof(mndb_ygen_header_t) must be multiple of MNDB_YGEN_ALIGN");

typedef bool (*mndb_ygen_each_header_func_t)(mndb_ygen_header_t *header,
                                            void *user_data);

mndb_ygen_header_t *
mndb_ygen_header(uint8_t *ptr);

uint8_t *
mndb_ygen_alloc(mndb_ygen_t *mem, size_t size,
               mndb_ygen_copy_func_t copy_func,
               uint8_t *roots[], size_t roots_len)
               __attribute__((malloc)) __attribute__((alloc_size(2)));

void
mndb_ygen_init(mndb_ygen_t *mem, size_t size, mndb_ygen_flags_t flags);

void
mndb_ygen_destroy(mndb_ygen_t *mem);

uint8_t *
mndb_ygen_header_data(mndb_ygen_header_t *header);

bool
mndb_ygen_each_header(mndb_ygen_t *mem, mndb_ygen_each_header_func_t cb, void *user_data);

uint8_t *
mndb_ygen_copy(mndb_ygen_t *mem, uint8_t *ptr);

bool
mndb_ygen_gc(mndb_ygen_t *mem, uint8_t *roots[], size_t len);
