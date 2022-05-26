#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 
 * Needed DS:
 * - std::unordered_map<>
 *  - insertion
 *  - deletion
 *  - search
 *
 *
 *
 *
 * */

/*
 * Enhancements:
 * - allocate bigger arrays for every node of the map
 * - think about a map for the outer index
 *
 * */

/* DEFINES --------------------------------------------------------------------------------------------------------------- */
#define DEFAULT_MEMORY_BLOCK            (1024u * 10000u)
#define DEFAULT_MAP_BLOCK_LIST_LEN       1024u
#define NOT_NULL_PRT(P)                 P != NULL
#define LOG_I(P)                        printf("[INFO] : %s\n",P);
#define LOG_E(P)                        printf("[ERROR]: %s\n",P);
#define LOG_INT(M,V)                    printf("[INFO] : %s %u\n", M, V);
#define LOG_HEX(M,V)                    printf("[INFO] : %s 0x%lx\n", M, V);

/* ALIAS ----------------------------------------------------------------------------------------------------------------- */
typedef int32_t         _i;
typedef uint32_t        _ui;
typedef int64_t         _l;
typedef uint64_t        _ul;
typedef int8_t          _c;
typedef uint8_t         _uc;

/* STRUCT TYPEDEF -------------------------------------------------------------------------------------------------------- */
/**
* @brief allocate a generic memory block;
*/
typedef struct S_MEMORY_BLOCK
{
  _ul size;       /* size if the memory block */
  _uc *start;     /* start memory pointer */
  _uc *current;   /* current block pointer */
  _uc *end;       /* end memory pointer */
 
  void(*init)(struct S_MEMORY_BLOCK *block, _ul size);
  void(*deinit)(struct S_MEMORY_BLOCK *block);
  _uc*(*get_block)(struct S_MEMORY_BLOCK *block, _ul size);
} MEMORY_BLOCK;

/**
* @brief            array style hash map to store CSTR
* @param start_ptr  the starting point of the string
* @param end_ptr    the ending point of the string
*/
typedef struct S_STR_LIMITS
{
  _uc *start_ptr; /* string start pointer */
  _uc *end_ptr;   /* string end pointer */
} STR_LIMITS;

/**
* @brief            single node or list head of MAP 
* @param str_limits the struct with pointers of the string limits
* @param next       the next string in case of collision 
*/
typedef struct S_MAP_NODE
{
  struct S_STR_LIMITS str_limits; /* struct containing the string bounds */ 
  struct S_MAP_NODE   *next;      /* pointer to the next node */
} MAP_NODE;

/**
* @brief            intermediate node containing an array of MAP_NODE 
* @param id         the id of the list. this can be used to retrieve the hashed family (hash value / 2^10)
* @param node_list  a fixed C array containig MAP_NODE pointers 
* @param start      pointer to the start of the array
* @param next       pointer to the next element
*/
typedef struct S_MAP_BLOCK_LIST
{
  _ui                    id;                                      /* hash ID, aka block ID */
  MAP_NODE               *node_list[DEFAULT_MAP_BLOCK_LIST_LEN];   /* MAP_NODE pointer array */
  struct S_MAP_BLOCK_LIST *next;
  void(*insert)(const _ui index, const _uc *key, struct S_MAP_BLOCK_LIST*, MEMORY_BLOCK*, MEMORY_BLOCK*);
  void(*delete)(const _ui index,const _uc *key, struct S_MAP_BLOCK_LIST*);
} MAP_BLOCK_LIST;

/**
 * @brief                 custom hash map with C style arrays
 * @param list_size       the linked list size         
 * @param current.list    the current active node for MAP NODE 
 * @param *lists          a C linked list each containing an array of MAP_NODE struct 
 */
typedef struct S_MAP
{
  _ui             list_size; /* total blocks */
  struct 
  {
    MAP_BLOCK_LIST *list;     /* block head */
  } current;
  MAP_BLOCK_LIST   *lists;    /* block of 1024 MAP_NODE */

  void(*init)(struct S_MAP*, struct S_MEMORY_BLOCK*);
  void(*deinit)(struct S_MAP*);
  void(*insert)(struct S_MAP*, _ui, struct S_MEMORY_BLOCK*);
  void(*insert_key)(struct S_MAP*, const _uc*, MEMORY_BLOCK *, MEMORY_BLOCK*);
  void(*remove_key)(struct S_MAP*, const _uc*);
  void(*find_key)(struct S_MAP*, const _uc*);
} MAP;

