#include "common/mndb-mem.h"

#include "../test-helper.h"

typedef struct {
  clock_t ts;
  int8_t f0;
  int16_t f1;
  int32_t f2;
  int64_t f3;

  char fx[];
} obj_t;

static obj_t *
alloc_obj(mndb_mem_t *mem)
{
  obj_t *obj = (obj_t *) mndb_mem_alloc(mem, sizeof(obj_t) + 100, NULL, NULL, NULL);
}

static void
test_mark_gc()
{
  
}



int
main(int argc, const char *argv[])
{
  test_mark_gc();
  test_copy_gc();
}


