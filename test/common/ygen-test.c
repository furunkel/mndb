#include "mndb.h"

#include "../test-helper.h"
#include <alloca.h>
#include <stdio.h>
#include <limits.h>

static int g_n_iterations = 5;
static int g_n_gcs = 8;
static int g_n_objs = 1024 * 6;
static long g_max_mem = 1024L * 1024L * 1024L * 2L;


static const char *const _mndb_log_tag = "ygen-test";

#include "../text.h"

static const mndb_ygen_flags_t g_flags[] = {
  MNDB_YGEN_FLAGS_NONE,
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
  int id;
  struct tree_obj_s *left;
  struct tree_obj_s *right;
  uint64_t cookie;
} tree_obj_t;

typedef struct cyclic_obj_s {
  int id;
  size_t next_len;
  uint64_t cookie;
  struct cyclic_obj_s *next[];
} cyclic_obj_t;


typedef bool (*obj_valid_func_t)(uint8_t *);



static flat_obj_t *
alloc_flat_obj(mndb_ygen_t *ygen, int id, size_t size, uint8_t *roots[], size_t roots_len)
{
  flat_obj_t *obj = (flat_obj_t *) mndb_ygen_alloc(ygen, sizeof(flat_obj_t) + size, 0, roots, roots_len);

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

  if(obj->cookie != 0xdeadbeefdeadbeef - (uint64_t)obj->id)
    return false;

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

  return valid;
}


static bool
is_cyclic_obj_valid(uint8_t *ptr)
{
  cyclic_obj_t *obj = (cyclic_obj_t *) ptr;
  return obj->cookie == 0xdeadbeefdeadbeef - (uint64_t)obj->id;
}

static void
copy_tree(mndb_ygen_t *ygen, uint8_t *data)
{
  tree_obj_t *tree = (tree_obj_t *) data;
  tree->left = (tree_obj_t *)mndb_ygen_copy(ygen, (uint8_t *)tree->left);
  tree->right = (tree_obj_t *)mndb_ygen_copy(ygen, (uint8_t *)tree->right);
}

static void
copy_cyclic_obj(mndb_ygen_t *ygen, uint8_t *data)
{
  cyclic_obj_t *obj = (cyclic_obj_t *) data;
  for(size_t i = 0; i < obj->next_len; i++)
  {
    obj->next[i] = (cyclic_obj_t *)mndb_ygen_copy(ygen, (uint8_t *) obj->next[i]);
  }
}

static mndb_ptr_stack_t *
alloc_random_graph(mndb_ygen_t *ygen, int size)
{
  mndb_ptr_stack_t *stack = mndb_ptr_stack_new((size_t)size);

  for(int i = 0; i < size; i++)
  {
    int degree = rand() % 256;
    cyclic_obj_t *obj = (cyclic_obj_t *) mndb_ygen_alloc(ygen, sizeof(cyclic_obj_t) + (size_t)degree * sizeof(cyclic_obj_t *),
                                                         0,
                                                         mndb_ptr_stack_data(stack),
                                                         mndb_ptr_stack_cur(stack));
    obj->cookie = 0xdeadbeefdeadbeef - (uint64_t)i;
    obj->next_len = (size_t) degree;
    obj->id = i;
    memset(obj->next, 0, obj->next_len * sizeof(cyclic_obj_t *));
    mndb_ptr_stack_push(stack, (uint8_t *)obj);
  }

  for(size_t j = 0; j < mndb_ptr_stack_cur(stack); j++)
  {
    cyclic_obj_t *obj = (cyclic_obj_t *) mndb_ptr_stack_at(stack, j);
    for(size_t k = 0; k < obj->next_len; k++)
    {
      if((int)k % (16 + (rand() % 32)))
      {
        obj->next[k] = obj;
      }
      else
      {
        obj->next[k] = (cyclic_obj_t *) mndb_ptr_stack_at(stack, (size_t)(rand() % (int)MIN(mndb_ptr_stack_cur(stack), j + 10)));
      }
    }
  }

  return stack;
}

