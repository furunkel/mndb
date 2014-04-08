#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "mndb-mem.h"
#include "mndb-util.h"


#define MNDB_EACH_IDA_ENTRY_BEGIN(table) \
  for(uint8_t *entry = table->data;                                                    \
      entry < table->data + table->cur;                                                \
      entry += sizeof(uint8_t *))                                                      \
  {                                                                                    \
    if(unlikely((uintptr_t)entry % MNDB_IDA_TABLE_PAGE_SIZE == 0)) continue; \
                                                                                       \
    fprintf(stderr, "%s\n", __func__);\
    uint8_t *ptr = mndb_translate_ida(entry);                                           \
    fprintf(stderr, "/%s\n", __func__);\
    if(likely(!((uintptr_t)ptr & 1)))                                     \
    {

#define MNDB_EACH_IDA_ENTRY_END(table) \
    }                                       \
  }

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

static void
mndb_ida_table_init(mndb_ida_table_t *table, size_t size, mndb_mem_t *mem);

static void
mndb_ida_table_destroy(mndb_ida_table_t *table);

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

  // uninitialized
  mem->ida_table = NULL;
}


void
mndb_mem_destroy(mndb_mem_t *mem)
{
  if(mem->ida_table != NULL)
  {
    mndb_ida_table_destroy(mem->ida_table);
  }

  if(mem->data != NULL)
  {
    free(mem->data);
  }
}

void
mndb_ida_table_destroy(mndb_ida_table_t *table)
{
  if(table->data != NULL)
  {
    free(table->data);
  }
}

mndb_ida_table_t *
mndb_ida_table_alloc(mndb_mem_t *mem, size_t size)
{

  void *m = aligned_alloc(MNDB_IDA_TABLE_PAGE_SIZE, size + sizeof(mndb_ida_table_t));

  if(m != NULL)
  {
    mndb_ida_table_t *table = (mndb_ida_table_t *) m;
    table->mem = mem;
    table->size = size;
    table->cur = 0;
    table->next = NULL;
    return table;
  }
  else
  {
    fprintf(stderr, "WARN: initializing root table failed\n");
    return NULL;
  }
}

__attribute__((warn_unused_result)) static mndb_ida_table_t *
mndb_ida_table_grow(mndb_ida_table_t *table)
{
  mndb_ida_table_t *new_table = mndb_ida_table_alloc(table->mem, table->size * MNDB_IDA_TABLE_GROWTH_FACTOR);
  new_table->next = table;

  return new_table;
}

static void
mndb_ida_table_move(mndb_ida_table_t *table, ptrdiff_t amount)
{
  if(amount == 0) return;

  MNDB_EACH_IDA_ENTRY_BEGIN(table);

  fprintf(stderr, "move(%d) %p: %p -> %p\n", amount, entry, *(uint8_t**)entry, *(uint8_t **)entry + amount);
  *(uint8_t **)entry += amount;
  MNDB_EACH_IDA_ENTRY_END(table);
}

static bool
mndb_mem_resize(mndb_mem_t *mem, size_t size)
{
  uint8_t *new_data = realloc(mem->data, size);
  if(likely(new_data != NULL))
  {
    mndb_ida_table_move(mem->ida_table, new_data - mem->data);
    mem->size = size;
    mem->data = new_data;
    return true;
  }
  else
  {
    fprintf(stderr, "WARN: resizing mem failed\n");
    return false;
  }
}

static void
mndb_ida_table_forward(mndb_ida_table_t *table)
{
  MNDB_EACH_IDA_ENTRY_BEGIN(table);
  mndb_mem_header_t *header = mndb_mem_header(ptr);
  if(likely(header->fwd_ptr != NULL))
  {
    fprintf(stderr, "forwarding %p: %p -> %p\n", entry, *(uint8_t**)entry, header->fwd_ptr);
    *(uint8_t **)entry = mndb_mem_header_data((mndb_mem_header_t *)header->fwd_ptr);
  }
  MNDB_EACH_IDA_ENTRY_END(table);

}

