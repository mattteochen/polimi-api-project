/*  
 * @brief: API final project
 * @author: Kaixi Matteo Chen
 * @copyright: 2022 myself
 *
 * */

#include <assert.h>
#include <stdbool.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* DEFINES --------------------------------------------------------------------------------------------------------------- */

#define LOCAL_TEST 0

#define NOT_FOUND                       0x0
#define WIN                             0x1
#define NOT_EXISTS                      0x2
#define WRONG_MATCH                     0x3
#define TRIE_DELETE_KEY                 0x0
#define TRIE_MAINTAIN_KEY               0x1
#define DEFAULT_MEMORY_BLOCK            (1024u * 1u)
#define DEFAULT_MAP_BLOCK_LIST_LEN      1024u
#define NOT_NULL_PRT(P)                 P != NULL
#if LOCAL_TEST == 1
#define LOG_I(P)                        printf("[INFO] : %s\n",P);
#define LOG_E(P)                        printf("[ERROR]: %s\n",P);
#define LOG_INT(M,V)                    printf("[INFO] : %s %u\n", M, V);
#define LOG_HEX(M,V)                    printf("[INFO] : %s 0x%lx\n", M, V);
#define LOG_STR(M,V)                    printf("[INFO] : %s %s\n", M, V);
#else
#define LOG_I(P)    
#define LOG_E(P)    
#define LOG_INT(M,V)
#define LOG_HEX(M,V)
#define LOG_STR(M,V)
#endif

/* ALIAS ----------------------------------------------------------------------------------------------------------------- */
typedef int32_t         _i;
typedef uint32_t        _ui;
typedef int64_t         _l;
typedef uint64_t        _ul;
typedef int8_t          _c;
typedef uint8_t         _uc;

/* STRUCT TYPEDEF -------------------------------------------------------------------------------------------------------- */

typedef struct S_CHAR_COUNTER_INTERNAL
{
  _ui target_index;             /* the desired index where to find the char */
  struct S_CHAR_COUNTER_INTERNAL *next;  /* pointer to the next element of the list  */
} CHAR_COUNTER_INTERNAL;

typedef struct S_CHAR_COUNTER
{
  CHAR_COUNTER_INTERNAL *head;  /* head, a pointer to a C linked list */
  CHAR_COUNTER_INTERNAL *tail;  /* tail, a pointer to a C linked list */
  _ui counter;                  /* linked list node counter */
} CHAR_COUNTER;

/**
* @brief a generic memory block;
*/
typedef struct S_MEMORY_BLOCK_NODE
{
  _uc *start;                       /* start memory pointer */
  _uc *current;                     /* current block pointer */
  _uc *end;                         /* end memory pointer */
  struct S_MEMORY_BLOCK_NODE *next; /* a pointer to the next block */

  void(*allocate)(struct S_MEMORY_BLOCK_NODE*, const _ul);
} MEMORY_BLOCK_NODE;

typedef struct S_MEMORY_BLOCK
{
  MEMORY_BLOCK_NODE *finder; /* pointer to the current active block */  
  MEMORY_BLOCK_NODE *list;   /* a C linked list of MEMORY_BLOCK_NODE */ 

  void(*init)     (struct S_MEMORY_BLOCK*, _ul);
  void(*add_block)(struct S_MEMORY_BLOCK*, _ul);
  void(*deinit)   (struct S_MEMORY_BLOCK*);
  _uc*(*get_block)(struct S_MEMORY_BLOCK*, _ul);
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
  _ui                     id;                                       /* hash ID, aka block ID */
  MAP_NODE                *node_list[DEFAULT_MAP_BLOCK_LIST_LEN];   /* MAP_NODE pointer array */
  struct S_MAP_BLOCK_LIST *next;                                    /* a pointer to the next node */

  void(*insert)(const _ui, const _uc *, struct S_MAP_BLOCK_LIST*, MEMORY_BLOCK*);
  void(*delete)(const _ui, const _uc *, struct S_MAP_BLOCK_LIST*);
} MAP_BLOCK_LIST;

/**
 * @brief                 custom hash map with C style arrays
 * @param list_size       the linked list size         
 * @param current.list    the current active node for MAP NODE 
 * @param *lists          a C linked list each containing an array of MAP_NODE struct 
 */
