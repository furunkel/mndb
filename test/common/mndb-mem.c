#include "common/mndb-mem.h"

#include "../test-helper.h"

static int n_iterations = 1024;
static int n_objs = 1024;


#include "../text.h"

static const mndb_mem_flags_t flags[] = {
  MNDB_MEM_FLAGS_MARK,
  MNDB_MEM_FLAGS_MARK | MNDB_MEM_FLAGS_SHRINK,
  MNDB_MEM_FLAGS_COPY,
  MNDB_MEM_FLAGS_COPY | MNDB_MEM_FLAGS_SHRINK
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



static flat_obj_t *
  alloc_flat_obj(mndb_mem_t *mem, int id, size_t size)
{
  mndb_mem_mark_or_copy_func_t copy_func = {.copy_func = NULL};
  flat_obj_t *obj = (flat_obj_t *) mndb_mem_alloc(mem, sizeof(flat_obj_t) + size, NULL, copy_func, NULL);

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
is_flat_obj_valid(flat_obj_t *obj)
{
  return obj->f0 == 0xab
         && obj->f1 == 0xcdef
         && obj->f2 == 0xabcdef + (uint32_t)obj->id
         && obj->f3 == 0x0a1b3c4d5e6f - (uint64_t)obj->id
         && !strncmp(obj->data, text, obj->size);
}

static void
test_mark_gc_keep_none(mndb_mem_t *mem)
{
  for(int i = 0; i < n_objs; i++)
  {
    alloc_flat_obj(mem, i, ((size_t)rand() % (MNDB_ARY_LEN(text) - 32)) + 32);
  }
  assert(mndb_mem_gc(mem, NULL, 0));
}

static void
test_mark_gc()
{
  for(unsigned int i = 0; i < MNDB_ARY_LEN(flags); i++)
  {
    mndb_mem_t mem;
    mndb_mem_init(&mem, 1024, flags[i]);

    for(int j = 0; j < n_iterations; j++)
    {
      test_mark_gc_keep_none(&mem);
      //test_mark_gc_keep_one(&mem);
      //test_mark_gc_keep_all(&mem);
      //test_mark_gc_keep_some(&mem);
    }
  }
}



int
main(int argc, const char *argv[])
{
  if(argc > 1) n_iterations = atoi(argv[1]);
  if(argc > 2) n_objs = atoi(argv[2]);

  test_mark_gc();
  //test_copy_gc();
}


