#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <inttypes.h>

#include "mndb-mem.h"
#include "mndb-util.h"

#define MNDB_EACH_HEADER_BEGIN(mem) \
{                                   \
  uint8_t *data_end_ptr = mem->data + mem->cur; \
  uint8_t * data_ptr = mem->data; \
  while(data_ptr < data_end_ptr) {\
    mndb_mem_header_t *header = (mndb_mem_header_t *) data_ptr;


#define MNDB_EACH_HEADER_END(mem) \
    data_ptr += header->size; \
  }\
}

static const char *const _mndb_log_tag = "mem";

void
mndb_mem_init(mndb_mem_t *mem, size_t size, mndb_mem_flags_t flags)
{
  // size == 0 will be interpreted as uninitialized !
  // for mems/tables etc. so yo can't init with zero size
  assert(size > 0);

  mem->data = malloc(size);
  mem->cur = 0;
  mem->cur2 = 0;
  mem->data2 = NULL;
  mem->size = mem->data != NULL ? size : 0;
  mem->flags = flags;
}


void
mndb_mem_destroy(mndb_mem_t *mem)
{
  if(mem->data != NULL)
  {
    free(mem->data);
  }
}

static bool
mndb_mem_resize(mndb_mem_t *mem, size_t size)
{
  uint8_t *new_data = realloc(mem->data, size);
  if(likely(new_data != NULL))
  {
    mem->size = size;
    mem->data = new_data;
    return true;
  }
  else
  {
    mndb_warn("resizing mem failed\n");
    return false;
  }
}

mndb_mem_header_t *
mndb_mem_header(uint8_t *ptr)
{
  return (mndb_mem_header_t *) (ptr - sizeof(mndb_mem_header_t));
}

void
mndb_unref(uint8_t *ptr)
{
  mndb_mem_header_unref(mndb_mem_header(ptr));
}

void
mndb_ref(uint8_t *ptr)
{
  mndb_mem_header_ref(mndb_mem_header(ptr));
}

void
mndb_free(uint8_t *ptr)
{
  mndb_mem_header_free(mndb_mem_header(ptr));
}

uint8_t *
mndb_mem_alloc(mndb_mem_t *mem, size_t size,
                   mndb_mem_fwd_func_t fwd_func,
                   mndb_mem_mark_or_copy_func_t mark_or_copy_func,
                   mndb_mem_fin_func_t fin_func)
{
  uintptr_t new_cur;
  new_cur = mem->cur + sizeof(mndb_mem_header_t) + size;

  if(unlikely(new_cur >= mem->size))
  {
    if(unlikely(!mndb_mem_resize(mem, (size_t)(new_cur + (mem->size / 2)))))
    {
      return NULL;
    }
  }

  mndb_mem_header_t *header = (mndb_mem_header_t *)(mem->data + mem->cur);

  mndb_debug("alloc header: %p\n", header);
  header->size      = size + sizeof(mndb_mem_header_t);
  header->refc      = (mem->flags & MNDB_MEM_FLAGS_MARK) ? 0 : 1;
  header->fwd_func  = fwd_func;
  header->mark_or_copy_func = mark_or_copy_func;
  header->fin_func = fin_func;
  header->fwd_ptr   = NULL;

  mem->cur = new_cur;

  return mndb_mem_header_data(header);
}

void
mndb_mem_header_unref(mndb_mem_header_t *header)
{
  header->refc = (uint16_t)MAX(0, header->refc - 1);
}

void
mndb_mem_header_ref(mndb_mem_header_t *header)
{
  header->refc++;
}

void
mndb_mem_header_free(mndb_mem_header_t *header)
{
  mndb_debug("free header: %p\n", header);
  header->refc = 0;
}

uint8_t *
mndb_mem_header_data(mndb_mem_header_t *header)
{
  return (uint8_t *)header + sizeof(mndb_mem_header_t);
}

static void
mndb_mem_mark_from_roots(mndb_mem_t *mem, uint8_t *roots[], size_t len)
{
  for(size_t i = 0; i < len; i++)
  {
    uint8_t *ptr = roots[i];
    mndb_mem_header_t *header = mndb_mem_header(ptr);

    assert(header->mark_or_copy_func.mark_func != NULL);
    mndb_mem_mark(mndb_mem_header_data(header));
  }
}

bool
mndb_mem_copy_from_roots(mndb_mem_t *mem, uint8_t *roots[], size_t len)
{
  if(len == 0 || roots == NULL)
  {
    mndb_error("no roots given");
    return false;
  }

  mem->cur2 = 0;
  mem->data2 = malloc(mem->size);
  if(unlikely(mem->data2 == NULL))
  {
    mndb_error("allocating to space failed\n");
    mem->size = 0;
    return false;
  }

  for(size_t i = 0; i < len; i++)
  {
    uint8_t *ptr = roots[i];
    mndb_mem_header_t *header = mndb_mem_header(ptr);

    assert(header->mark_or_copy_func.copy_func != NULL);
    roots[i] = mndb_mem_copy(mem, mndb_mem_header_data(header));
  }

  if(likely(mem->data2 != NULL))
  {
    uint8_t *data = mem->data;
    mem->data = mem->data2;
    mem->data2 = NULL;
    free(data);

    MNDB_EACH_HEADER_BEGIN(mem)
      header->fwd_ptr = NULL;
    MNDB_EACH_HEADER_END(mem)

  }
  else
  {
    mndb_warn("empty copy gc run\n");
  }

  return true;
}