typedef struct S_MAP
{
  _ui             list_size;  /* total blocks */
  struct 
  {
    MAP_BLOCK_LIST *list;     /* block head */
  } current;
  MAP_BLOCK_LIST   *lists;    /* block of 1024 MAP_NODE */

  void(*init)         (struct S_MAP*, struct S_MEMORY_BLOCK*);
  void(*deinit)       (struct S_MAP*);
  void(*insert)       (struct S_MAP*, _ui, struct S_MEMORY_BLOCK*);
  void(*insert_key)   (struct S_MAP*, const _uc*, MEMORY_BLOCK *);
  void(*remove_key)   (struct S_MAP*, const _uc*);
  MAP_NODE*(*find_key)(struct S_MAP*, const _uc*);
} MAP;

/**
 * @brief       custom trie for ordered print purposes
 */
typedef struct S_TRIE
{
  struct S_TRIE  *childs[256];  /* possible childs based on ACII, active status signaled by non zero pointer */
  _uc status[256];              /* signal if a path is active or not */

  void(*add_node)    (struct S_TRIE*, MEMORY_BLOCK*, const _uc);
  void(*insert_key)  (struct S_TRIE*, MEMORY_BLOCK*, const _uc*, size_t, size_t);
  void(*remove_key)  (struct S_TRIE*, const _uc*, const size_t);
  bool(*has_childs)  (struct S_TRIE*);
  bool(*find_key)    (struct S_TRIE*, const _uc*, const size_t, const size_t);
  void(*clean)        (struct S_TRIE*, 
                      int32_t*, 
                      _uc*, 
                      const _uc *format,
                      _uc *wrong_chars, 
                      _ui *min_counter, 
                      _ui *exact_char_counter, 
                      bool *min_counter_finalized,
                      CHAR_COUNTER *exact_char_pos,
                      CHAR_COUNTER *avoid_char_pos,
                      _ui*, 
                      const size_t);    
  void(*clean_status)(struct S_TRIE *);
} TRIE;

/* GLOOBAL VARIABLES ----------------------------------------------------------------------------------------------------- */
const _uc c_str_new_match[]         = "+nuova_partita";
const _uc c_str_start_insert_keys[] = "+inserisci_inizio";
const _uc c_str_end_insert_keys[]   = "+inserisci_fine";
const _uc c_str_print_filtered[]    = "+stampa_filtrate";

#if LOCAL_TEST == 1
FILE *fp;
#endif

/* LIBRARY FUNCTION DEFINITION ------------------------------------------------------------------------------------------- */
static void f_add_trie_node       (TRIE*, MEMORY_BLOCK*, const _uc);
static void f_insert_key_trie     (TRIE*, MEMORY_BLOCK*, const _uc*, const size_t, size_t);
static void f_remove_key_trie     (TRIE*, const _uc*, size_t);
static bool f_trie_node_has_child (TRIE*);
static bool f_trie_find_key       (TRIE*, const _uc*, const size_t, const size_t);
static void f_trie_clean_keys(TRIE *root,
                             int32_t *available,
                             _uc *buffer,
                             const _uc *format,
                             _uc *wrong_chars, 
                             _ui *min_counter, 
                             _ui *exact_char_counter, 
                             bool *min_counter_finalized,
                             CHAR_COUNTER *exact_char_pos,
                             CHAR_COUNTER *avoid_char_pos,
                             _ui *chars_map,
                             const size_t index);
static void f_trie_clean_status(TRIE *root);

static void f_insert_inner_map_node_list(const _ui index,
                                         const _uc *key,
                                         MAP_BLOCK_LIST *map_node,
                                         MEMORY_BLOCK *memory_block)
{
  LOG_INT("Allocating string for hash index: ", index);
  assert(index < 1024);
  assert(map_node != 0);
  assert(memory_block != 0);
  assert(key != 0);

  _uc *mem_start = memory_block->get_block(memory_block, sizeof(MAP_NODE));
  MAP_NODE *new_node = (MAP_NODE*)mem_start; 
  if (!new_node)
  {
    LOG_E("Allocation error");
    assert(new_node);
    exit(EXIT_FAILURE);
  }
  new_node->next = 0;
  /* allocate string */
  mem_start = memory_block->get_block(memory_block, strlen((const char*)key)+1); /* terminator! */    
  new_node->str_limits.start_ptr = mem_start;
  new_node->str_limits.end_ptr = memory_block->finder->current-1;
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
  size_t size = strlen((const char*)key);

  /* scan the linked list at the index calculated by the hash */
  while (finder)
  {
    if (memcmp((const void*)finder->str_limits.start_ptr, (const void*)key, size) == 0)
    {
      /* head remove */
      if (!finder_prev)
      {
        map_node->node_list[index] = finder->next; 
        LOG_STR("Map removed: ", key);
        return;
      }
      /* otherwise */
      else 
      {
        finder_prev->next = finder->next;
        LOG_STR("Map removed: ", key);
        return;
      }
    }
    finder = finder->next;
  }
}

