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
#define DEFAULT_MAP_NODE_LIST_LEN       1024u
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
  _ul size;
  _ul free_size;
  _uc *start;
  _uc *current;
  _uc *end;
 
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
  _uc *start_ptr;
  _uc *end_ptr;
} STR_LIMITS;

/**
* @brief            single node or list head of MAP 
* @param str_limits the struct with pointers of the string limits
* @param next       the next string in case of collision 
*/
typedef struct S_MAP_NODE
{
  struct S_STR_LIMITS str_limits; 
  struct S_MAP_NODE   *next;
} MAP_NODE;

/**
* @brief            intermediate node containing an array of MAP_NODE 
* @param id         the id of the list. this can be used to retrieve the hashed family (hash value / 2^10)
* @param node_list  a fixed C array containig MAP_NODE pointers 
* @param start      pointer to the start of the array
* @param next       pointer to the next element
*/
typedef struct S_MAP_NODE_LIST
{
  _ui                    id;
  MAP_NODE               *node_list[DEFAULT_MAP_NODE_LIST_LEN];
  struct S_MAP_NODE_LIST *next;
  void(*insert)(const _ui index, const _uc *key, struct S_MAP_NODE_LIST*, MEMORY_BLOCK*);
  void(*delete)(struct S_MAP_NODE*);
} MAP_NODE_LIST;

/**
 * @brief                 custom hash map with C style arrays
 * @param list_size       the linked list size         
 * @param current.list    the current active node for MAP NODE 
 * @param *lists          a C linked list each containing an array of MAP_NODE struct 
 */