void
mndb_ida_table_remove(mndb_ida_table_t *table, uint8_t *val)
{
  assert(val > table->data && val < table->data + table->size);
  if(table->free > 0)
  {
    *(uintptr_t *)val = table->free | 1;
  }
  else
  {
    *(uintptr_t *)val = 0 | 1;
  }
  table->free = val - table->data;
  assert(table->free < table->size);
}

uint8_t *
mndb_ida_table_find(mndb_ida_table_t *table, uint8_t *val)
{
  MNDB_EACH_IDA_ENTRY_BEGIN(table);
  if(*(uint8_t**)entry == val)
  {
    return entry;
  }
  MNDB_EACH_IDA_ENTRY_END(table);

  return NULL;
}

uint8_t *
mndb_ida_table_insert(mndb_ida_table_t *head_table, uint8_t *val)
{
  /* free cell */
  for(mndb_ida_table_t *table = head_table; table != NULL; table = table->next)
  {
    if(likely(table->free > 0))
    {
      uint8_t *head = table->data + table->free;
      uintptr_t new_free = (*(uintptr_t *)head) & ~1;

      /* pointers in free list should never
       * lie on page boundary
       */
      assert(((uintptr_t)head) % MNDB_IDA_TABLE_PAGE_SIZE != 0);

      *(uint8_t **)head = val;

      fprintf(stderr, "old_free: %u\n", table->free);
      fprintf(stderr, "new_free: %u\n", new_free);
      table->free = new_free;
      assert(table->free < table->size);

      return (uint8_t *)head;
    }
    /* no free cells but space at the end */
    else
    {
      uintptr_t new_cur;
      uintptr_t max_new_cur;

      /* Worst-case space requirement is 2 * sizeof(uint8_t *)
       * in case we hit a page boundary
       */
      new_cur = table->cur + sizeof(uint8_t *);
      max_new_cur = table->cur + 2 * sizeof(uint8_t *);

      if(unlikely(max_new_cur >= table->size))
      {
        if(likely(table->next != NULL)) continue;

        mndb_ida_table_grow(head_table);
        table = head_table;
        // table->cur changed
        new_cur = table->cur + sizeof(uint8_t *);
        max_new_cur = table->cur + 2 * sizeof(uint8_t *);
      }

      uint8_t **key = (uint8_t **)(table->data + table->cur);
      if(unlikely((uintptr_t)key % MNDB_IDA_TABLE_PAGE_SIZE == 0))
      {
        *key = (uint8_t *)table;
        key++;
        new_cur = max_new_cur;
      }

      fprintf(stderr, "ida_ptr: %p\n", key);
      fprintf(stderr, "data: %p\n", table->data);
      fprintf(stderr, "cur: %d\n", table->cur);
      fprintf(stderr, "cur: %d\n", table->cur & 1);

      assert(((uintptr_t)table->data & 1) == 0);
      assert((table->cur & 1) == 0);
      assert(((uintptr_t)val & 1) == 0);
      *key = val;

      assert((uintptr_t)key % sizeof(uint8_t) == 0);
      table->cur = new_cur;

      return (uint8_t *)key;
    }
  }

  assert(0);
  return NULL;
}

mndb_mem_header_t *
mndb_mem_header(uint8_t *ptr)
{
  return (mndb_mem_header_t *) (ptr - sizeof(mndb_mem_header_t));
}

mndb_ida_table_t *
mndb_get_ida_table(uint8_t *ptr)
{
  return *(mndb_ida_table_t **)((uintptr_t)ptr & ~(MNDB_IDA_TABLE_PAGE_SIZE - 1));
}

