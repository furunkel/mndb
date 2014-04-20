#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <inttypes.h>

#include "common/mndb-ygen.h"
#include "common/mndb-util.h"

#define EACH_HEADER_BEGIN(ygen) \
{                                   \
  uint8_t *data_end_ptr = ygen->data + ygen->cur; \
  uint8_t * data_ptr = ygen->data; \
  while(data_ptr < data_end_ptr) {\
    mndb_ygen_header_t *header = (mndb_ygen_header_t *) data_ptr;


#define EACH_HEADER_END(ygen) \
    data_ptr += header->size; \
  }\
}

static const char *const _mndb_log_tag = "ygen";

void
mndb_ygen_init(mndb_ygen_t *ygen, size_t size, mndb_ygen_flags_t flags)
{
  assert(size > 0);

  ygen->data = aligned_alloc(MNDB_YGEN_ALIGN, size);
  if(ygen->data == NULL)
  {
    mndb_error("allocating ygen failed");
  }
  ygen->cur = 0;
  ygen->cur2 = 0;
  ygen->data2 = NULL;
  ygen->size = ygen->data != NULL ? size : 0;
  ygen->flags = flags;
  ygen->copy_funcs_cur = 0;
}

void
mndb_ygen_destroy(mndb_ygen_t *ygen)
{
  if(ygen->data != NULL)
  {
    free(ygen->data);
  }
}

mndb_ygen_header_t *
mndb_ygen_header(uint8_t *ptr)
{
  return (mndb_ygen_header_t *) (ptr - offsetof(mndb_ygen_header_t, data));
}


uint8_t *
mndb_ygen_header_data(mndb_ygen_header_t *header)
{
  return header->data;
}

static uint8_t *
mndb_ygen_copy_header(mndb_ygen_t *ygen, mndb_ygen_header_t *header)
{
  if(likely(header->fwd_ptr == NULL))
  {
    header->fwd_ptr = ygen->data2 + ygen->cur2;
    mndb_debug("copying header %p (data: %p, size: %d) to %p", header, header->data, header->size, header->fwd_ptr);
    ygen->cur2 += header->size;
    header = (mndb_ygen_header_t *) memcpy(header->fwd_ptr, header, header->size);

    if(likely(header->copy_func_idx < ygen->copy_funcs_cur))
    {
      (*ygen->copy_funcs[header->copy_func_idx])(ygen, header->data);
    }
  }
  else
  {
    mndb_debug("already copied header %p (%p)", header, header->data);
    header = (mndb_ygen_header_t *) (header->fwd_ptr);
  }
  return header->data;
}

uint8_t *
mndb_ygen_copy(mndb_ygen_t *ygen, uint8_t *ptr)
{
  if(unlikely(ptr == NULL)) return NULL;

  mndb_ygen_header_t *header = mndb_ygen_header(ptr);
  return mndb_ygen_copy_header(ygen, header);
}


bool
mndb_ygen_copy_from_roots(mndb_ygen_t *ygen, size_t size, uint8_t *roots[], size_t len)
{
  uintptr_t freed;
  bool retval = true;

  if((len == 0 || roots == NULL) && size <= ygen->size)
  {
    freed = ygen->cur;
    ygen->cur = 0;

    retval = true;
    goto done;
  }

  assert(size >= ygen->cur);

  ygen->cur2 = 0;
  ygen->data2 = aligned_alloc(MNDB_YGEN_ALIGN, size);
  if(unlikely(ygen->data2 == NULL))
  {
    mndb_error("allocating to space failed");

    retval = false;
    goto done;
  }

  for(size_t i = 0; i < len; i++)
  {
    roots[i] = mndb_ygen_copy(ygen, roots[i]);
  }

  freed = ygen->cur - ygen->cur2;
  mndb_debug("gc cur from %zd to %zd", ygen->cur, ygen->cur2);
  assert(ygen->cur >= ygen->cur2);

  free(ygen->data);
  ygen->data = ygen->data2;
  ygen->data2 = NULL;
  ygen->size = size;
  ygen->cur = ygen->cur2;
  ygen->cur2 = 0;

  EACH_HEADER_BEGIN(ygen)
    header->fwd_ptr = NULL;
  EACH_HEADER_END(ygen)

done:
  mndb_debug("copy gc finished with status %d (freed %" PRIuPTR " bytes)", retval, freed);
  return retval;
}

static bool
mndb_ygen_resize(mndb_ygen_t *ygen, size_t size, uint8_t *roots[], size_t len)
{
  mndb_debug("resizing from %zd to %zd", ygen->size, size);
  return mndb_ygen_copy_from_roots(ygen, size, roots, len);
}

void *
mndb_ygen_each_header(mndb_ygen_t *ygen, mndb_ygen_each_header_func_t cb, void *user_data)
{
  EACH_HEADER_BEGIN(ygen)
    user_data = ((*cb)(header, user_data));
  EACH_HEADER_END(ygen)
  return user_data;
}

bool
mndb_ygen_gc(mndb_ygen_t *ygen, uint8_t *roots[], size_t len)
{
  size_t size;

  if(ygen->cur >= (ygen->size / 2 + ygen->size / 4))
  {
    size = ygen->size + ygen->size / 2;
  }
  else
  {
    size = ygen->size + ygen->cur / 2;
  }
  return mndb_ygen_copy_from_roots(ygen, size, roots, len);
}

uint8_t *
mndb_ygen_alloc(mndb_ygen_t *ygen,
                size_t size,
                uint16_t copy_func_idx,
                uint8_t *roots[],
                size_t roots_len)
{

  size_t new_cur;
  size_t total_size = sizeof(mndb_ygen_header_t) + MNDB_ALIGN(size, MNDB_YGEN_ALIGN);

  assert(size > 0);
  assert(total_size <= MNDB_YGEN_MAX_ALLOC_SIZE);

  new_cur = ygen->cur + total_size;

  if(unlikely(new_cur >= ygen->size))
  {
    mndb_debug("new cur %zd (before %zd) exceeds size (%zd), resizing...", new_cur, ygen->cur, ygen->size);

    size_t new_size = new_cur + (new_cur / 2) + (new_cur / 4);
    if(unlikely(!mndb_ygen_resize(ygen, new_size, roots, roots_len)))
    {
      return NULL;
    }
    new_cur = ygen->cur + total_size;
  }

  mndb_ygen_header_t *header = (mndb_ygen_header_t *)(ygen->data + ygen->cur);

  assert((uintptr_t)header->data % MNDB_YGEN_ALIGN == 0);
  assert(((uintptr_t)header->data + size) <= (uintptr_t)ygen->data + new_cur);

  header->fwd_ptr = NULL;
  header->copy_func_idx = copy_func_idx;
  header->size = (uint16_t) total_size;
  header->age = 0;

  assert(header->size > 0);

  mndb_debug("allocated header %p (%p) of size %d (%zd data)", header, header->data, header->size, size);
  mndb_debug("cur from %zd to %zd , %zd %zd", ygen->cur, new_cur, total_size, ygen->cur + total_size);

  ygen->cur = new_cur;

  assert((uintptr_t)ygen->data + new_cur == (uintptr_t)header + header->size);
  return header->data;
}

uint16_t
mndb_ygen_register_copy_func(mndb_ygen_t *ygen, mndb_ygen_copy_func_t copy_func)
{
  uint16_t idx = ygen->copy_funcs_cur;

  ygen->copy_funcs[ygen->copy_funcs_cur++] = copy_func;

  return idx;
}