static void f_allocate_new_memory_block_node(MEMORY_BLOCK_NODE *node, const _ul size)
{
  node->start = (_uc*)calloc(size, sizeof(_uc));
  assert(node->start);
  node->current = node->start;
  node->end = node->start + size;
}

static void f_init_memory_block(MEMORY_BLOCK *block, _ul size)
{
  assert(block);

  /* create new memory block node */
  MEMORY_BLOCK_NODE *new_node = calloc(1, sizeof(MEMORY_BLOCK_NODE));
  new_node->allocate = f_allocate_new_memory_block_node;
  new_node->next = 0;
  /* allocate resources */
  new_node->allocate(new_node, size);
  /* assign the new node */
  block->list = new_node;
  block->finder = new_node;
  LOG_I("Memory block initialized");
}

static void f_add_memory_block_node(MEMORY_BLOCK *block, const _ul size)
{
  assert(block);

  /* create new memory block node */
  MEMORY_BLOCK_NODE *new_node = calloc(1, sizeof(MEMORY_BLOCK_NODE));
  new_node->allocate = f_allocate_new_memory_block_node;
  new_node->next = 0;
  /* allocate resources */
  new_node->allocate(new_node, size);
  /* assign the new node */
  block->finder->next = new_node;
  block->finder = new_node;
}

static void f_deinit_memory_block(MEMORY_BLOCK *block)
{
  MEMORY_BLOCK_NODE *finder = block->list;
  MEMORY_BLOCK_NODE *finder_prev = 0;
  while (finder)
  {
    if (finder_prev) free(finder_prev->start);
    finder_prev = finder;
    finder = finder->next;
  }
  if (finder_prev) free(finder_prev->start);
  LOG_I("Memory block deallocation complete");
}

static _uc *f_get_memory_block(MEMORY_BLOCK *block, _ul size)
{
  assert(block->finder);
  assert(size > 0);
  if (block->finder->current+size > block->finder->end)
  {
    _ul new_size = (size > DEFAULT_MEMORY_BLOCK ? ((size/DEFAULT_MEMORY_BLOCK)+1)*DEFAULT_MEMORY_BLOCK : DEFAULT_MEMORY_BLOCK);
    LOG_I("Allocating new block node");
    block->add_block(block, new_size);
  }
  _uc *ret = block->finder->current;
  block->finder->current += size;
  LOG_HEX("Memory block used:", block->finder->current-ret);
  return ret;
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
#ifdef USE_MAP_DEINIT
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
#endif
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

/* https://en.wikipedia.org/wiki/Jenkins_hash_function */
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
                                      MEMORY_BLOCK *memory_block)
{
  /* scan all the node blocks */ 
  MAP_BLOCK_LIST *finder = map->lists;
  while (finder)
  {
    if (finder->id == which_block)
    {
      finder->insert(which_index_in_block, key, finder, memory_block);
      return;
    }
    finder = finder->next;
  }
  /* if not present, add the new block to the end of the list (current pointer) */
  map->insert(map, which_block, memory_block);
  map->current.list->insert(which_index_in_block, key, map->current.list, memory_block);
}