static tree_obj_t *
alloc_random_tree_(tree_obj_t **root, mndb_ygen_t *ygen, mndb_ptr_stack_t *stack, int size, int id)
{
  if(size == 0) return NULL;

  tree_obj_t *tree = (tree_obj_t *) mndb_ygen_alloc(ygen, sizeof(tree_obj_t),
                                                   0,
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
  tree_obj_t *left_tree = alloc_random_tree_(root, ygen, stack, left, id);

  mndb_debug("pushing left tree %p", left_tree);
  mndb_ptr_stack_push(stack, (uint8_t *)left_tree);

  tree_obj_t *right_tree = alloc_random_tree_(root, ygen, stack, right, left == 0 ? id : (left_tree->id + 1));
  left_tree = (tree_obj_t *) mndb_ptr_stack_pop(stack);
  mndb_debug("popped left tree %p", left_tree);

  tree = (tree_obj_t *) mndb_ptr_stack_pop(stack);
  mndb_debug("popped tree %p", left_tree);

  tree->left = left_tree;
  tree->right = right_tree;
  tree->id = right == 0 ? (left == 0 ? id : tree->left->id + 1) : tree->right->id + 1;
  tree->cookie = 0xdeadbeefdeadbeef - (uint16_t)tree->id;

  mndb_debug("%p->left = %p", tree, tree->left);
  mndb_debug("%p->right = %p", tree, tree->right);

  return tree;
}

static tree_obj_t *
alloc_random_tree(mndb_ygen_t *ygen, int size)
{
  mndb_ptr_stack_t *stack = mndb_ptr_stack_new((size_t)size);
  tree_obj_t *retval = alloc_random_tree_(NULL, ygen, stack, size, 1);

  mndb_ptr_stack_free(stack);

  return retval;
}

static void *
count_headers(mndb_ygen_header_t *header, void *user_data)
{
  return (void *)((uintptr_t)user_data + 1);
}

static void
assert_header_count_equals(mndb_ygen_t *ygen, int count)
{
  uintptr_t actual_count = 0;
  actual_count = (uintptr_t) mndb_ygen_each_header(ygen, count_headers, (void *)actual_count);

  assert((int)actual_count == count);
}

static void *
find_root(mndb_ygen_header_t *header, void *user_data)
{
  if(user_data == NULL) return NULL;

  uint8_t **roots = (uint8_t **) user_data;
  size_t len = *((size_t *) roots);

  for(size_t i = 0; i < len; i++)
  {
    if(roots[i] == mndb_ygen_header_data(header))
      return user_data;
  }

  return NULL;
}

static void
assert_all_headers_are_roots(mndb_ygen_t *ygen, uint8_t *roots[], size_t len)
{
  uint8_t *data = alloca(sizeof(roots) + sizeof(len));
  *((uint8_t ***)data) = roots;
  *((size_t *)(data + sizeof(roots))) = len;

  assert(mndb_ygen_each_header(ygen, find_root, data));
}


static void *
check_header(mndb_ygen_header_t *header, void *user_data)
{
  if(user_data == NULL) return NULL;

  obj_valid_func_t func = (obj_valid_func_t) user_data;
  if((*func)(mndb_ygen_header_data(header)))
  {
    return user_data;
  }
  else
  {
    return NULL;
  }
}

static void
assert_all_headers_are_valid(mndb_ygen_t *ygen, obj_valid_func_t func)
{
  assert(mndb_ygen_each_header(ygen, check_header, func));
}

static void
test_mem_flat()
{
  printf("## TEST FLAT\n");

  mndb_ygen_t ygen;
  mndb_ygen_init(&ygen, 1024, MNDB_YGEN_FLAGS_NONE);

  for(int j = 0; j < g_n_gcs; j++)
  {
    uint8_t *roots[g_n_objs];
    for(int i = 0; i < g_n_objs; i++)
    {
      size_t size = ((size_t)rand() % (MNDB_ARY_LEN(text) - 32)) + 32 + sizeof(flat_obj_t);
      size = MIN(size, MNDB_YGEN_MAX_ALLOC_SIZE - MNDB_YGEN_MAX_ALLOC_SIZE / 8);
      if(mndb_ygen_cur(&ygen) >= (size_t)g_max_mem)
      {
        mndb_debug("memory limit reached, forcing GC...");
        break;
      }
      roots[i] = (uint8_t *)alloc_flat_obj(&ygen, i, size, roots, (size_t)i);
      assert((uintptr_t)roots[i] % MNDB_YGEN_ALIGN == 0);
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
    assert(mndb_ygen_gc(&ygen, roots, (size_t)n_roots));

    if(n_roots == 0) assert(mndb_ygen_cur(&ygen) == 0);

    assert_header_count_equals(&ygen, n_roots);
    assert_all_headers_are_valid(&ygen, is_flat_obj_valid);
    assert_all_headers_are_roots(&ygen, roots, (size_t)n_roots);
  }

  mndb_ygen_destroy(&ygen);
}


static void
test_mem_tree()
{
  printf("## TEST TREE\n");

  mndb_ygen_t ygen;
  mndb_ygen_init(&ygen, 24, MNDB_YGEN_FLAGS_NONE);
  mndb_ygen_register_copy_func(&ygen, copy_tree);

  for(int j = 0; j < g_n_gcs; j++)
  {
    uint8_t *roots[1];
    tree_obj_t *tree = alloc_random_tree(&ygen, g_n_objs);
    roots[0] = (uint8_t *) tree;

    if(j == 0) assert_header_count_equals(&ygen, g_n_objs);

    assert(mndb_ygen_gc(&ygen, roots, (size_t)1));
    tree = (tree_obj_t *) roots[0];
    assert_header_count_equals(&ygen, g_n_objs);
    assert(tree->id == g_n_objs);
    assert_all_headers_are_valid(&ygen, is_tree_obj_valid);
  }

  mndb_ygen_destroy(&ygen);
}

static void
mark_node(cyclic_obj_t *node, int comp_sizes[], int id)
{
  if(comp_sizes[node->id] != INT_MIN) return;

  if(node->id == id)
  {
    comp_sizes[node->id] = 0;
  }
  else
  {
    comp_sizes[node->id] = -id;
  }

  comp_sizes[id]++;
  mndb_debug("c %d", comp_sizes[id]);
  for(size_t i = 0; i < node->next_len; i++)
  {
    mark_node(node->next[i], comp_sizes, id);
  }
}

static void
test_mem_cyclic()
{
  printf("## TEST CYCLIC\n");

  mndb_ygen_t ygen;
  mndb_ygen_init(&ygen, 1024, MNDB_YGEN_FLAGS_NONE);
  mndb_ygen_register_copy_func(&ygen, copy_cyclic_obj);

  for(int j = 0; j < g_n_gcs; j++)
  {
    uint8_t *roots[1];
    mndb_ptr_stack_t *nodes = alloc_random_graph(&ygen, g_n_objs);
    int *comp_sizes = malloc((size_t)g_n_objs * sizeof(int));
    for(int i = 0; i < g_n_objs; i++) comp_sizes[i] = INT_MIN;

    for(size_t i = 0; i < mndb_ptr_stack_cur(nodes); i++)
    {
      cyclic_obj_t *node = (cyclic_obj_t *) mndb_ptr_stack_at(nodes, i);
      mndb_debug("marking %d", node->id);
      mark_node(node, comp_sizes, node->id);
    }

    cyclic_obj_t *root = NULL;
    int root_comp_size = -1;

    for(size_t i = 0; i < mndb_ptr_stack_cur(nodes); i++)
    {
      cyclic_obj_t *node = (cyclic_obj_t *) mndb_ptr_stack_at(nodes, i);
      mndb_debug("count %d -> %d", node->id,comp_sizes[node->id] );
      if(comp_sizes[node->id] > 0)
      {
        root = node;
        root_comp_size = comp_sizes[node->id];
        break;
      }
    }

    assert(root != NULL);
    assert(root_comp_size > 0);

    roots[0] = (uint8_t *) root;

    if(j == 0) assert_header_count_equals(&ygen, g_n_objs);

    assert(mndb_ygen_gc(&ygen, roots, (size_t)1));

    assert_header_count_equals(&ygen, root_comp_size);
    assert_all_headers_are_valid(&ygen, is_cyclic_obj_valid);

    mndb_ptr_stack_free(nodes);
    free(comp_sizes);
  }


  mndb_ygen_destroy(&ygen);
}



static void
test_mem()
{
  for(int i = 0; i < g_n_iterations; i++)
  {
    test_mem_flat();
    test_mem_tree();
    test_mem_cyclic();
  }
}



int
main(int argc, const char *argv[])
{
  mndb_init(&argc, &argv);

  if(argc > 1) g_n_iterations = atoi(argv[1]);
  if(argc > 2) g_n_objs = atoi(argv[2]);
  if(argc > 3) g_max_mem = atol(argv[3]);

  test_mem();
}