uint8_t *
mndb_translate_ida(uint8_t *entry)
{
  uint8_t *ptr = *((uint8_t **)entry);
  fprintf(stderr, "aaa %p\n", ptr);
  //assert(((uintptr_t)ptr & 1) == 0);
  return ptr;
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

void
mndb_unref_ida(uint8_t *ptr)
{
  mndb_mem_header_t *header = mndb_mem_header(mndb_translate_ida(ptr));
  mndb_ida_table_t *table = mndb_get_ida_table(ptr);
  mndb_mem_header_unref(header);
  if(unlikely(header->refc == 0))
  {
    mndb_ida_table_remove(table, ptr);
  }
}

void
mndb_ref_ida(uint8_t *ptr)
{
  mndb_mem_header_ref(mndb_mem_header(mndb_translate_ida(ptr)));
}

void
mndb_free_ida(uint8_t *ptr)
{
  fprintf(stderr, "free ida: %p\n", ptr);
  mndb_mem_header_t *header = mndb_mem_header(mndb_translate_ida(ptr));
  mndb_ida_table_t *table = mndb_get_ida_table(ptr);
  mndb_mem_header_free(header);
  mndb_ida_table_remove(table, ptr);

  fprintf(stderr, "/free ida: %p\n", ptr);
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
    mndb_mem_resize(mem, (size_t)(new_cur + (mem->size / 2)));
  }

  mndb_mem_header_t *header = (mndb_mem_header_t *)(mem->data + mem->cur);

  fprintf(stderr, "alloc header: %p\n", header);
  header->size      = size + sizeof(mndb_mem_header_t);
  header->refc      = (mem->flags & MNDB_MEM_FLAGS_MARK) ? 0 : 1;
  header->fwd_func  = fwd_func;
  header->mark_or_copy_func = mark_or_copy_func;
  header->fin_func = fin_func;
  header->fwd_ptr   = NULL;

  mem->cur = new_cur;

  return mndb_mem_header_data(header);
}



uint8_t *
mndb_mem_alloc_ida(mndb_mem_t *mem, size_t size,
                   mndb_mem_fwd_func_t fwd_func,
                   mndb_mem_mark_or_copy_func_t mark_or_copy_func,
                   mndb_mem_fin_func_t fin_func)
{
  uint8_t *data = mndb_mem_alloc(mem, size, fwd_func, mark_or_copy_func, fin_func);

  if(unlikely(mem->ida_table == NULL))
  {
    mem->ida_table = mndb_ida_table_alloc(mem, MNDB_INITIAL_IDA_TABLE_SIZE);
  }
  uint8_t *ida_ptr = mndb_ida_table_insert(mem->ida_table, data);

  fprintf(stderr, "alloc root: %p <- %p\n", ida_ptr, data);

  return ida_ptr;
}

void
mndb_mem_header_unref(mndb_mem_header_t *header)
{
  header->refc = MAX(0, header->refc - 1);
}

void
mndb_mem_header_ref(mndb_mem_header_t *header)
{
  header->refc++;
}

void
mndb_mem_header_free(mndb_mem_header_t *header)
{
  fprintf(stderr, "free header: %p\n", header);
  header->refc = 0;
}

uint8_t *
mndb_mem_header_data(mndb_mem_header_t *header)
{
  return (uint8_t *)header + sizeof(mndb_mem_header_t);
}

void
mndb_mem_mark_ida(mndb_mem_t *mem)
{
  uint8_t **i = (uint8_t **) mem->ida_table->data;
  while((uint8_t *)i < (mem->ida_table->data + mem->ida_table->size))
  {
    if(likely(*i != NULL))
    {
      mndb_mem_header_t *header = (mndb_mem_header_t *)*i;
      assert(header->mark_or_copy_func.copy_func != NULL);
      mndb_mem_mark(mndb_mem_header_data(header));
    }
    i++;
  }
}

static void
mndb_mem_mark_from_roots(mndb_mem_t *mem, uint8_t *roots[], size_t len)
{
  for(int i = 0; i < len; i++)
  {
    uint8_t *ptr = roots[i];
    mndb_mem_header_t *header = mndb_mem_header(ptr);

    assert(header->mark_or_copy_func.mark_func != NULL);
    mndb_mem_mark(mndb_mem_header_data(header));
  }
}


