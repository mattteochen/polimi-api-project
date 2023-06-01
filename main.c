#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

#define DEBUG_DP 0
#define DEBUG_FILTER 0
#define DEBUG_INPUT 0

const char AGGIUNTA[] = "aggiunta";
const char NON_AGGIUNTA[] = "non aggiunta";
const char DEMOLITA[] = "demolita";
const char NON_DEMOLITA[] = "non demolita";
const char ROTTAMATA[] = "rottamata";
const char NON_ROTTAMATA[] = "non rottamata";
const char NP[] = "nessun percorso";

const char ADD_STATION[] = "aggiungi-stazione";
const char REMOVE_STATION[] = "demolisci-stazione";
const char ADD_CAR[] = "aggiungi-auto";
const char BREAK_CAR[] = "rottama-auto";
const char GET_PATH[] = "pianifica-percorso";

static inline uint32_t get_station_index_in_array_from_id(uint32_t id);

typedef enum {
  STR2INT_SUCCESS,
  STR2INT_OVERFLOW,
  STR2INT_UNDERFLOW,
  STR2INT_INCONVERTIBLE
} Str2IntErrNo;

typedef struct {
  uint32_t* fuels;
  uint32_t* stations;
} FilteredStationFuel;

typedef struct {
  uint32_t size;
  uint32_t* arr; //flatten 2d array ([station,fuel]) into 1d array
} StationArray;

typedef struct {
  uint32_t size;
  uint32_t* dp;
  FilteredStationFuel filtered;
} DpArrayRes;

typedef struct FuelListNode {
  uint32_t fuel_level;
  uint32_t count;
  struct FuelListNode* left;
  struct FuelListNode* right;
} FuelListNode;

typedef struct BstStationsListNode {
  int station_id;
  FuelListNode* fuels;
  struct BstStationsListNode* left;
  struct BstStationsListNode* right;
} BstStationsListNode;

typedef struct {
  int32_t idx;
  int32_t jumps;
  uint32_t* buff;
  uint32_t buff_idx;
} Pair;

// typedef struct {
//   uint32_t fuel_level;
//   uint32_t count;
// } PriorityQueueNode;

// typedef struct {
//   PriorityQueueNode* heap;
//   int capacity;
//   int size;
// } PriorityQueue;

uint32_t g_stations_size = 0;
BstStationsListNode* g_stations_bst = 0;
BstStationsListNode* g_last_created_station_node = 0;
uint8_t g_map_changed = 0;
StationArray g_stations_array = {0, 0};

Str2IntErrNo str2int(int *out, char *s, int base) {
  char *end;
  if (s[0] == '\0' || isspace(s[0]))
    return STR2INT_INCONVERTIBLE;
  errno = 0;
  long l = strtol(s, &end, base);
  /* Both checks are needed because INT_MAX == LONG_MAX is possible. */
  if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX))
    return STR2INT_OVERFLOW;
  if (l < INT_MIN || (errno == ERANGE && l == LONG_MIN))
    return STR2INT_UNDERFLOW;
  if (*end != '\0')
    return STR2INT_INCONVERTIBLE;
  *out = l;
  return STR2INT_SUCCESS;
}

//------------------------PRIORITY QUEUE - FUEL LIST------------------------

//This structure is used to store the all the fuels in a sorted manner for a given station

// void resize_priority_queue(PriorityQueue* pq, int capacity) {
//   if (pq->size > capacity) {
//     printf("Can not fill old heap into new one");
//     return;
//   }
//   PriorityQueueNode* new_heap = realloc(pq->heap, sizeof(PriorityQueueNode) * capacity);
//   if (!new_heap) {
//     printf("Realloc failed");
//     exit(1);
//   }
//   pq->heap = new_heap;
//   pq->capacity = capacity;
// }

// PriorityQueue* create_priority_queue(int capacity) {
//   PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
//   pq->heap = (PriorityQueueNode*)malloc((capacity) * sizeof(PriorityQueueNode));
//   pq->capacity = capacity;
//   pq->size = 0;
//   return pq;
// }

// void swap(PriorityQueueNode* a, PriorityQueueNode* b) {
//   PriorityQueueNode temp = *a;
//   *a = *b;
//   *b = temp;
// }

