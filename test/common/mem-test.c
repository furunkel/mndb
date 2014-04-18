#include "common/mndb-mem.h"
#include "common/mndb-ptr-stack.h"

#include "../test-helper.h"
#include <alloca.h>
#include <stdio.h>

static int g_n_iterations = 5;
static int g_n_gcs = 8;
static int g_n_objs = 1024 * 6;
static long g_max_mem = 1024L * 1024L * 1024L * 2L;


static const char *const _mndb_log_tag = "mem-test";

#include "../text.h"

static const mndb_mem_flags_t g_flags[] = {
  MNDB_MEM_FLAGS_NONE,
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

typedef struct tree_obj_s {
  struct tree_obj_s *left;
  struct tree_obj_s *right;
  int id;
} tree_obj_t;

typedef struct cyclic_obj_s {
  int id;
  size_t next_len;
  struct tree_obj_s *next[];
} cyclic_obj_t;


typedef bool (*obj_valid_func_t)(uint8_t *);



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
is_tree_obj_valid(uint8_t *ptr)
{
  tree_obj_t *obj = (tree_obj_t *) ptr;
  bool valid;

  if(obj->left == NULL && obj->right == NULL)
  {
    valid = true;
  }
  else if(obj->left != NULL && obj->right == NULL)
  {
    valid = obj->id == obj->left->id + 1;
  }
  else
  {
    valid = obj->id == obj->right->id + 1;
  }

  assert(valid);
  return valid;
}

static void
copy_tree(mndb_mem_t *mem, uint8_t *data)
{
  tree_obj_t *tree = (tree_obj_t *) data;
  tree->left = (tree_obj_t *)mndb_mem_copy(mem, (uint8_t *)tree->left);
  tree->right = (tree_obj_t *)mndb_mem_copy(mem, (uint8_t *)tree->right);
}

static tree_obj_t *
alloc_random_tree_(tree_obj_t **root, mndb_mem_t *mem, mndb_ptr_stack_t *stack, int size, int id)
{
  if(size == 0) return NULL;

  tree_obj_t *tree = (tree_obj_t *) mndb_mem_alloc(mem, sizeof(tree_obj_t),
                                                   copy_tree,
                                                   mndb_ptr_stack_data(stack),
                                                   mndb_ptr_stack_cur(stack));
  tree->id = 0;
  tree->left = NULL;
  tree->right = NULL;

  if(root == NULL) root = &tree;


  int new_size = size - 1;
  int left;
  int right;
  if(new_size > 0)
  {
    left = (new_size / 2) + ((rand() % (new_size)) - new_size / 2);
    right = new_size - left;
  }
  else
  {
    left = 0;
    right = 0;
  }

  mndb_debug("pushing tree %p", tree);
  mndb_ptr_stack_push(stack, (uint8_t *)tree);
  tree_obj_t *left_tree = alloc_random_tree_(root, mem, stack, left, id);

  mndb_debug("pushing left tree %p", left_tree);
  mndb_ptr_stack_push(stack, (uint8_t *)left_tree);

  tree_obj_t *right_tree = alloc_random_tree_(root, mem, stack, right, left == 0 ? id : (left_tree->id + 1));
  left_tree = (tree_obj_t *) mndb_ptr_stack_pop(stack);
  mndb_debug("popped left tree %p", left_tree);

  tree = (tree_obj_t *) mndb_ptr_stack_pop(stack);
  mndb_debug("popped tree %p", left_tree);

  tree->left = left_tree;
  tree->right = right_tree;
  tree->id = right == 0 ? (left == 0 ? id : tree->left->id + 1) : tree->right->id + 1;

  mndb_debug("%p->left = %p", tree, tree->left);
  mndb_debug("%p->right = %p", tree, tree->right);

  return tree;
}

static tree_obj_t *
alloc_random_tree(mndb_mem_t *mem, int size)
{
  mndb_ptr_stack_t *stack = mndb_ptr_stack_new(1024 * 7);
  tree_obj_t *retval = alloc_random_tree_(NULL, mem, stack, size, 1);

  mndb_ptr_stack_free(stack);

  return retval;
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
assert_all_headers_are_roots(mndb_mem_t *mem, uint8_t *roots[], size_t len)
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
assert_all_headers_are_valid(mndb_mem_t *mem, obj_valid_func_t func)
{
  assert(mndb_mem_each_header(mem, check_header, func));
}

static void
test_mem_flat()
{
  printf("## TEST FLAT\n");

  mndb_mem_t mem;
  mndb_mem_init(&mem, 1024, MNDB_MEM_FLAGS_NONE);

  for(int j = 0; j < g_n_gcs; j++)
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
      roots[i] = (uint8_t *)alloc_flat_obj(&mem, i, size, roots, (size_t)i);
    }

    int n_roots;

    switch(j % 3)
    {
      case 0:
        n_roots = 0;
        break;
      case 1:
        n_roots = g_n_objs;
        break;
      case 2:
        n_roots = (rand() % (g_n_objs / 4)) + (rand() % (g_n_objs / 4));
        break;
      default:
        n_roots = 0;
        assert(0);
    }
    assert(mndb_mem_gc(&mem, roots, (size_t)n_roots));

    if(n_roots == 0) assert(mndb_mem_used(&mem) == 0);

    assert_header_count_equals(&mem, n_roots);
    assert_all_headers_are_valid(&mem, is_flat_obj_valid);
    assert_all_headers_are_roots(&mem, roots, (size_t)n_roots);
  }

  mndb_mem_destroy(&mem);
}


static void
test_mem_tree()
{
  printf("## TEST TREE\n");

  mndb_mem_t mem;
  mndb_mem_init(&mem, 24, MNDB_MEM_FLAGS_NONE);

  for(int j = 0; j < g_n_gcs; j++)
  {
    uint8_t *roots[1];
    tree_obj_t *tree = alloc_random_tree(&mem, g_n_objs);
    roots[0] = (uint8_t *) tree;
    assert(mndb_mem_gc(&mem, roots, (size_t)1));
    tree = (tree_obj_t *) roots[0];
    assert_header_count_equals(&mem, g_n_objs);
    assert(tree->id == g_n_objs);
    assert_all_headers_are_valid(&mem, is_tree_obj_valid);
  }

  mndb_mem_destroy(&mem);
}

static void
test_mem_cyclic()
{
  printf("## TEST CYCLIC\n");

  mndb_mem_t mem;
  mndb_mem_init(&mem, 1024, MNDB_MEM_FLAGS_NONE);

  for(int j = 0; j < g_n_gcs; j++)
  {
    uint8_t *roots[1];
    tree_obj_t *tree = alloc_random_tree(&mem, g_n_objs);
    roots[0] = (uint8_t *) tree;
    assert(mndb_mem_gc(&mem, roots, (size_t)1));
    tree = (tree_obj_t *) roots[0];
    assert_header_count_equals(&mem, g_n_objs);
    assert(tree->id == g_n_objs);
    assert_all_headers_are_valid(&mem, is_tree_obj_valid);
  }

  mndb_mem_destroy(&mem);
}



static void
test_mem()
{
  for(int i = 0; i < g_n_iterations; i++)
  {
    test_mem_flat();
    test_mem_tree();
  }
}



int
main(int argc, const char *argv[])
{
  if(argc > 1) g_n_iterations = atoi(argv[1]);
  if(argc > 2) g_n_objs = atoi(argv[2]);
  if(argc > 3) g_max_mem = atol(argv[3]);

  test_mem();
}


