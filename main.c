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
#define NOT_FOUND                       0x0
#define WIN                             0x1
#define NOT_EXISTS                      0x2
#define WRONG_MATCH                     0x3
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
  MAP_NODE*(*find_key)(struct S_MAP*, const _uc*);
} MAP;

/**
 * @brief       custom trie for ordered print purposes
 */
typedef struct S_TRIE
{
  struct S_TRIE  *childs[256];  /* possible childs based on ACII, active status signaled by non zero pointer */
  void(*add_node)(struct S_TRIE*, MEMORY_BLOCK*, const _uc);
  void(*add_word)(struct S_TRIE*, MEMORY_BLOCK*, const _uc*, size_t, size_t);
} TRIE;

/* LIBRARY FUNCTION DEFINITION ------------------------------------------------------------------------------------------- */
static void f_add_word_trie(TRIE *root, MEMORY_BLOCK *mem_block_ds_blocks, const _uc *key, const size_t index, size_t size);

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
                                         MEMORY_BLOCK *mem_block_ds_blocks)
{
  LOG_INT("Allocating string for hash index: ", index);
  assert(index < 1024);
  assert(map_node != 0);
  assert(mem_block_map_nodes_keys != 0);
  assert(key != 0);

  _uc *mem_start = mem_block_ds_blocks->get_block(mem_block_ds_blocks, sizeof(MAP_NODE));
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
    //TODO add new memory
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
  MAP_BLOCK_LIST *new_node = (MAP_BLOCK_LIST*)starting_point;
  if (!new_node)
  {
    LOG_E("Allocation error");
    exit(EXIT_FAILURE);
  }
  new_node->id = id;
  new_node->insert = f_insert_inner_map_node_list;
  new_node->delete = f_delete_inner_map_node_list;

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
                                      MEMORY_BLOCK *mem_block_ds_blocks)
{
  LOG_INT("Test for key:", which_block);
  /* scan all the node blocks */ 
  MAP_BLOCK_LIST *finder = map->lists;
  while (finder)
  {
    if (finder->id == which_block)
    {
      LOG_I("Old, insert internal");
      finder->insert(which_index_in_block, key, finder, mem_block_map_nodes_keys, mem_block_ds_blocks);
      return;
    }
    finder = finder->next;
  }
  /* if not present, add the new block to the end of the list (current pointer) */
  map->insert(map, which_block, mem_block_ds_blocks);
  LOG_I("Allocated new, insert internal");
  map->current.list->insert(which_index_in_block, key, map->current.list, mem_block_map_nodes_keys, mem_block_ds_blocks);
}

static void f_map_insert_key(MAP *map, const _uc *key, MEMORY_BLOCK *mem_block_map_nodes_keys, MEMORY_BLOCK *mem_block_ds_blocks)
{
  const size_t len = strlen((const char*)key);
  const _ui hashed = hash((const _uc*)key, len); 
  const _ui which_block = (_ui)hashed >> 10;
  const _ui which_index_in_block = (_ui)hashed%DEFAULT_MAP_BLOCK_LIST_LEN;

  f_map_insert_key_internal(which_block, which_index_in_block, map, key, len, mem_block_map_nodes_keys, mem_block_ds_blocks);
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

static MAP_NODE *f_map_find_key_internal(MAP *map, const _ui which_block, const _ui which_index_in_block, const _uc *key)
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
      return finder;
    }
    finder = finder->next;
  }
  return NOT_FOUND;
}

static MAP_NODE *f_map_find_key(MAP *map, const _uc *key)
{
  const size_t len = strlen((const char*)key);
  const _ui hashed = hash((const _uc*)key, len); 
  const _ui which_block = (_ui)hashed >> 10;
  const _ui which_index_in_block = (_ui)hashed%DEFAULT_MAP_BLOCK_LIST_LEN;
  
  return f_map_find_key_internal(map, which_block, which_index_in_block, key);
}

static void f_add_trie_node(TRIE *root, MEMORY_BLOCK *mem_block_ds_blocks, const _uc key)
{
  _uc *new_node = mem_block_ds_blocks->get_block(mem_block_ds_blocks, sizeof(TRIE));
  assert(new_node);

  root->childs[key] = (TRIE*)new_node;
  root->childs[key]->add_node = f_add_trie_node;
  root->childs[key]->add_word = f_add_word_trie;
}

static void f_add_word_trie(TRIE *root, MEMORY_BLOCK *mem_block_ds_blocks, const _uc *key, const size_t index, size_t size)
{
  if (index >= size) return;
  //TODO: removable
  if (!root || !root->add_word || !root->add_node)
  {
    LOG_E("Trie error");
    exit(EXIT_FAILURE);
  }
  const uint8_t is_there_path = (root->childs[key[index]] != 0);
  if (!is_there_path)
  {
    /* allocate a new child */
    root->add_node(root, mem_block_ds_blocks, key[index]);
  }
  root->add_word(root->childs[key[index]], mem_block_ds_blocks, key, index+1, size);
}