// void heapify(PriorityQueue* pq, int index) {
//   PriorityQueueNode* array = pq->heap; 
//   int size = pq->size;
//   if (size > 1) {
//     // Find the largest among root, left child and right child
//     int largest = index;
//     int l = 2 * index + 1;
//     int r = 2 * index + 2;
//     if (l < size && array[l].fuel_level > array[largest].fuel_level)
//       largest = l;
//     if (r < size && array[r].fuel_level > array[largest].fuel_level)
//       largest = r;

//     // Swap and continue heapifying if root is not largest
//     if (largest != index) {
//       swap(&array[index], &array[largest]);
//       heapify(pq, largest);
//     }
//   }
// }

// void enqueue_priority_queue(PriorityQueue* pq, int fuel_level, int count) {
//   PriorityQueueNode* array = pq->heap;
//   if (pq->size == 0) {
//     array[0].fuel_level = fuel_level;
//     array[0].count = count;
//     pq->size++;
//   } else {
//     array[pq->size].fuel_level = fuel_level;
//     array[pq->size].count = count;
//     pq->size++;
//     for (int i=(pq->size / 2 - 1); i >= 0; i--) {
//       heapify(pq, i);
//     }
//   }
// }

// //currently, a not present node dequeue is not supported
// void dequeue_priority_queue(PriorityQueue* pq, int fuel_level) {
//   PriorityQueueNode* array = pq->heap;
//   uint32_t i;
//   for (i = 0; i < pq->size; i++) {
//     if (fuel_level == array[i].fuel_level)
//       break;
//   }

//   swap(&array[i], &array[pq->size - 1]);
//   pq->size--;
//   for (int i=(pq->size / 2 - 1); i >= 0; i--) {
//     heapify(pq, i);
//   }
// }

// PriorityQueueNode* front_priority_queue(PriorityQueue* pq) {
//   if (pq->size == 0) {
//     printf("PriorityQueue is empty. No front_priority_queue element.\n");
//     return 0;
//   }

//   return &(pq->heap[0]);
// }

// int is_priority_queue_empty(PriorityQueue* pq) {
//   return pq->size == 0;
// }

// void print_priority_queue(PriorityQueue* pq) {
//   for (int i = 0; i < pq->size; ++i)
//     printf("f: %d - c: %d\n", pq->heap[i].fuel_level, pq->heap[i].count);
//   printf("\n\n");
// }

// void fee_priority_queue(PriorityQueue* pq) {
//   free(pq->heap);
//   free(pq);
// }

// //------------------------BINARY SEARCH TREE - FUEL LIST------------------------

// //This structure is used to store the all the fuels in a sorted manner for a given station

FuelListNode* fuel_list_create_node(uint32_t fuel_level, uint32_t count) {
  FuelListNode* node = calloc(1, sizeof(FuelListNode));
  node->fuel_level = fuel_level;
  node->count = count;
  return node;
}

FuelListNode* fuel_list_insert_node(FuelListNode* root, uint32_t fuel_level, uint32_t count) {
  if (root == NULL) {
    return fuel_list_create_node(fuel_level, count);
  }

  if (fuel_level < root->fuel_level) {
    root->left = fuel_list_insert_node(root->left, fuel_level, count);
  } else {
    root->right = fuel_list_insert_node(root->right, fuel_level, count);
  }

  return root;
}

FuelListNode* fuel_list_find_min_node(FuelListNode* node) {
  if (!node) {
    return 0;
  }
  while (node->left != NULL) {
    node = node->left;
  }
  return node;
}

FuelListNode* fuel_list_find_max_node(FuelListNode* node) {
  if (!node) {
    return 0;
  }
  while (node->right != NULL) {
    node = node->right;
  }
  return node;
}

FuelListNode* fuel_list_remove_node(FuelListNode* root, uint32_t fuel_level) {
  if (root == NULL) {
    return root;
  }

  if (fuel_level < root->fuel_level) {
    root->left = fuel_list_remove_node(root->left, fuel_level);
  } else if (fuel_level > root->fuel_level) {
    root->right = fuel_list_remove_node(root->right, fuel_level);
  } else {

    // Case 1: No child or one child
    if (!root->left && !root->right) {
      free(root);
      return NULL;
    }

    // Case 2: One child
    if (root->left == NULL || root->right == NULL) {
      FuelListNode* temp = root->right;
      temp = !root->left ? root->right : root->left;
      free(root);
      return temp;
    }

    // Case 3: Two children
    FuelListNode* minRight = fuel_list_find_min_node(root->right);
    root->fuel_level = minRight->fuel_level;
    root->right = fuel_list_remove_node(root->right, minRight->fuel_level);
  }

  return root;
}

