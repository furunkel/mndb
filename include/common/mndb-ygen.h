#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#include <stdalign.h>

#define MNDB_YGEN_ALIGN (alignof(void *))
#define MNDB_YGEN_MAX_COPY_FUNCS 255
#define MNDB_YGEN_MAX_ALLOC_SIZE UINT16_MAX

typedef enum {
  MNDB_YGEN_FLAGS_NONE = 0,
} mndb_ygen_flags_t;


#define mndb_ygen_flags(ygen) ((ygen)->flags)
#define mndb_ygen_size(ygen) ((ygen)->size)
#define mndb_ygen_cur(ygen) ((ygen)->cur)

typedef struct mndb_ygen_s mndb_ygen_t;

typedef void (*mndb_ygen_copy_func_t)(mndb_ygen_t *ygen, uint8_t *data);

struct mndb_ygen_s {
  size_t cur;
  size_t cur2;
  size_t size;
  uint8_t *data;
  uint8_t *data2;
  mndb_ygen_flags_t flags;
  uint16_t copy_funcs_cur;
  mndb_ygen_copy_func_t copy_funcs[MNDB_YGEN_MAX_COPY_FUNCS];
};

typedef struct {
  uint8_t *fwd_ptr;
  uint16_t copy_func_idx;
  uint16_t size;
  uint16_t age;
  alignas(MNDB_YGEN_ALIGN) uint8_t data[];
} mndb_ygen_header_t;


typedef void * (*mndb_ygen_each_header_func_t)(mndb_ygen_header_t *header,
                                            void *user_data);

mndb_ygen_header_t *
mndb_ygen_header(uint8_t *ptr);

uint8_t *
mndb_ygen_alloc(mndb_ygen_t *ygen, size_t size,
               uint16_t copy_func_idx,
               uint8_t *roots[], size_t roots_len)
               __attribute__((malloc)) __attribute__((alloc_size(2)));

void
mndb_ygen_init(mndb_ygen_t *ygen, size_t size, mndb_ygen_flags_t flags);

void
mndb_ygen_destroy(mndb_ygen_t *ygen);

uint8_t *
mndb_ygen_header_data(mndb_ygen_header_t *header);

void *
mndb_ygen_each_header(mndb_ygen_t *ygen, mndb_ygen_each_header_func_t cb, void *user_data);

uint8_t *
mndb_ygen_copy(mndb_ygen_t *ygen, uint8_t *ptr);

bool
mndb_ygen_gc(mndb_ygen_t *ygen, uint8_t *roots[], size_t len);

uint16_t
mndb_ygen_register_copy_func(mndb_ygen_t *ygen, mndb_ygen_copy_func_t copy_func);