/* PROGRAM FUNCTIONS ----------------------------------------------------------------------------------------------------- */
void test(MAP *map, TRIE *trie, MEMORY_BLOCK *mem_block_map_nodes_keys, MEMORY_BLOCK *mem_block_ds_blocks)
{
  const _uc a[] = "hello0world";
  const _uc b[] = "today-is_monday__";
  const _uc c[] = "358836796754876----jvfnvjn";

  map->insert_key(map, a, mem_block_map_nodes_keys, mem_block_ds_blocks);
  trie->add_word(trie, mem_block_ds_blocks, a, 0, strlen((const char*)a));
  map->insert_key(map, b, mem_block_map_nodes_keys, mem_block_ds_blocks);
  trie->add_word(trie, mem_block_ds_blocks, b, 0, strlen((const char*)b));
  map->insert_key(map, c, mem_block_map_nodes_keys, mem_block_ds_blocks);
  trie->add_word(trie, mem_block_ds_blocks, c, 0, strlen((const char*)c));
  MAP_NODE *found = 0;
  found = map->find_key(map, a);
  found = map->find_key(map, b);
  found = map->find_key(map, c);
}

static void get_char_map(const _uc *key, _ui *map)
{
  memset((void*)key, 0, 256);
  for (_ui i = 0; i < strlen((const char*)key); i++)
  {
    map[key[i]]++;
  }
}

static void format_match(const _uc *target, const _uc *test, _uc *format, const size_t size)
{
  _ui char_map_target[256] = {0};
  _ui char_map_test[256] = {0};
  get_char_map(target, char_map_target);
  get_char_map(test, char_map_test);
  
  memset((void*)format, 0, size);

  /* assign correct position */
  for (_ui i = 0; i < size; i++)
  {
    if (target[i] == test[i])
    {
      format[i] = '+';
      char_map_target[target[i]]--;
      char_map_test[test[i]]--;
    }
  }

  /* assign wrong index */
  for (_ui i = 0; i < size; i++)
  {
    if (format[i]) continue;
    if (char_map_target[test[i]] && char_map_test[test[i]])
    {
      char_map_target[test[i]]--;
      char_map_test[test[i]]--;
      format[i] = '|';
    }
  }

  /* assing wrong */
  for (_ui i = 0; i < size; i++)
  {
    if (!format[i]) format[i] = '/';
  }
}

static _ui solve(MAP *map, const _uc *target, const _uc *test, _uc *format)
{
  const size_t size = strlen((const char*)target);
  if (memcmp((const void*)target, (const void*)test, size) == 0)
  {
    printf("ok\n");
    return WIN;
  }
  else if (map->find_key(map, test) == NOT_FOUND)
  {
    printf("not_exists\n"); 
    return NOT_EXISTS;
  }
  else
  {
    format_match(target, test, format, size);
    printf("%s\n", format);
    return WRONG_MATCH;
  }
}

static TRIE *f_get_new_trie(MEMORY_BLOCK *mem_block_ds_blocks)
{
  _uc *new_node = mem_block_ds_blocks->get_block(mem_block_ds_blocks, sizeof(TRIE));
  TRIE *trie = (TRIE*)new_node;
  trie->add_node = f_add_trie_node;
  trie->add_word = f_add_word_trie;
  return trie;
}

/* GLOOBAL VARIABLES ----------------------------------------------------------------------------------------------------- */
const _uc c_str_start_match[]     = "+inizio_partita";
const _uc c_str_new_match[]       = "+nuova_partita";
const _uc c_str_start_add_words[] = "+inserisci_inizio";
const _uc c_str_end_add_words[]   = "+inserisci_fine";
const _uc c_str_print_filtered[]  = "+stampa_filtrate";



/* MAIN ------------------------------------------------------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
  /* init mem blocks */
  MEMORY_BLOCK mem_block_ds_blocks = { .start = 0,
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
  mem_block_ds_blocks.init(&mem_block_ds_blocks, DEFAULT_MEMORY_BLOCK);
  mem_block_map_nodes_keys.init(&mem_block_map_nodes_keys, DEFAULT_MEMORY_BLOCK);
  
  /* init hash map */
  MAP map = { .init = f_init_map, 
              .deinit = f_deinit_map,
              .insert = f_insert_map_node_list,
              .insert_key = f_map_insert_key,
              .find_key = f_map_find_key, 
              .lists = NULL,
              .current.list = NULL};
  map.init(&map, &mem_block_ds_blocks);

  /* init trie root */
  TRIE *trie = f_get_new_trie(&mem_block_ds_blocks);

  /* test DSA */
  test(&map, trie, &mem_block_map_nodes_keys, &mem_block_ds_blocks);

  /* deallocate all sources */
  mem_block_ds_blocks.deinit(&mem_block_ds_blocks);
  mem_block_map_nodes_keys.deinit(&mem_block_map_nodes_keys);

  return 0;
}