FuelListNode* fuel_list_search(FuelListNode* root, uint32_t fuel_level) {
  if (root == NULL || root->fuel_level == fuel_level) {
    return root;
  }

  if (fuel_level < root->fuel_level) {
    return fuel_list_search(root->left, fuel_level);
  } else {
    return fuel_list_search(root->right, fuel_level);
  }
}

uint8_t fuel_list_update_count(FuelListNode* root, uint32_t fuel_level, int32_t delta) {
  FuelListNode* target = fuel_list_search(root, fuel_level);
  if (!target) {
    return 0;
  }

  if (delta < 0 && -delta > target->count) {
    target->count = 0;
  }
  target->count += delta;
  return 1;
}

void print_fuel_list(FuelListNode* root) {
  if (root == NULL) {
    return;
  }

  print_fuel_list(root->left);
  printf("l: %d - c: %d\n", root->fuel_level, root->count);
  print_fuel_list(root->right);
}

void fuel_list_free(FuelListNode* root) {
  if (root == NULL) {
    return;
  }

  fuel_list_free(root->left);
  fuel_list_free(root->right);
  free(root);
}

//------------------------BINARY SEARCH TREE - STATION LIST------------------------

//This station_id structure is used to store the g_stations_bst with the max available car autonomy in a sorted manner

BstStationsListNode* stations_list_create_node(int station_id, uint32_t max_fuel, uint32_t count) {
  BstStationsListNode* node = calloc(1, sizeof(BstStationsListNode));
  node->station_id = station_id;
  node->fuels = fuel_list_insert_node(node->fuels, max_fuel, count);
  g_last_created_station_node = node;
  return node;
}

BstStationsListNode* stations_list_insert_node(BstStationsListNode* root, uint32_t station_id, uint32_t max_fuel, uint32_t count) {
  if (root == NULL) {
    g_stations_size++;
    return stations_list_create_node(station_id, max_fuel, count);
  }

  if (station_id < root->station_id) {
    root->left = stations_list_insert_node(root->left, station_id, max_fuel, count);
  } else {
    root->right = stations_list_insert_node(root->right, station_id, max_fuel, count);
  }

  return root;
}

BstStationsListNode* stations_list_find_min_node(BstStationsListNode* node) {
  while (node->left != NULL) {
    node = node->left;
  }
  return node;
}

BstStationsListNode* stations_list_remove_node(BstStationsListNode* root, int station_id) {
  if (root == NULL) {
    return root;
  }

  if (station_id < root->station_id) {
    root->left = stations_list_remove_node(root->left, station_id);
  } else if (station_id > root->station_id) {
    root->right = stations_list_remove_node(root->right, station_id);
  } else {
    g_stations_size--;

    // Case 1: No child or one child
    if (!root->left && !root->right) {
      fuel_list_free(root->fuels); 
      free(root);
      return NULL;
    }

    // Case 2: One child
    if (root->left == NULL || root->right == NULL) {
      BstStationsListNode* temp = root->right;
      temp = !root->left ? root->right : root->left;
      fuel_list_free(root->fuels); 
      free(root);
      return temp;
    }

    // Case 3: Two children
    BstStationsListNode* minRight = stations_list_find_min_node(root->right);
    root->station_id = minRight->station_id;
    root->right = stations_list_remove_node(root->right, minRight->station_id);
  }

  return root;
}

BstStationsListNode* stations_list_search(BstStationsListNode* root, int station_id) {
  if (root == NULL || root->station_id == station_id) {
    return root;
  }

  if (station_id < root->station_id) {
    return stations_list_search(root->left, station_id);
  } else {
    return stations_list_search(root->right, station_id);
  }
}

void print_stations_list_bst(BstStationsListNode* root) {
  if (root == NULL) {
    return;
  }

  print_stations_list_bst(root->left);
  printf("station %d\n", root->station_id);
  print_fuel_list(root->fuels);
  print_stations_list_bst(root->right);
}

void stations_list_free(BstStationsListNode* root) {
  if (root == NULL) {
    return;
  }

  stations_list_free(root->left);
  stations_list_free(root->right);
  fuel_list_free(root->fuels);
  free(root);
}

