#include "common/mndb-mem.h"

#include "../test-helper.h"
#include <alloca.h>

static int g_n_iterations = 5;
static int g_n_objs = 1024 * 6;
static long g_max_mem = 1024L * 1024L * 1024L * 2L;


static const char *const _mndb_log_tag = "mem-test";

#include "../text.h"

static const mndb_mem_flags_t g_flags[] = {
  MNDB_MEM_FLAGS_NONE,
  MNDB_MEM_FLAGS_SHRINK,
};

typedef struct {
  int id;
  clock_t ts;
  size_t size;

  uint8_t f0;
  uint16_t f1;
  uint32_t f2;
  uint64_t f3;

  char data[];
} flat_obj_t;

typedef bool (*obj_valid_func_t)(uint8_t *);

enum {
  KEEP_ALL,
  KEEP_SOME,
  KEEP_NONE
};


static flat_obj_t *
alloc_flat_obj(mndb_mem_t *mem, int id, size_t size, uint8_t *roots[], size_t roots_len)
{
  flat_obj_t *obj = (flat_obj_t *) mndb_mem_alloc(mem, sizeof(flat_obj_t) + size, NULL, roots, roots_len);

  obj->id = id;
  obj->ts = clock();
  obj->size = size;

  obj->f0 = 0xab;
  obj->f1 = 0xcdef;
  obj->f2 = 0xabcdef + (uint32_t)id;
  obj->f3 = 0x0a1b3c4d5e6f - (uint64_t)id;

  strncpy(obj->data, text, size);

  return obj;
}

static bool
is_flat_obj_valid(uint8_t *ptr)
{
  flat_obj_t *obj = (flat_obj_t *) ptr;
  return obj->f0 == 0xab
         && obj->f1 == 0xcdef
         && obj->f2 == 0xabcdef + (uint32_t)obj->id
         && obj->f3 == 0x0a1b3c4d5e6f - (uint64_t)obj->id
         && !strncmp(obj->data, text, obj->size);
}

static bool
count_headers(mndb_mem_header_t *header, void *user_data)
{
  *((uintptr_t *)user_data) += 1;
  return true;
}

static void
assert_header_count_equals(mndb_mem_t *mem, int count)
{
  uintptr_t actual_count = 0;
  mndb_mem_each_header(mem, count_headers, &actual_count);

  assert((int)actual_count == count);
}


static bool
find_root(mndb_mem_header_t *header, void *user_data)
{
  uint8_t **roots = (uint8_t **) user_data;
  size_t len = *((size_t *) roots);

  for(size_t i = 0; i < len; i++)
  {
    if(roots[i] == mndb_mem_header_data(header))
      return true;
  }

  return false;
}

static void
assert_roots(mndb_mem_t *mem, uint8_t *roots[], size_t len)
{
  uint8_t *data = alloca(sizeof(roots) + sizeof(len));
  *((uint8_t ***)data) = roots;
  *((size_t *)(data + sizeof(roots))) = len;

  assert(mndb_mem_each_header(mem, find_root, data));
}


static bool
check_header(mndb_mem_header_t *header, void *user_data)
{
  obj_valid_func_t func = (obj_valid_func_t) user_data;
  return (*func)(mndb_mem_header_data(header));
}

static void
assert_headers_valid(mndb_mem_t *mem, obj_valid_func_t func)
{
  assert(mndb_mem_each_header(mem, check_header, func));
}

static void
test_gc_flat(mndb_mem_t *mem, mndb_mem_flags_t flags, int mode)
{
  size_t total = 0;
  uint8_t *roots[g_n_objs];
  for(int i = 0; i < g_n_objs; i++)
  {
    size_t size = ((size_t)rand() % (MNDB_ARY_LEN(text) - 32)) + 32;
    total += size + sizeof(flat_obj_t);
    if((int)total >= g_max_mem)
    {
      mndb_debug("memory limit reached, forcing GC...");
      break;
    }
    roots[i] = (uint8_t *)alloc_flat_obj(mem, i, size, roots, (size_t)i);
  }

  int n_roots;

  switch(mode)
  {
    case KEEP_NONE:
      n_roots = 0;
      break;
    case KEEP_ALL:
      n_roots = g_n_objs;
      break;
    case KEEP_SOME:
      n_roots = (rand() % (g_n_objs / 4)) + (rand() % (g_n_objs / 4));
      break;
    default:
      n_roots = 0;
      assert(0);
  }
  assert(mndb_mem_gc(mem, roots, (size_t)n_roots));

  if(mode == KEEP_NONE) assert(mndb_mem_used(mem) == 0);

  assert_header_count_equals(mem, n_roots);
  assert_headers_valid(mem, is_flat_obj_valid);

  if(mode == KEEP_SOME) assert_roots(mem, roots, (size_t)n_roots);

  (void) is_flat_obj_valid;
}

static void
test_gc()
{
  for(unsigned int i = 0; i < MNDB_ARY_LEN(g_flags); i++)
  {
    mndb_mem_t mem;
    mndb_mem_init(&mem, 1024, g_flags[i]);

    for(int j = 0; j < g_n_iterations; j++)
    {
      test_gc_flat(&mem, g_flags[i], KEEP_ALL);
      test_gc_flat(&mem, g_flags[i], KEEP_NONE);
      test_gc_flat(&mem, g_flags[i], KEEP_SOME);
    }

    mndb_mem_destroy(&mem);
  }
}



int
main(int argc, const char *argv[])
{
  if(argc > 1) g_n_iterations = atoi(argv[1]);
  if(argc > 2) g_n_objs = atoi(argv[2]);
  if(argc > 3) g_max_mem = atol(argv[3]);

  test_gc();
}