typedef struct S_MAP
{
  _ui             list_size;
  struct 
  {
    MAP_NODE_LIST *list;
  } current;
  MAP_NODE_LIST   *lists; /* block of 1024 MAP_NODE */

  void(*init)(struct S_MAP*);
  void(*deinit)(struct S_MAP*);
  void(*insert)(struct S_MAP*, _ui);
  void(*insert_key)(struct S_MAP*, const _uc*, MEMORY_BLOCK *);
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

static void f_insert_inner_map_node_list(const _ui index, const _uc *key, MAP_NODE_LIST *map_node, MEMORY_BLOCK *memory_block_map_nodes)
{
  LOG_INT("Allocating string for hash index: ", index);
  assert(index < 1024);
  assert(map_node != 0);
  assert(memory_block_map_nodes != 0);
  assert(key != 0);
  /* new head */
  if (map_node->node_list[index] == NULL)
  {
    /* TODO: use mem block */
    map_node->node_list[index] = calloc(1, sizeof(MAP_NODE));
    if (!map_node->node_list[index])
    {
      LOG_E("Allocation error");
      exit(EXIT_FAILURE);
    }
    map_node->node_list[index]->next = NULL;

    _uc *mem_start = memory_block_map_nodes->get_block(memory_block_map_nodes, strlen((const char*)key)+1);     
    map_node->node_list[index]->str_limits.start_ptr = mem_start;
    map_node->node_list[index]->str_limits.end_ptr = memory_block_map_nodes->current-1;
    strcpy((char*)map_node->node_list[index]->str_limits.start_ptr, (const char*)key);
    assert(strcmp((const char*)map_node->node_list[index]->str_limits.start_ptr, (const char*)key) == 0);
  }
  else
  {
    //TODO
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


static void f_init_map(MAP *map)
{
  /* the calloc call will set all to zero */
  map->lists = calloc(1, sizeof(MAP_NODE_LIST));
  if (!map->lists)
  {
    LOG_E("Allocation error");
    exit(EXIT_FAILURE);
  }
  for (_ui i = 0; i < DEFAULT_MAP_NODE_LIST_LEN; i++) map->lists->node_list[i] = NULL;
  map->current.list = map->lists;
  map->lists->insert = f_insert_inner_map_node_list;
  map->list_size++;
}

static void f_deinit_map(MAP *map)
{
  MAP_NODE_LIST *curr = map->lists;
  MAP_NODE_LIST *prev = NULL;
  while (curr)
  {
    LOG_INT("Deallocating id:", curr->id);
    /* deinit inner allocations */
    MAP_NODE *curr_map_node_list_head = NULL;
    for (_ui i = 0; i < DEFAULT_MAP_NODE_LIST_LEN; i++)
    {
      curr_map_node_list_head = curr->node_list[i];
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
    prev = curr;
    curr = curr->next;
  }
  if (prev) free(prev);
  LOG_I("Map deallocation complete");
}

static void f_insert_map_node_list(MAP *map, _ui id)
{
  /* TODO: get mem block from mem block */
  MAP_NODE_LIST *new_node = calloc(1, sizeof(MAP_NODE_LIST));
  if (!new_node)
  {
    LOG_E("Allocation error");
    exit(EXIT_FAILURE);
  }
  new_node->id = id;
  new_node->insert = f_insert_inner_map_node_list;
  for (_ui i = 0; i < DEFAULT_MAP_NODE_LIST_LEN; i++) new_node->node_list[i] = NULL;

  /* this control can be avoided */
  if (map->lists == NULL)
  {
    map->lists = new_node;
    map->current.list = map->lists;
  }
  else
  {    map->current.list->next = new_node;
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

static void f_map_insert_key_internal(const _ui which_node, const _ui which_index_in_node, MAP *map, const _uc *key, const size_t key_len, MEMORY_BLOCK *memory_block_map_nodes)
{
  LOG_INT("Test for key:", which_node);
  /* scan all the node blocks */ 
  MAP_NODE_LIST *curr = map->lists;
  while (curr)
  {
    if (curr->id == which_node)
    {
      LOG_I("Old, insert internal");
      curr->insert(which_index_in_node, key, curr, memory_block_map_nodes);
      return;
    }
    curr = curr->next;
  }
  /* if not present, add the new block to the end of the list (current pointer) */
  map->insert(map, which_node);
  LOG_I("Allocated new, insert internal");
  map->current.list->insert(which_index_in_node, key, map->current.list, memory_block_map_nodes);
}

static void f_map_insert_key(MAP *map, const _uc *key, MEMORY_BLOCK *memory_block_map_nodes)
{
  const size_t len = strlen((const char*)key);
  const _ui hashed = hash((const _uc*)key, len); 
  const _ui which_node = (_ui)hashed >> 10;
  const _ui which_index_in_node = (_ui)hashed%DEFAULT_MAP_NODE_LIST_LEN;

  f_map_insert_key_internal(which_node, which_index_in_node, map, key, len, memory_block_map_nodes);
}

/* PROGRAM FUNCTIONS ----------------------------------------------------------------------------------------------------- */
void test(MAP *map, MEMORY_BLOCK *memory_block_map_nodes)
{
  const _uc a[] = "nvbgurw47t47t4tqwhofuhe";
  const _uc b[] = "bvdsbvbvbv245632bvdsvnf";
  const _uc c[] = "nvbgurvvd77__4tqwhofuhe";

  map->insert_key(map, a, memory_block_map_nodes);
  map->insert_key(map, b, memory_block_map_nodes);
  map->insert_key(map, c, memory_block_map_nodes);
}


/* MAIN ------------------------------------------------------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
  /* init mem blocks */
  MEMORY_BLOCK memory_block_map = { .start = 0, .end = 0, .current = 0, .size = 0, .free_size = 0, .init = f_init_memory_block, .deinit = f_deinit_memory_block, .get_block = f_get_memory_block};
  MEMORY_BLOCK memory_block_map_nodes = { .start = 0, .end = 0, .current = 0, .size = 0, .free_size = 0, .init = f_init_memory_block, .deinit = f_deinit_memory_block, .get_block = f_get_memory_block};
  memory_block_map.init(&memory_block_map, DEFAULT_MEMORY_BLOCK);
  memory_block_map_nodes.init(&memory_block_map_nodes, DEFAULT_MEMORY_BLOCK);
  
  /* init hash map */
  MAP map = {.init = f_init_map, .deinit = f_deinit_map, .insert = f_insert_map_node_list, .insert_key = f_map_insert_key, .lists = NULL, .current.list = NULL};
  map.init(&map);

  test(&map, &memory_block_map_nodes);

  /* deallocate all sources */
  memory_block_map.deinit(&memory_block_map);
  memory_block_map_nodes.deinit(&memory_block_map_nodes);
  map.deinit(&map);


  return 0;
}