void stations_list_bst_to_array_recursive(BstStationsListNode* root) {
  if (root == NULL) {
    return;
  }

  stations_list_bst_to_array_recursive(root->left);
  g_stations_array.arr[g_stations_array.size++] = root->station_id;
  FuelListNode* max = fuel_list_find_max_node(root->fuels);
  g_stations_array.arr[g_stations_array.size++] = max ? max->fuel_level : 0; 
  stations_list_bst_to_array_recursive(root->right);
}

void stations_list_bst_to_array() {
  if (g_stations_array.arr != NULL) {
    free(g_stations_array.arr);
    g_stations_array.arr = NULL;
    g_stations_array.size = 0;
  }

  //TODO: can maintain the buffer if new size is equal or lower
  g_stations_array.arr = (uint32_t*)malloc(sizeof(uint32_t) * (g_stations_size << 2));
  stations_list_bst_to_array_recursive(g_stations_bst);
}

void print_station_array(StationArray* s) {
  if (!s) {
    return;
  }

  for (int i=0; i<s->size; i+=2) {
    printf("s: %d - f: %d\n", s->arr[i], s->arr[i+1]);
  }
}

// Retrieve an array with the stations max available gas fuel associated to stations id [lhs, rhs] or [rhs, lhs]
FilteredStationFuel get_stations_id_and_fuels_array(uint32_t lhs, uint32_t rhs, uint32_t size, int8_t incrementer) {
  
  uint32_t* fuel = malloc(sizeof(uint32_t) * size);
  uint32_t* station = malloc(sizeof(uint32_t) * size);
  
  if (!fuel || !station) {
    printf("Malloc failed in <get_stations_fuels_array>\n");
    exit(1);
  }

  int32_t dest_idx = incrementer > 0 ? 0 : size-1;
  uint32_t start = get_station_index_in_array_from_id(lhs < rhs ? lhs : rhs);
  uint32_t end = get_station_index_in_array_from_id(lhs > rhs ? lhs : rhs);
  for (uint32_t i=start; i<g_stations_array.size && i <= end; i+=2) {
    fuel[dest_idx] = g_stations_array.arr[i+1]; //save the max fuel
    station[dest_idx] = g_stations_array.arr[i]; //save the station id
    dest_idx += incrementer;
  }

  FilteredStationFuel ans = {
    fuel, station
  };
  return ans;
}

//the fuel index is to be intended coming from the array containing only the fuels, used in the dp computation
static inline int32_t get_station_id_from_fuel_index(uint32_t idx) {
  if (idx*2 >= g_stations_array.size) {
    // printf("index %d overflow in <get_station_id_from_fuel_index>\n", idx*2);
    return INT_MAX;
  }
  //g_stations_array:[1,2,3,4,5,6] -> with fuel_idx = 2(fuel 6) the original station id associated 5 
  return g_stations_array.arr[idx * 2];
}

static inline uint32_t get_stations_id_distance_diff_from_fuel_index(uint32_t a, uint32_t b) {
  // printf("diff: %d\n", ans);
  return abs(get_station_id_from_fuel_index(b) - get_station_id_from_fuel_index(a));
}

static inline uint32_t get_station_index_in_array_from_id(uint32_t id) {
  for (uint32_t i=0; i<g_stations_array.size; i+=2) {
    if (g_stations_array.arr[i] == id) {
      // printf("found station id %d at idx %d\n", id, i);
      return i;
    }
  }
  printf("No station found in station array for: %d\n", id);
  exit(1);
}

//retrieve how many stations are there between two stations id, extremes excluded
static inline uint32_t get_stations_between(uint32_t lhs, uint32_t rhs) {
  if (lhs < rhs) {
    return ((get_station_index_in_array_from_id(rhs) - get_station_index_in_array_from_id(lhs)) / 2) - 1;
  } else {
    return ((get_station_index_in_array_from_id(lhs) - get_station_index_in_array_from_id(rhs)) / 2) - 1;
  }
}