static void f_map_insert_key(MAP *map, const _uc *key, MEMORY_BLOCK *memory_block)
{
  const size_t len = strlen((const char*)key);
  const _ui hashed = hash((const _uc*)key, len); 
  const _ui which_block = (_ui)hashed >> 10;
  const _ui which_index_in_block = (_ui)hashed%DEFAULT_MAP_BLOCK_LIST_LEN;

  f_map_insert_key_internal(which_block, which_index_in_block, map, key, len, memory_block);
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
      LOG_STR("Map found: ", key);
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

static void f_add_trie_node(TRIE *root, MEMORY_BLOCK *memory_block, const _uc key)
{
  _uc *new_node = memory_block->get_block(memory_block, sizeof(TRIE));
  assert(new_node);

  root->childs[key]               = (TRIE*)new_node;
  root->childs[key]->add_node     = f_add_trie_node;
  root->childs[key]->insert_key   = f_insert_key_trie;
  root->childs[key]->remove_key   = f_remove_key_trie;
  root->childs[key]->has_childs   = f_trie_node_has_child;
  root->childs[key]->find_key     = f_trie_find_key;
  root->childs[key]->clean        = f_trie_clean_keys;
  root->childs[key]->clean_status = f_trie_clean_status;
}

static void f_insert_key_trie(TRIE *root, MEMORY_BLOCK *memory_block, const _uc *key, const size_t index, size_t size)
{
  if (index >= size) return;
  //TODO: removable
  if (!root || !root->insert_key || !root->add_node)
  {
    LOG_E("Trie error");
    exit(EXIT_FAILURE);
  }
  const uint8_t is_there_path = (root->childs[key[index]] != 0);
  if (!is_there_path)
  {
    /* allocate a new child */
    root->add_node(root, memory_block, key[index]);
    root->status[key[index]] = 1;
  }
  root->insert_key(root->childs[key[index]], memory_block, key, index+1, size);
}

static void f_remove_key_trie(TRIE *root, const _uc *key, size_t index)
{
  const uint8_t finder = key[index];

  if (root->has_childs(root))
  {
    root->remove_key(root->childs[finder], key, index+1);    
    if (!root->childs[finder]->has_childs(root->childs[finder]))
    {
      root->childs[finder] = 0;
    }
  }
}

static bool f_trie_node_has_child(TRIE *root)
{
  for (_ui i = 0; i < 256; i++)
  {
    if (root->childs[i]) return 1;  
  }
  return 0;
}

static bool f_trie_find_key(TRIE *root, const _uc *key, const size_t index, const size_t size)
{
  if (index >= size) return 1;
  if (!root->childs[key[index]]) return 0;
  return 1 * root->find_key(root->childs[key[index]], key, index+1, size);
}

static void f_trie_clean_status(TRIE *root)
{
  if (!root || !root->has_childs(root)) return;
  for (_ui i = 0; i < 256; i++)
  {
    if (root->childs[i])
    {
      root->status[i] = 1;
      root->clean_status(root->childs[i]);
    }
  }
}

static void f_trie_clean_keys(TRIE *root,
                             int32_t *available,
                             _uc *buffer,
                             const _uc *format,
                             _uc *wrong_chars, 
                             _ui *min_counter, 
                             _ui *exact_char_counter, 
                             bool *min_counter_finalized,
                             CHAR_COUNTER *exact_char_pos,
                             CHAR_COUNTER *avoid_char_pos,
                             _ui *chars_map,
                             const size_t index)
{
  if (!root)
  {
    return;
  }
  if (!root->has_childs(root))
  {
    //(*available)++;
    return;
  }
  for (_ui i = 0; i < 256; i++)
  {
    if (!root->status[i]) continue;

    /* delete all the subtrees with this character or wrong position, no more recursion needed */
    if (root->childs[i] && wrong_chars[i])
    {
      root->status[i] = 0;
    }
    else if (root->childs[i])
    {
      /* build the frequency map recursively */
      chars_map[i]++;
      /* build the word recursively */
      buffer[index] = i;

      /* leaf ? stop here as the next node will be empty */
      if (!root->childs[i]->has_childs(root->childs[i]))
      {
        buffer[index+1] = '\0';
        bool end = 0;

        /* control wrong characters in wrong position */
        for (_ui j = 0; j < 256 && !end; j++)
        {
          if (!avoid_char_pos[j].head) continue;
          CHAR_COUNTER_INTERNAL *finder = avoid_char_pos[j].head;
          while (finder)
          {
            _ui _index = finder->target_index;
            if (buffer[_index] == j)
            {
              root->status[i] = 0;
              end = 1;
              break;
            }
            finder = finder->next;
          }
        }

        /* check corrispondence with good chars in good positions */
        for (_ui j = 0; j < 256 && !end; j++)
        {
          if (!exact_char_pos[j].head) continue;
          CHAR_COUNTER_INTERNAL *finder = exact_char_pos[j].head;
          while (finder)
          {
            _ui _index = finder->target_index;
            if (buffer[_index] != j)
            {
              root->status[i] = 0;
              end = 1;
              break;
            }
            finder = finder->next;
          }
        }

        /* check for finalized char nums */
        for (_ui j = 0; j < 256 && !end; j++)
        {
          if (min_counter_finalized[j])
          {
            if (exact_char_counter[j] != chars_map[j])
            {
              root->status[i] = 0;
              end = 1;
            }
          }
        }

        /* check the minumim chars value computed by previous tests */
        for (_ui j = 0; j < 256 && !end; j++)
        {
          if (min_counter[j])
          {
            if (chars_map[j] < min_counter[j])
            {
              root->status[i] = 0;
              end = 1;
            }
          }
        }
        
        /* increment the counter if active */
        if (root->status[i]) (*available)++;
      }
      /* this node is not the end of the word... */
      else
      {
        root->clean(root->childs[i], 
                    available, 
                    buffer, 
                    format, 
                    wrong_chars, 
                    min_counter,
                    exact_char_counter,
                    min_counter_finalized, 
                    exact_char_pos, 
                    avoid_char_pos,
                    chars_map, 
                    index+1); 
      }
      /* cleanup the frequency map*/
      chars_map[i]--;
    }
  }
}

static void f_print_trie_2(TRIE *root, _uc *buffer, size_t index, const _ui target_size)
{
  uint8_t is_last = 1;
  if (!root)
  {
    return;
  }
  for (_ui i = 0; i < 256; i++)
  {
    if (root->childs[i] && root->status[i])
    {
      is_last = 0;
      buffer[index] = i;
      f_print_trie_2(root->childs[i], buffer, index+1, target_size);
      /* do not compute the buffer clean because it is overwritten */
    }
  }
  if (is_last)
  {
    buffer[index] = '\0';
    if (strlen((const char*)buffer) == target_size) printf("%s\n", buffer);
  }
}

static void f_print_trie(TRIE *root, _uc *buffer, size_t index, const _ui target_size)
{
  uint8_t is_last = 1;
  if (!root)
  {
    return;
  }
  for (_ui i = 0; i < 256; i++)
  {
    if (root->childs[i] && root->status[i])
    {
      is_last = 0;
      buffer[index] = i;
      f_print_trie(root->childs[i], buffer, index+1, target_size);
      /* do not compute the buffer clean because it is overwritten */
    }
  }
  if (is_last)
  {
    buffer[index] = '\0';
  #if LOCAL_TEST == 1
    if (strlen((const char*)buffer) == target_size) fprintf(fp, "%s\n", buffer);
  #else
    if (strlen((const char*)buffer) == target_size) printf("%s\n", buffer);
  #endif
  }
}

/* PROGRAM FUNCTIONS ----------------------------------------------------------------------------------------------------- */
static void get_char_map(const _uc *key, _ui *map)
{
  memset((void*)map, 0, sizeof(_ui)*256);
  for (_ui i = 0; i < strlen((const char*)key); i++)
  {
    map[key[i]]++;
  }
}

static _uc f_check_index_char_counter(CHAR_COUNTER_INTERNAL *counter, const _ui target)
{
  CHAR_COUNTER_INTERNAL *finder = counter;
  while (finder)
  {
    if (finder->target_index == target) return 1;
    finder = finder->next;
  }
  return 0;
}

//static _uc f_add_index_to_char_list_internal(MEMORY_BLOCK *memory_block, CHAR_COUNTER_INTERNAL *root[], const _uc target, const _ui index)
//{
//  _uc *alloc = memory_block->get_block(memory_block, sizeof(CHAR_COUNTER_INTERNAL));
//  CHAR_COUNTER_INTERNAL *new_node = (CHAR_COUNTER_INTERNAL*)alloc;
//  new_node->next = 0;
//  new_node->target_index = index;
//  if (!root[target])
//  {
//    /* new head */
//    root[target] = new_node;
//    return 1;
//  }
//  else if (!f_check_index_char_counter(root[target], index))
//  {
//    /* head insert */
//    new_node->next = root[target];
//    root[target] = new_node;
//    return 1; 
//  }
//  return 0;
//}

static void f_add_index_to_char_list(MEMORY_BLOCK *memory_block, CHAR_COUNTER *counter, const _uc target, const _ui index)
{
  _uc *alloc = 0;
  if (!counter[target].head)
  {
    /* new head */
    alloc = memory_block->get_block(memory_block, sizeof(CHAR_COUNTER_INTERNAL));
    CHAR_COUNTER_INTERNAL *new_node = (CHAR_COUNTER_INTERNAL*)alloc;
    new_node->next = 0;
    new_node->target_index = index;
    counter[target].head = new_node;
    counter[target].tail = new_node;
    counter[target].counter = 1;
  }
  /* check if this index is already present */
  else if (!f_check_index_char_counter(counter[target].head, index))
  {
    /* tail insert */
    alloc = memory_block->get_block(memory_block, sizeof(CHAR_COUNTER_INTERNAL));
    CHAR_COUNTER_INTERNAL *new_node = (CHAR_COUNTER_INTERNAL*)alloc;
    new_node->next = 0;
    new_node->target_index = index;
    counter[target].tail->next = new_node;
    counter[target].tail = new_node;
    counter[target].counter++;
  }
}

static void format_match(MEMORY_BLOCK *memory_block,
                         const _uc *target, 
                         const _uc *test, 
                         _uc *format, 
                         const size_t size, 
                         _uc *wrong_chars, 
                         _ui *min_counter,
                         _ui *exact_char_counter,
                         bool *min_counter_finalized,
                         CHAR_COUNTER *exact_char_pos,
                         CHAR_COUNTER *avoid_char_pos)
{
  _ui char_map_target[256] = {0};
  _ui char_map_target_cp[256] = {0};
  _ui char_map_test[256] = {0};
  _ui min_counter_cp[256] = {0};
  get_char_map(target, char_map_target);
  get_char_map(test, char_map_test);

  memcpy(char_map_target_cp, char_map_target, sizeof(_ui)*256);

  memset((void*)format, 0, size);

  /* assign correct position */
  for (_ui i = 0; i < size; i++)
  {
    if (target[i] == test[i])
    {
      /* format output */
      format[i] = '+';

      /* sign the exact positions */
      /* the char test[i] must be at index i */
      f_add_index_to_char_list(memory_block, exact_char_pos, test[i], i);

      /* bring the counter to min values */
      min_counter_cp[test[i]]++;

      /* update counter map */
      char_map_target[test[i]]--;
      char_map_test[test[i]]--;
    }
  }

  for (_ui i = 0; i < size; i++)
  {
    if (format[i]) continue;
    if (char_map_test[test[i]] && !char_map_target[test[i]])
    {
      /* format output */
      format[i] = '/';
      char_map_test[test[i]]--;
      
      /* finalize the counter */
      min_counter_finalized[test[i]] = true;
      /* ok sign the viewed labels */
      exact_char_counter[test[i]] = min_counter_cp[test[i]];

      /* sign position to avoid for this char */
      f_add_index_to_char_list(memory_block, avoid_char_pos, test[i], i);

      /* sign the chars to avoid if not present */
      if (char_map_target_cp[test[i]] == 0)
      {
        wrong_chars[test[i]] = 1;
      }
    }
    else if (char_map_test[test[i]] && char_map_target[test[i]])
    {
      format[i] = '|';
      char_map_target[test[i]]--;
      char_map_test[test[i]]--;
      
      /* sign position to avoid for this char */
      f_add_index_to_char_list(memory_block, avoid_char_pos, test[i], i);

      /* increment the counter */
      min_counter_cp[test[i]]++;
    }
  }
  format[size] = '\0';
  
  /* update the min discovered values */
  for (int i = 0; i < 256; i++)
  {
    min_counter[i] = (min_counter[i] > min_counter_cp[i] ? min_counter[i] : min_counter_cp[i]);
  }
}

static _ui solve(TRIE *trie,
                 MEMORY_BLOCK *memory_block,
                 _uc *buffer,
                 const _uc *target,
                 const _uc *test, 
                 _uc *format, 
                 _uc *wrong_chars, 
                 _ui *min_counter, 
                 _ui *exact_char_counter, 
                 bool *min_counter_finalized,
                 CHAR_COUNTER *exact_char_pos,
                 CHAR_COUNTER *avoid_char_pos,
                 _ui *test_char_map,
                 _ui *wrong_counter,
                 const _ui max_wrong)
{
  const size_t size = strlen((const char*)target);
  if (memcmp((const void*)target, (const void*)test, size) == 0)
  {
  #if LOCAL_TEST == 1
    fprintf(fp, "ok\n");
  #else
    printf("ok\n");
  #endif
    return WIN;
  }
  else if (trie->find_key(trie, test, 0, strlen((const char*)test)) == NOT_FOUND)
  {
  #if LOCAL_TEST == 1
    fprintf(fp, "not_exists\n");
  #else
    printf("not_exists\n"); 
  #endif
    return NOT_EXISTS;
  }
  else
  {
    int32_t available = 0;
    format_match(memory_block, 
                 target, 
                 test, 
                 format, 
                 size, 
                 wrong_chars, 
                 min_counter,
                 exact_char_counter,
                 min_counter_finalized, 
                 exact_char_pos, 
                 avoid_char_pos);
    

    trie->clean(trie, 
                &available, 
                buffer, 
                format, 
                wrong_chars, 
                min_counter,
                exact_char_counter,
                min_counter_finalized, 
                exact_char_pos, 
                avoid_char_pos,
                test_char_map, 0); 
  #if LOCAL_TEST == 1
    fprintf(fp, "%s\n%u\n", format, available);
  #else
    printf("%s\n%u\n", format, available);
  #endif
    (*wrong_counter)++;
  #if LOCAL_TEST == 1
    if ((*wrong_counter) >= max_wrong) fprintf(fp, "ko\n");
  #else
    if ((*wrong_counter) >= max_wrong) printf("ko\n");
  #endif
    return WRONG_MATCH;
  }
}

static TRIE *f_get_new_trie(MEMORY_BLOCK *memory_block)
{
  _uc *new_node = memory_block->get_block(memory_block, sizeof(TRIE));
  TRIE *trie         = (TRIE*)new_node;
  trie->add_node     = f_add_trie_node;
  trie->insert_key   = f_insert_key_trie;
  trie->remove_key   = f_remove_key_trie;
  trie->has_childs   = f_trie_node_has_child;
  trie->find_key     = f_trie_find_key;
  trie->clean        = f_trie_clean_keys;
  trie->clean_status =f_trie_clean_status; 
  return trie;
}

static void clean_new_line(_uc *buffer)
{
  size_t last_index = strlen((const char*)buffer);
  if (buffer[last_index-1] == '\n') buffer[last_index-1] = '\0';
}

static _uc get_vocabulary(TRIE *trie,MEMORY_BLOCK *memory_block, _uc *buffer, _ui read_len)
{
  while (fgets((char*)buffer, read_len, stdin))
  {
    /* delete the new line */
    clean_new_line(buffer);

    /* in game started ? */
    if (!memcmp((void*)buffer, (void*)(c_str_new_match), sizeof(c_str_new_match))) return 1;

    trie->insert_key(trie, memory_block, buffer, 0, strlen((const char*)buffer));
  }
  return 0;
}

static void f_add_words(TRIE *trie, MEMORY_BLOCK *memory_block, _uc *buffer, const _ui str_len)
{
  bool _continue = 1;
  do
  {
    if (fgets((char*)buffer, str_len, stdin))
    {
      clean_new_line(buffer);
      _continue = memcmp((void*)buffer, (void*)c_str_end_insert_keys, sizeof(c_str_end_insert_keys)); 
      if (_continue) trie->insert_key(trie, memory_block, buffer, 0, strlen((const char*)buffer));
    }
  } while (_continue);
}

void test(MAP *map, TRIE *trie, MEMORY_BLOCK *memory_block)
{
  /* utils preparation */
  _uc *buffer = 0;
  _uc tester[12]; /* max 32 bit integer */
  _uc *target = 0;
  size_t str_len = 0;

  /* helper buffers */
  /* chars to avoid in the current match */
  _uc *mem_alloc = memory_block->get_block(memory_block, sizeof(_uc)*256);
  _uc *wrong_chars = mem_alloc;

  /* the counter of min chars that must be in the target */
  mem_alloc = memory_block->get_block(memory_block, sizeof(_ui)*256);
  _ui *min_counter = (_ui*)mem_alloc;

  /* is the counter an exact value or a min value ? */
  mem_alloc = memory_block->get_block(memory_block, sizeof(bool)*256);
  bool *min_counter_finalized = (bool*)mem_alloc;
  
  /* positions with a certain match */
  mem_alloc = memory_block->get_block(memory_block, sizeof(_ui)*256);
  _ui *exact_char_counter = (_ui*)mem_alloc;

  /* positions with a certain not match */
  mem_alloc = memory_block->get_block(memory_block, sizeof(CHAR_COUNTER)*256);
  CHAR_COUNTER *avoid_char_pos = (CHAR_COUNTER*)mem_alloc;

  /* general buffer */
  mem_alloc = memory_block->get_block(memory_block, sizeof(_uc)*256);
  _uc *str_buffer = mem_alloc;
  
  /* general buffer for recursive trie computation */
  mem_alloc = memory_block->get_block(memory_block, sizeof(_ui)*256);
  _ui *test_char_map = (_ui*)mem_alloc;

  /* exact chars list */
  mem_alloc = memory_block->get_block(memory_block, sizeof(CHAR_COUNTER)*256);
  CHAR_COUNTER *exact_char_pos = (CHAR_COUNTER*)mem_alloc;
  
  /* get the length of the strings */
  if (fgets((char*)tester, 11, stdin))
    str_len = atoi((const char*)tester);
  else
    exit(EXIT_FAILURE);
  str_len = (_ul)(str_len > 20 ? str_len : 20);
  buffer = memory_block->get_block(memory_block, str_len);
  _uc start_game = get_vocabulary(trie, memory_block, buffer, str_len);

  /* the printf buffer */
  mem_alloc = memory_block->get_block(memory_block, str_len);
  _uc *format = mem_alloc;

  while (1)
  {
    if (!start_game) return;
    start_game = 0;

    /* insid a game */
    _ui counter = 0;
    _ui shots = 0;
    _ui wrong_counter = 0;

    while (fgets((char*)buffer, str_len, stdin))
    {
      clean_new_line(buffer);

      /* the first is the target */
      if (!counter)
      {
        target = memory_block->get_block(memory_block, str_len);
        strcpy((char*)target, (const char*)buffer);
        
        /* init buffers for new match */
        memset((void*)wrong_chars, 0, sizeof(_uc)*256);
        memset((void*)min_counter, 0, sizeof(_ui)*256);
        memset((void*)exact_char_counter, 0, sizeof(_ui)*256);
        memset((void*)min_counter_finalized, 0, sizeof(bool)*256);
        memset((void*)exact_char_pos, 0, sizeof(CHAR_COUNTER)*256);
        memset((void*)avoid_char_pos, 0, sizeof(CHAR_COUNTER)*256);
        memset((void*)test_char_map, 0, sizeof(_ui)*256);

        /* clean old flags */
        trie->clean_status(trie);
      }
      /* the second represent how many shots */
      else if (counter == 1)
      {
        shots = atoi((const char*)buffer);
        /* we are on */
      }
      /* command print trie */
      else if (!memcmp((void*)buffer, (void*)c_str_print_filtered, sizeof(c_str_print_filtered)))
      {
        f_print_trie(trie, format, 0, strlen((const char*)target));
      }
      /* command add words */
      else if (!memcmp((void*)buffer, (void*)c_str_start_insert_keys, sizeof(c_str_start_insert_keys)))
      {
        f_add_words(trie, memory_block, buffer, str_len); 
        int32_t available;
        /* clean new words, point of optimization */
        trie->clean(trie, 
                    &available, 
                    buffer, 
                    format, 
                    wrong_chars, 
                    min_counter,
                    exact_char_counter,
                    min_counter_finalized, 
                    exact_char_pos, 
                    avoid_char_pos,
                    test_char_map, 0); 
      }
      /* command new game */
      else if (!memcmp((void*)buffer, (void*)c_str_new_match, sizeof(c_str_new_match)))
      {
        //return;
        start_game = 1;
        break;
      }
      /* check word */
      else
      {
        /* skip if out of limits */
        if (wrong_counter >= shots) continue;
        if (solve(trie,
              memory_block,
              str_buffer,
              target, 
              buffer, 
              format, 
              wrong_chars, 
              min_counter,
              exact_char_counter,
              min_counter_finalized, 
              exact_char_pos,
              avoid_char_pos,
              test_char_map, 
              &wrong_counter,
              shots) == WIN) {};
      }
      counter++;
    }
  }
}

/* MAIN ------------------------------------------------------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
  /* init mem blocks */
  MEMORY_BLOCK memory_block      = {.init = f_init_memory_block,
                                           .deinit = f_deinit_memory_block,
                                           .get_block = f_get_memory_block,
                                           .add_block = f_add_memory_block_node};
  memory_block.init(&memory_block, DEFAULT_MEMORY_BLOCK);
  
  /* init hash map */
  MAP map = { .init = f_init_map, 
              .deinit = f_deinit_map,
              .insert = f_insert_map_node_list,
              .insert_key = f_map_insert_key,
              .find_key = f_map_find_key,
              .remove_key = f_map_remove_key,
              .lists = NULL,
              .current.list = NULL};
  map.init(&map, &memory_block);

  /* init trie root */
  TRIE *trie = f_get_new_trie(&memory_block);

#if LOCAL_TEST == 1
  fp = fopen("out.txt", "w");
#endif
  /* test */
  test(&map, trie, &memory_block);
#if LOCAL_TEST == 1
  fclose(fp);
#endif

  /* deallocate all sources */
  memory_block.deinit(&memory_block);

  return 0;
}