/* LIBRARY FUNCTION DEFINITION ------------------------------------------------------------------------------------------- */
static void exit_if_dirty(_uc *ptr)
{
  if (NOT_NULL_PRT(ptr))
  {
    LOG_E("Pointer failure, not null during init\n")
    exit(EXIT_FAILURE);
  }
}

static void f_insert_inner_map_node_list(const _ui index,
                                         const _uc *key,
                                         MAP_BLOCK_LIST *map_node,
                                         MEMORY_BLOCK *mem_block_map_nodes_keys,
                                         MEMORY_BLOCK *mem_block_map_blocks)
{
  LOG_INT("Allocating string for hash index: ", index);
  assert(index < 1024);
  assert(map_node != 0);
  assert(mem_block_map_nodes_keys != 0);
  assert(key != 0);

  /* allocate node */
  /* TODO: use mem block */
  //MAP_NODE *new_node = calloc(1, sizeof(MAP_NODE));
  _uc *mem_start = mem_block_map_blocks->get_block(mem_block_map_blocks, sizeof(MAP_NODE));
  MAP_NODE *new_node = (MAP_NODE*)mem_start; 
  if (!new_node)
  {
    LOG_E("Allocation error");
    assert(new_node);
    exit(EXIT_FAILURE);
  }
  new_node->next = 0;
  /* allocate string */
  mem_start = mem_block_map_nodes_keys->get_block(mem_block_map_nodes_keys, strlen((const char*)key)+1);     
  new_node->str_limits.start_ptr = mem_start;
  new_node->str_limits.end_ptr = mem_block_map_nodes_keys->current-1;
  strcpy((char*)new_node->str_limits.start_ptr, (const char*)key);
  assert(strcmp((const char*)new_node->str_limits.start_ptr, (const char*)key) == 0);

  /* new head */
  if (map_node->node_list[index] == NULL)
  {
    map_node->node_list[index] = new_node;
  }
  else
  {
    new_node->next = map_node->node_list[index];
    map_node->node_list[index] = new_node;
  }
}

static void f_delete_inner_map_node_list(const _ui index, const _uc *key, MAP_BLOCK_LIST *map_node)
{
  assert(index >= 0 && index < 1024);
  assert(key);
  assert(map_node);
  MAP_NODE *finder = map_node->node_list[index];
  MAP_NODE *finder_prev = NULL;
  /* scan the linked list at the index calculated by the hash */
  while (finder)
  {
    if (memcmp((const void*)finder->str_limits.start_ptr, (const void*)key, strlen((const char*)key)) == 0)
    {
      /* head remove */
      if (!finder_prev)
      {
        map_node->node_list[index] = finder->next; 
        free(finder);
        return;
      }
      /* otherwise */
      else 
      {
        finder_prev->next = finder->next;
        free(finder);
        return;
      }
    }
    finder = finder->next;
  }
}

static void f_init_memory_block(MEMORY_BLOCK *block, _ul size)
{
  exit_if_dirty(block->start);
  block->start = (_uc*)calloc(size, sizeof(_uc));
  block->current = block->start;
  block->end = block->start + size;
  LOG_I("Allocation memory block passed.")
}

static void f_deinit_memory_block(MEMORY_BLOCK *block)
{
  free(block->start);
  LOG_I("Memory block deallocation complete");
}

static _uc *f_get_memory_block(MEMORY_BLOCK *block, _ul size)
{
  assert(block !=0 );
  assert(size > 0);
  if (block->current+size > block->end)
  {
    LOG_E("Out of memory block, allocate new block");
    exit(EXIT_SUCCESS);
  }
  else
  {
    _uc *ret = block->current;
    block->current += size;
    LOG_HEX("Memory block used:", block->current-ret);
    return ret;
  }
}


static void f_init_map(MAP *map, MEMORY_BLOCK *memory_block)
{
  _uc *starting_point = memory_block->get_block(memory_block, sizeof(MAP_BLOCK_LIST));
  //map->lists = calloc(1, sizeof(MAP_BLOCK_LIST));
  map->lists = (MAP_BLOCK_LIST*)starting_point;
  if (!map->lists)
  {
    LOG_E("Allocation error");
    exit(EXIT_FAILURE);
  }
  for (_ui i = 0; i < DEFAULT_MAP_BLOCK_LIST_LEN; i++) map->lists->node_list[i] = NULL;
  map->current.list = map->lists;
  map->lists->insert = f_insert_inner_map_node_list;
  map->lists->delete = f_delete_inner_map_node_list;
  map->list_size++;
}