//the caller must check that start and end stations do exist in the current map
DpArrayRes compute_min_path_dp(int start_station, int end_station) {
  DpArrayRes ret = {0, 0, 0, 0};

  uint32_t arr_size = get_stations_between(start_station, end_station) + 2; //add the extremes
  //filter out only the needed stations max fuels
  FilteredStationFuel filtered =
    get_stations_id_and_fuels_array(start_station, end_station, arr_size, start_station < end_station ? 1 : -1);
  uint32_t* dp = calloc(arr_size, sizeof(uint32_t));

#if DEBUG_FILTER
  printf("arr size: %d\n", arr_size);
  for (int i=0; i<arr_size; i++) {
    printf("s: %d f: %d\n", filtered.stations[i], filtered.fuels[i]);
  }
  printf("\n");
#endif

  for(int idx=arr_size-2; idx>=0; idx--){ //do not use uint32_t as this will go negative
    uint32_t steps = filtered.fuels[idx];

    int min = INT_MAX;
    if(steps > 0){
      for(int i=1; i<=steps; i++){
        uint32_t idx_station_id = get_station_id_from_fuel_index(idx);
        uint32_t idx_pi_station_id = get_station_id_from_fuel_index(idx+i);
        if (start_station < end_station && idx_station_id + steps < idx_pi_station_id) {
          break;
        }
        if (start_station > end_station && steps < idx_station_id && idx_station_id - steps > idx_pi_station_id) {
          break;
        }
        if(idx+i < arr_size) {
          //update min if it is min and the step is reacheble as single steps have not weight = 1 but weight = get_stations_id_distance_diff_from_fuel_index
          if (dp[idx+i] <= min && steps >= get_stations_id_distance_diff_from_fuel_index(idx, idx+i)) {
            min = dp[idx+i];
          }
        }
      }
    }
    dp[idx] = min == INT_MAX ? min : min+1;
  }

#if (DEBUG_DP)
  for (uint32_t i=0; i<arr_size; ++i) {
    printf("%d ", dp[i]);
  }
  printf("\n\n");
#endif
  ret.dp = dp;
  ret.filtered = filtered;
  ret.size = arr_size;
  return ret;
}

//given a best dp array, backtrack to the best path
void backtrack_best_route(DpArrayRes in) {

  const uint32_t* fuels = in.filtered.fuels;
  const uint32_t* stations = in.filtered.stations;
  const uint32_t* dp = in.dp;
  const uint32_t size = in.size; 
 
  uint32_t queue_front = 0;
  uint32_t queue_back = 0;
  const uint32_t queue_size = size * 10;
  Pair* queue = calloc(queue_size, sizeof(Pair)); //TODO: dynamic size
  queue[queue_back].idx = 0;
  queue[queue_back].jumps = dp[0];
  queue[queue_back].buff_idx = 0;
  queue[queue_back++].buff = calloc(size, sizeof(uint32_t)); //size is the max upperbound

  while (queue_back != queue_front) {
    Pair p = queue[queue_front++]; //increment front pointer

    if (p.jumps == 0) {
      //TODO: use a single stdout
      printf("%d ", stations[0]);
      for (uint32_t i=0; i<p.buff_idx; i++) {
        printf("%d ", stations[p.buff[i]]);
      }
      printf("\n");
      continue;
    }

    for (uint32_t i=1; i<=fuels[p.idx]; i++) {
      if (p.idx+i < size && p.jumps-1 == dp[p.idx+i]) {

        queue[queue_back].idx = p.idx+i;
        queue[queue_back].jumps = p.jumps-1;
        queue[queue_back].buff = malloc(sizeof(uint32_t) * size);

        //copy the p buffer into the new node
        memcpy(queue[queue_back].buff, p.buff, sizeof(uint32_t) * p.buff_idx);
        queue[queue_back].buff_idx = p.buff_idx;
        queue[queue_back].buff[queue[queue_back].buff_idx++] = p.idx+i;
        queue_back++;
      }
    }
  }

  for (uint32_t i=0; i<queue_size; ++i) {
    if (queue[i].buff) {
      free(queue[i].buff);
    }
  }
  free(queue);
}