void
mndb_mem_copy_from_roots(mndb_mem_t *mem, uint8_t *roots[], size_t len)
{
  if(len == 0 || roots == NULL)
  {
    fprintf(stderr, "ERROR: no roots given");
    abort();
  }

  if(mem->data2 != NULL)
  {
    free(mem->data2);
  }

  mem->cur2 = 0;

  for(int i = 0; i < len; i++)
  {
    uint8_t *ptr = roots[i];
    mndb_mem_header_t *header = mndb_mem_header(ptr);

    assert(header->mark_or_copy_func.copy_func != NULL);
    mndb_mem_copy(mem, mndb_mem_header_data(header));
  }

  if(likely(mem->data2 != NULL))
  {
    uint8_t *data = mem->data;
    mem->data = mem->data2;
    free(data);

    MNDB_EACH_HEADER_BEGIN(mem)
      header->fwd_ptr = NULL;
    MNDB_EACH_HEADER_END(mem)

  }
  else
  {
    fprintf(stderr, "WARN: empty copy gc run\n");
  }
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
  if(unlikely(mem->data2 == NULL))
  {
    mem->data2 = malloc(mem->size);
    mem->cur2 = 0;
    if(unlikely(mem->data2 == NULL))
    {
      fprintf(stderr, "WARN: allocating to space failed\n");
      mem->size = 0;
      return ptr;
    }
  }
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
mndb_mem_each_header(mndb_mem_t *mem, mndb_mem_each_header_func_t cb)
{
  for(uint8_t *ptr = mem->data;
      ptr < mem->data + mem->cur;
      ptr = ptr + ((mndb_mem_header_t *)ptr)->size)
  {
    mndb_mem_header_t *header = (mndb_mem_header_t *) ptr;
    (*cb)(header, mem, mndb_mem_header_data(header));
  }
}

void
mndb_mem_each_ida_header(mndb_mem_t *mem, mndb_mem_each_header_func_t cb)
{
  if(mem->ida_table == NULL) return;

  mndb_ida_table_t *table = mem->ida_table;

  MNDB_EACH_IDA_ENTRY_BEGIN(table);
    mndb_mem_header_t *header = mndb_mem_header(ptr);
    (*cb)(header, mem, entry);
  MNDB_EACH_IDA_ENTRY_END(table);
}

void
_mndb_mem_compact_step_compute_fwd_addrs(mndb_mem_t *mem, uintptr_t *free_)
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
  *free_ = free;
}

void
_mndb_mem_compact_step_fwd(mndb_mem_t *mem)
{
  /* 2 - Forward pointers */

  MNDB_EACH_HEADER_BEGIN(mem)
    if(likely(header->fwd_func != NULL && header->refc > 0))
    {
      (*header->fwd_func)(mndb_mem_header_data(header));
    }
  MNDB_EACH_HEADER_END(mem)

  if(mem->ida_table != NULL)
  {
    mndb_ida_table_forward(mem->ida_table);
  }
}

void
_mndb_mem_compact_step_move(mndb_mem_t *mem, uintptr_t free)
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
_mndb_mem_compact_step_shrink(mndb_mem_t *mem, uintptr_t free)
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

void
mndb_mem_compact(mndb_mem_t *mem)
{
   uintptr_t free;

   _mndb_mem_compact_step_compute_fwd_addrs(mem, &free);
   _mndb_mem_compact_step_fwd(mem);
   _mndb_mem_compact_step_move(mem, free);

   if(mem->flags & MNDB_MEM_FLAGS_SHRINK)
   {
     _mndb_mem_compact_step_shrink(mem, free);
   }
}

void
mndb_mem_gc(mndb_mem_t *mem, uint8_t *roots[], size_t len)
{
  if(mem->flags & MNDB_MEM_FLAGS_MARK)
  {
    mndb_mem_mark_from_roots(mem, roots, len);
    mndb_mem_compact(mem);
  }
  else if(mem->flags & MNDB_MEM_FLAGS_COPY)
  {
    mndb_mem_copy_from_roots(mem, roots, len);
  }
  else
  {
    mndb_mem_compact(mem);
  }
}