static void f_deinit_map(MAP *map)
{
  MAP_BLOCK_LIST *finder = map->lists;
  MAP_BLOCK_LIST *prev = NULL;
  while (finder)
  {
    LOG_INT("Deallocating id:", finder->id);
    /* deinit inner allocations */
    MAP_NODE *curr_map_node_list_head = NULL;
    for (_ui i = 0; i < DEFAULT_MAP_BLOCK_LIST_LEN; i++)
    {
      curr_map_node_list_head = finder->node_list[i];
      if (curr_map_node_list_head)
      {
        MAP_NODE *curr_i = curr_map_node_list_head;
        MAP_NODE *prev_i = NULL;
        while(curr_i)
        {
          LOG_I("Deallocating str:");
          LOG_I(curr_i->str_limits.start_ptr);
          if (prev_i) free(prev_i); 
          prev_i = curr_i;
          curr_i = curr_i->next;
        }
        if (prev_i) free(prev_i);
      }
    }
    
    /* deinit outer allocation */
    if (prev) free(prev);
    prev = finder;
    finder = finder->next;
  }
  if (prev) free(prev);
  LOG_I("Map deallocation complete");
}

static void f_insert_map_node_list(MAP *map, _ui id, MEMORY_BLOCK *memory_block)
{
  _uc *starting_point = memory_block->get_block(memory_block, sizeof(MAP_BLOCK_LIST));
  //map->lists = calloc(1, sizeof(MAP_BLOCK_LIST));
  MAP_BLOCK_LIST *new_node = (MAP_BLOCK_LIST*)starting_point;
  /* TODO: get mem block from mem block */
  //MAP_BLOCK_LIST *new_node = calloc(1, sizeof(MAP_BLOCK_LIST));
  if (!new_node)
  {
    LOG_E("Allocation error");
    exit(EXIT_FAILURE);
  }
  new_node->id = id;
  new_node->insert = f_insert_inner_map_node_list;
  new_node->delete = f_delete_inner_map_node_list;
  for (_ui i = 0; i < DEFAULT_MAP_BLOCK_LIST_LEN; i++) new_node->node_list[i] = NULL;

  /* this control can be avoided */
  if (map->lists == NULL)
  {
    map->lists = new_node;
    map->current.list = map->lists;
  }
  else
  {  
    map->current.list->next = new_node;
    map->current.list = new_node;
  }
  map->list_size++;
}

/* TO REVIEW */
static _ui hash(const _uc *key, size_t length) {
  size_t i = 0;
  _ui hash = 0;
  while (i != length) {
    hash += key[i++];
    hash += hash << 10;
    hash ^= hash >> 6;
  }
  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;
  return hash;
}

static void f_map_insert_key_internal(const _ui which_block,
                                      const _ui which_index_in_block,
                                      MAP *map, const _uc *key,
                                      const size_t key_len,
                                      MEMORY_BLOCK *mem_block_map_nodes_keys,
                                      MEMORY_BLOCK *mem_block_map_blocks)
{
  LOG_INT("Test for key:", which_block);
  /* scan all the node blocks */ 
  MAP_BLOCK_LIST *finder = map->lists;
  while (finder)
  {
    if (finder->id == which_block)
    {
      LOG_I("Old, insert internal");
      finder->insert(which_index_in_block, key, finder, mem_block_map_nodes_keys, mem_block_map_blocks);
      return;
    }
    finder = finder->next;
  }
  /* if not present, add the new block to the end of the list (current pointer) */
  map->insert(map, which_block, mem_block_map_blocks);
  LOG_I("Allocated new, insert internal");
  map->current.list->insert(which_index_in_block, key, map->current.list, mem_block_map_nodes_keys, mem_block_map_blocks);
}

static void f_map_insert_key(MAP *map, const _uc *key, MEMORY_BLOCK *mem_block_map_nodes_keys, MEMORY_BLOCK *mem_block_map_blocks)
{
  const size_t len = strlen((const char*)key);
  const _ui hashed = hash((const _uc*)key, len); 
  const _ui which_block = (_ui)hashed >> 10;
  const _ui which_index_in_block = (_ui)hashed%DEFAULT_MAP_BLOCK_LIST_LEN;

  f_map_insert_key_internal(which_block, which_index_in_block, map, key, len, mem_block_map_nodes_keys, mem_block_map_blocks);
}

static MAP_BLOCK_LIST *f_get_block_from_id(_ui which_block, MAP* map)
{
  MAP_BLOCK_LIST *finder = map->lists;
  while(finder)
  {
    if (finder->id == which_block) return finder;
    finder = finder->next;
  }
  return NULL;
}