static inline void add_station(const uint8_t* input_buf) {
  uint32_t idx = 18;
  int32_t cars = 0;
  int32_t station_id = 0;
  char storage[12] = {0};
  uint8_t storage_idx = 0;

  //get station id
  while (input_buf[idx] != ' ') {
    storage[storage_idx++] = input_buf[idx++];
  }
  idx++;
  storage[storage_idx] = '\0';
  
  str2int(&station_id, storage, 10);
#if DEBUG_INPUT
  printf("station: %d\n", station_id);
#endif

  if (stations_list_search(g_stations_bst, station_id)) {
    printf("%s\n", NON_AGGIUNTA);
    return;
  }

  //get car nums
  storage_idx = 0;
  while (input_buf[idx] != ' ') {
    storage[storage_idx++] = input_buf[idx++];
  }
  idx++;
  storage[storage_idx] = '\0';

  str2int(&cars, storage, 10);
#if DEBUG_INPUT
  printf("cars: %d\n", cars);
#endif

  if (cars == 0) {
    g_stations_bst = stations_list_insert_node(g_stations_bst, station_id, 0, 1);
    printf("%s\n", AGGIUNTA);
    //signal the map has changed
    g_map_changed = 1;
    return;
  }
 
  for (uint32_t i=0; i<cars && input_buf[idx] != '\n' && input_buf[idx] != '\0'; i++) {
    //get fuel
    storage_idx = 0;
    while (input_buf[idx] != ' ' && input_buf[idx] != '\n' && input_buf[idx] != '\0') {
      storage[storage_idx++] = input_buf[idx++];
    }
    idx++;
    storage[storage_idx] = '\0';

    int32_t fuel = 0;
    str2int(&fuel, storage, 10);
#if DEBUG_INPUT
    printf("fuel: %d\n", fuel);
#endif

    //first fuel ? if yes add station and fuel
    if (i == 0) {
      g_stations_bst = stations_list_insert_node(g_stations_bst, station_id, fuel, 1);
    //else update the node fuel values
    } else {
      g_last_created_station_node->fuels = fuel_list_insert_node(g_last_created_station_node->fuels, fuel, 1);
    }
  }
  printf("%s\n", AGGIUNTA);

  //signal the map has changed
  g_map_changed = 1;
}

static inline void remove_station(const uint8_t* input_buf) {
  uint32_t idx = 19;
  int32_t station_id = 0;
  char storage[12] = {0};
  uint8_t storage_idx = 0;

  //get station id
  while (input_buf[idx] != '\n' && input_buf[idx] != '\0') {
    storage[storage_idx++] = input_buf[idx++];
  }
  idx++;
  storage[storage_idx] = '\0';
  
  str2int(&station_id, storage, 10);
#if DEBUG_INPUT
  printf("station: %d\n", station_id);
#endif

  //optimistic removal log(n)
  uint32_t before_delete = g_stations_size;
  g_stations_bst = stations_list_remove_node(g_stations_bst, station_id);
  if (g_stations_size == before_delete) {
    printf("%s\n", NON_DEMOLITA);
  } else {
    printf("%s\n", DEMOLITA);
    //signal map has changed
    g_map_changed = 1;
  }
}

static inline void add_car(const uint8_t* input_buf) {
  uint32_t idx = 14;
  int32_t fuel = 0;
  int32_t station_id = 0;
  char storage[12] = {0};
  uint8_t storage_idx = 0;

  //get station id
  while (input_buf[idx] != ' ') {
    storage[storage_idx++] = input_buf[idx++];
  }
  idx++;
  storage[storage_idx] = '\0';
  
  str2int(&station_id, storage, 10);
#if DEBUG_INPUT
  printf("station: %d\n", station_id);
#endif

  BstStationsListNode* station_node = stations_list_search(g_stations_bst, station_id);
  if (!station_node) {
    printf("%s\n", NON_AGGIUNTA);
    return;
  }

  //get fuel
  storage_idx = 0;
  while (input_buf[idx] != '\n' && input_buf[idx] != '\0') {
    storage[storage_idx++] = input_buf[idx++];
  }
  idx++;
  storage[storage_idx] = '\0';

  str2int(&fuel, storage, 10);
#if DEBUG_INPUT
  printf("fuel: %d\n", fuel);
#endif

  station_node->fuels = fuel_list_insert_node(station_node->fuels, fuel, 1);
  printf("%s\n", AGGIUNTA);

  //signal map has changed
  g_map_changed = 1;
}

static inline void remove_car(const uint8_t* input_buf) {
  uint32_t idx = 13;
  int32_t fuel = 0;
  int32_t station_id = 0;
  char storage[12] = {0};
  uint8_t storage_idx = 0;

  //get station id
  while (input_buf[idx] != ' ') {
    storage[storage_idx++] = input_buf[idx++];
  }
  idx++;
  storage[storage_idx] = '\0';
  
  str2int(&station_id, storage, 10);
#if DEBUG_INPUT
  printf("station: %d\n", station_id);
#endif

  BstStationsListNode* station_node = stations_list_search(g_stations_bst, station_id);
  if (!station_node) {
    printf("%s\n", NON_ROTTAMATA);
    return;
  }

  //get fuel
  storage_idx = 0;
  while (input_buf[idx] != '\n' && input_buf[idx] != '\0') {
    storage[storage_idx++] = input_buf[idx++];
  }
  idx++;
  storage[storage_idx] = '\0';

  str2int(&fuel, storage, 10);
#if DEBUG_INPUT
  printf("fuel: %d\n", fuel);
#endif

  if (fuel_list_update_count(station_node->fuels, fuel, -1)) {
    printf("%s\n", ROTTAMATA);
    //signal the map has changed
    g_map_changed = 1;
  } else {
    printf("%s\n", NON_ROTTAMATA);
  }
}