void
mndb_mem_mark(uint8_t *ptr)
{
  mndb_mem_header_t *header = mndb_mem_header(ptr);

  if(unlikely(header->refc == 1))
  {
    return;
  }

  header->refc = 1;
  (*header->mark_or_copy_func.mark_func)(mndb_mem_header_data(header));
}

uint8_t *
mndb_mem_copy(mndb_mem_t *mem, uint8_t *ptr)
{
  mndb_mem_header_t *header = mndb_mem_header(ptr);

  if(likely(header->fwd_ptr == NULL))
  {
    header->fwd_ptr = mem->data2 + mem->cur2;
    mem->cur2 += header->size;
    header = (mndb_mem_header_t *) memcpy(header->fwd_ptr, header, header->size);

    (*header->mark_or_copy_func.copy_func)(mem, mndb_mem_header_data(header));
  }
  else
  {
    header = (mndb_mem_header_t *) (header->fwd_ptr);
  }
  return mndb_mem_header_data(header);
}

inline void
mndb_mem_forward_reference(uint8_t **ref_ptr)
{
  mndb_mem_header_t *header = mndb_mem_header(*ref_ptr);
  *ref_ptr = header->fwd_ptr;
}


void
mndb_mem_each_header(mndb_mem_t *mem, mndb_mem_each_header_func_t cb, void *user_data)
{
  MNDB_EACH_HEADER_BEGIN(mem)
    (*cb)(header, user_data);
  MNDB_EACH_HEADER_END(mem)
}

static void
mndb_mem_compact_step_compute_fwd_addrs(mndb_mem_t *mem, uint8_t *roots[], size_t len, uintptr_t *free_)
{

  /* 1 - Compute forward addresses */
  uintptr_t free = 0;

  MNDB_EACH_HEADER_BEGIN(mem)
    if(header->refc == 0)
    {
      free += header->size;
      if(unlikely(header->fin_func != NULL))
      {
        (*header->fin_func)(mndb_mem_header_data(header));
      }
    }
    else
    {
      header->fwd_ptr = (uint8_t *)header - free;
    }
  MNDB_EACH_HEADER_END(mem)

  for(size_t i = 0; i < len; i++)
  {
    roots[i] = mndb_mem_header(roots[i])->fwd_ptr;
  }

  *free_ = free;
}

void
mndb_mem_compact_step_fwd(mndb_mem_t *mem)
{
  /* 2 - Forward pointers */

  MNDB_EACH_HEADER_BEGIN(mem)
    if(likely(header->fwd_func != NULL && header->refc > 0))
    {
      (*header->fwd_func)(mndb_mem_header_data(header));
    }
  MNDB_EACH_HEADER_END(mem)
}

void
mndb_mem_compact_step_move(mndb_mem_t *mem, uintptr_t free)
{
  MNDB_EACH_HEADER_BEGIN(mem)
    if(header->refc > 0)
    {
      header = (mndb_mem_header_t *) memmove(header->fwd_ptr, header, header->size);
    }
  MNDB_EACH_HEADER_END(mem)
  mem->cur -= free;
}

bool
mndb_mem_compact_step_shrink(mndb_mem_t *mem, uintptr_t free)
{
  bool shrunk = false;

  /* 4 - Shrink */
  if(unlikely(free / mem->size > .7))
  {
    mndb_mem_resize(mem, mem->size / 2);
    shrunk = true;
  }
  return shrunk;
}

static void
mndb_mem_compact(mndb_mem_t *mem, uint8_t *roots[], size_t len)
{
   uintptr_t free;

   mndb_mem_compact_step_compute_fwd_addrs(mem, roots, len, &free);
   mndb_mem_compact_step_fwd(mem);
   mndb_mem_compact_step_move(mem, free);

   if(mem->flags & MNDB_MEM_FLAGS_SHRINK)
   {
     mndb_mem_compact_step_shrink(mem, free);
   }
}

bool
mndb_mem_gc(mndb_mem_t *mem, uint8_t *roots[], size_t len)
{
  if(mem->flags & MNDB_MEM_FLAGS_MARK)
  {
    mndb_mem_mark_from_roots(mem, roots, len);
    mndb_mem_compact(mem, roots, len);
    return true;
  }
  else if(mem->flags & MNDB_MEM_FLAGS_COPY)
  {
    return mndb_mem_copy_from_roots(mem, roots, len);
  }
  else
  {
    mndb_mem_compact(mem, roots, len);
    return true;
  }
}