static void f_map_delete_key_internal(const _ui which_block, const _ui which_index_in_block, MAP *map, const _uc *key)
{
  MAP_BLOCK_LIST *block = f_get_block_from_id(which_block, map);
  assert(block);

  block->delete(which_index_in_block, key, block);
}

static void f_map_remove_key(MAP *map, const _uc *key)
{
  const size_t len = strlen((const char*)key);
  const _ui hashed = hash((const _uc*)key, len); 
  const _ui which_block = (_ui)hashed >> 10;
  const _ui which_index_in_block = (_ui)hashed%DEFAULT_MAP_BLOCK_LIST_LEN;
 
  f_map_delete_key_internal(which_block, which_index_in_block, map, key);
}

static void f_map_find_key_internal(MAP *map, const _ui which_block, const _ui which_index_in_block, const _uc *key)
{
  assert(which_index_in_block >= 0 && which_index_in_block < 1024);
  MAP_BLOCK_LIST *block = f_get_block_from_id(which_block, map);
  MAP_NODE *finder = block->node_list[which_index_in_block];
  size_t size = strlen((const char*)key);

  while (finder)
  {
    if (memcmp((void*)finder->str_limits.start_ptr, (void*)key, size) == 0)
    {
      LOG_I(key);
    }
    finder = finder->next;
  }
}

static void f_map_find_key(MAP *map, const _uc *key)
{
  const size_t len = strlen((const char*)key);
  const _ui hashed = hash((const _uc*)key, len); 
  const _ui which_block = (_ui)hashed >> 10;
  const _ui which_index_in_block = (_ui)hashed%DEFAULT_MAP_BLOCK_LIST_LEN;
  
  f_map_find_key_internal(map, which_block, which_index_in_block, key);
}

/* PROGRAM FUNCTIONS ----------------------------------------------------------------------------------------------------- */
void test(MAP *map, MEMORY_BLOCK *mem_block_map_nodes_keys, MEMORY_BLOCK *mem_block_map_blocks)
{
  const _uc a[] = "nvbgurw47t47t4tqwhofuhe";
  const _uc b[] = "bvdsbvbvbv245632bvdsvnf";
  const _uc c[] = "nvbgurvvd77__4tqwhofuhe";

  map->insert_key(map, a, mem_block_map_nodes_keys, mem_block_map_blocks);
  map->insert_key(map, b, mem_block_map_nodes_keys, mem_block_map_blocks);
  map->insert_key(map, c, mem_block_map_nodes_keys, mem_block_map_blocks);
  map->find_key(map, a);
  map->find_key(map, b);
  map->find_key(map, c);
}

static void get_char_map(_uc *key, _ui *map)
{
  memset((void*)key, 0, 256);
  for (_ui i = 0; i < strlen((const char*)key); i++)
  {
    map[key[i]]++;
  }
}


/* MAIN ------------------------------------------------------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
  /* init mem blocks */
  MEMORY_BLOCK mem_block_map_blocks = { .start = 0,
                                    .end = 0,
                                    .current = 0,
                                    .size = 0,
                                    .init = f_init_memory_block,
                                    .deinit = f_deinit_memory_block,
                                    .get_block = f_get_memory_block};
  MEMORY_BLOCK mem_block_map_nodes_keys = { .start = 0,
                                    .end = 0,
                                    .current = 0,
                                    .size = 0,
                                    .init = f_init_memory_block,
                                    .deinit = f_deinit_memory_block,
                                    .get_block = f_get_memory_block};
  mem_block_map_blocks.init(&mem_block_map_blocks, DEFAULT_MEMORY_BLOCK);
  mem_block_map_nodes_keys.init(&mem_block_map_nodes_keys, DEFAULT_MEMORY_BLOCK);
  
  /* init hash map */
  MAP map = { .init = f_init_map, 
              .deinit = f_deinit_map,
              .insert = f_insert_map_node_list,
              .insert_key = f_map_insert_key,
              .find_key = f_map_find_key, 
              .lists = NULL,
              .current.list = NULL};
  map.init(&map, &mem_block_map_blocks);

  /* test DSA */
  test(&map, &mem_block_map_nodes_keys, &mem_block_map_blocks);

  /* deallocate all sources */
  mem_block_map_blocks.deinit(&mem_block_map_blocks);
  mem_block_map_nodes_keys.deinit(&mem_block_map_nodes_keys);
  //map.deinit(&map);

  

  return 0;
}