static inline void compute_path(const uint8_t* input_buf) {
  uint32_t idx = 19;
  int32_t from = 0;
  int32_t to = 0;
  char storage[12] = {0};
  uint8_t storage_idx = 0;

  //get station id
  while (input_buf[idx] != ' ') {
    storage[storage_idx++] = input_buf[idx++];
  }
  idx++;
  storage[storage_idx] = '\0';
  
  str2int(&from, storage, 10);
#if DEBUG_INPUT
  printf("from: %d\n", from);
#endif

  //get fuel
  storage_idx = 0;
  while (input_buf[idx] != '\n' && input_buf[idx] != '\0') {
    storage[storage_idx++] = input_buf[idx++];
  }
  idx++;
  storage[storage_idx] = '\0';

  str2int(&to, storage, 10);
#if DEBUG_INPUT
  printf("to: %d\n", to);
#endif

  //fist compute the path existance
  BstStationsListNode *start_station_node = stations_list_search(g_stations_bst, from);
  BstStationsListNode *end_station_node = stations_list_search(g_stations_bst, to);
  if (!start_station_node || !end_station_node) {
    printf("%s\n", NP);
    return;
  }

  //if does exist compute the flatten road, if changed
  if (g_map_changed) {
    g_map_changed = 0;
    stations_list_bst_to_array();
  }
  //compute min path with dp and rebuild the path with backtracking
  DpArrayRes dp_res = compute_min_path_dp(from, to);
  if (dp_res.dp[0] != INT_MAX) {
    backtrack_best_route(dp_res);
  } else {
    printf("%s\n", NP);
  }
  free(dp_res.dp);
  free(dp_res.filtered.fuels);
  free(dp_res.filtered.stations);
}

void parse_cmd() {
  uint8_t buf[1024];
  while(fgets((char*)buf, 1023, stdin)) {
    // printf("%s\n", buf);

    //add station
    if (buf[0] == 'a' && buf[9] == 's') {
      add_station(buf); 
    //remove station
    } else if (buf[0] == 'd' && buf[10] == 's') {
      remove_station(buf);
    //add car
    } else if (buf[0] == 'a' && buf[9] == 'a') {
      add_car(buf);
    //remove car
    } else if (buf[0] == 'r' && buf[8] == 'a') {
      remove_car(buf);
    //get path
    } else {
      compute_path(buf);
    }
  }
}

int main() {
  parse_cmd();

  stations_list_free(g_stations_bst);
  free(g_stations_array.arr);
  return 0;
}

// int demo() {

//   g_stations_bst = stations_list_insert_node(g_stations_bst, 10, 20, 1);
//   g_stations_bst = stations_list_insert_node(g_stations_bst, 20, 30, 5);
//   g_stations_bst = stations_list_insert_node(g_stations_bst, 30, 10, 1);
//   g_stations_bst = stations_list_insert_node(g_stations_bst, 40, 20, 1);
//   g_stations_bst = stations_list_insert_node(g_stations_bst, 50, 10, 1);
//   g_stations_bst = stations_list_insert_node(g_stations_bst, 60, 10, 100);
//   g_stations_bst = stations_list_insert_node(g_stations_bst, 100, 100, 100);
//   print_stations_list_bst(g_stations_bst);

//   stations_list_bst_to_array();
//   // print_station_array(&g_stations_array);

//   DpArrayRes dp_res = compute_min_path_dp(10, 60);

//   if (dp_res.dp[0] != INT_MAX) {
//     backtrack_best_route(dp_res);
//   }
//   free(dp_res.dp);
//   free(dp_res.filtered.fuels);
//   free(dp_res.filtered.stations);

//   stations_list_free(g_stations_bst);
//   free(g_stations_array.arr);

//   return 0;
// }

