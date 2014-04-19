#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <inttypes.h>

#include "common/mndb-ygen.h"
#include "common/mndb-util.h"

#define MNDB_EACH_HEADER_BEGIN(mem) \
{                                   \
  uint8_t *data_end_ptr = mem->data + mem->cur; \
  uint8_t * data_ptr = mem->data; \
  while(data_ptr < data_end_ptr) {\
    mndb_ygen_header_t *header = (mndb_ygen_header_t *) data_ptr;


#define MNDB_EACH_HEADER_END(mem) \
    data_ptr += header->size; \
  }\
}

static const char *const _mndb_log_tag = "mem";

void
mndb_ygen_init(mndb_ygen_t *mem, size_t size, mndb_ygen_flags_t flags)
{
  assert(size > 0);

  mem->data = aligned_alloc(MNDB_YGEN_ALIGN, size);
  mem->cur = 0;
  mem->cur2 = 0;
  mem->data2 = NULL;
  mem->size = mem->data != NULL ? size : 0;
  mem->flags = flags;
}


void
mndb_ygen_destroy(mndb_ygen_t *mem)
{
  if(mem->data != NULL)
  {
    free(mem->data);
  }
}

mndb_ygen_header_t *
mndb_ygen_header(uint8_t *ptr)
{
  return (mndb_ygen_header_t *) (ptr - sizeof(mndb_ygen_header_t));
}


uint8_t *
mndb_ygen_header_data(mndb_ygen_header_t *header)
{
  return header->data;
}

static uint8_t *
mndb_ygen_copy_header(mndb_ygen_t *mem, mndb_ygen_header_t *header)
{
  if(likely(header->fwd_ptr == NULL))
  {
    header->fwd_ptr = mem->data2 + mem->cur2;
    mndb_debug("copying header %p (%p) to %p", header, header->data, header->fwd_ptr);
    mem->cur2 += header->size;
    header = (mndb_ygen_header_t *) memcpy(header->fwd_ptr, header, header->size);

    if(likely(header->copy_func != NULL))
    {
      (*header->copy_func)(mem, header->data);
    }
  }
  else
  {
    mndb_debug("alredy copied header %p (%p)", header, header->data);
    header = (mndb_ygen_header_t *) (header->fwd_ptr);
  }
  return header->data;
}

uint8_t *
mndb_ygen_copy(mndb_ygen_t *mem, uint8_t *ptr)
{
  if(unlikely(ptr == NULL)) return NULL;

  mndb_ygen_header_t *header = mndb_ygen_header(ptr);
  return mndb_ygen_copy_header(mem, header);
}


bool
mndb_ygen_copy_from_roots(mndb_ygen_t *mem, size_t size, uint8_t *roots[], size_t len)
{
  uintptr_t freed;
  bool retval = true;

  if((len == 0 || roots == NULL) && size <= mem->size)
  {
    freed = mem->cur;
    mem->cur = 0;

    retval = true;
    goto done;
  }

  assert(size >= mem->cur);

  mem->cur2 = 0;
  mem->data2 = aligned_alloc(MNDB_YGEN_ALIGN, size);
  if(unlikely(mem->data2 == NULL))
  {
    mndb_error("allocating to space failed");

    retval = false;
    goto done;
  }

  for(size_t i = 0; i < len; i++)
  {
    uint8_t *ptr = roots[i];

    if(unlikely(ptr == NULL)) continue;

    mndb_ygen_header_t *header = mndb_ygen_header(ptr);

    mndb_debug("about to copy root header %p (%p)", header, header->data);
    roots[i] = mndb_ygen_copy_header(mem, header);
  }

  freed = mem->cur - mem->cur2;
  mndb_debug("gc cur from %zd to %zd", mem->cur, mem->cur2);
  assert(mem->cur >= mem->cur2);

  free(mem->data);
  mem->data = mem->data2;
  mem->data2 = NULL;
  mem->size = size;
  mem->cur = mem->cur2;
  mem->cur2 = 0;

  MNDB_EACH_HEADER_BEGIN(mem)
    header->fwd_ptr = NULL;
  MNDB_EACH_HEADER_END(mem)

done:
  mndb_debug("copy gc finished with status %d (freed %" PRIuPTR " bytes)", retval, freed);
  return retval;
}

static bool
mndb_ygen_resize(mndb_ygen_t *mem, size_t size, uint8_t *roots[], size_t len)
{
  mndb_debug("resizing from %zd to %zd", mem->size, size);
  return mndb_ygen_copy_from_roots(mem, size, roots, len);
}

bool
mndb_ygen_each_header(mndb_ygen_t *mem, mndb_ygen_each_header_func_t cb, void *user_data)
{
  MNDB_EACH_HEADER_BEGIN(mem)
    if(!((*cb)(header, user_data))) return false;
  MNDB_EACH_HEADER_END(mem)
  return true;
}

bool
mndb_ygen_gc(mndb_ygen_t *mem, uint8_t *roots[], size_t len)
{
  size_t size;

  if(mem->cur >= (mem->size / 2 + mem->size / 4))
  {
    size = mem->size + mem->size / 2;
  }
  else
  {
    size = mem->size + mem->cur / 2;
  }
  return mndb_ygen_copy_from_roots(mem, size, roots, len);
}

uint8_t *
mndb_ygen_alloc(mndb_ygen_t *mem,
               size_t size,
               mndb_ygen_copy_func_t copy_func,
               uint8_t *roots[],
               size_t roots_len)
{
  uintptr_t new_cur;
  size_t total_size = MNDB_ALIGN(size + sizeof(mndb_ygen_header_t), MNDB_YGEN_ALIGN);
  new_cur = mem->cur + total_size;

  if(unlikely(new_cur >= mem->size))
  {
    size_t new_size = new_cur + (new_cur / 2) + (new_cur / 4);
    if(unlikely(!mndb_ygen_resize(mem, new_size, roots, roots_len)))
    {
      return NULL;
    }
  }

  mndb_ygen_header_t *header = (mndb_ygen_header_t *)(mem->data + mem->cur);

  mndb_debug("allocated header %p (%p) of size %zd (%zd total)", header, header->data, size, total_size);
  header->size      = total_size;
  header->copy_func = copy_func;
  header->fwd_ptr   = NULL;

  mem->cur = new_cur;

  return header->data;
}
