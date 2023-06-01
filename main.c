#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#define PATH_NOT_FOUND -1;

const char AGGIUNTA[] = "aggiunta";
const char NON_AGGIUNTA[] = "non aggiunta";
const char DEMOLITA[] = "demolita";
const char NON_DEMOLITA[] = "non demolita";
const char ROTTAMATA[] = "rottamata";
const char NON_ROTTAMATA[] = "non rottamata";
const char NP[] = "nessun percorso";

static inline uint32_t get_station_index_in_array_from_id(uint32_t id);

typedef struct {
  uint32_t* fuels;
  uint32_t* stations;
} FilteredStationFuel;

typedef struct {
  int size;
  int* arr; //flatten 2d array ([station,fuel]) into 1d array
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
}Pair;

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
StationArray g_stations_array = {0, 0};

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
  FuelListNode* node = (FuelListNode*)malloc(sizeof(FuelListNode));
  node->fuel_level = fuel_level;
  node->count = count;
  node->left = NULL;
  node->right = NULL;
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

void fuel_list_update_count(FuelListNode* root, uint32_t fuel_level, int32_t delta) {
  FuelListNode* target = fuel_list_search(root, fuel_level);
  if (!target) {
    return;
  }

  if (delta < 0 && -delta > target->count) {
    target->count = 0;
  }
  target->count += delta;
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
  BstStationsListNode* node = (BstStationsListNode*)malloc(sizeof(BstStationsListNode));
  node->station_id = station_id;
  node->fuels = fuel_list_insert_node(node->fuels, max_fuel, count);
  node->left = NULL;
  node->right = NULL;
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
      free(root);
      return NULL;
    }

    // Case 2: One child
    if (root->left == NULL || root->right == NULL) {
      BstStationsListNode* temp = root->right;
      temp = !root->left ? root->right : root->left;
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

  g_stations_array.arr = (int*)malloc(sizeof(int) * (g_stations_size << 2));
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

// Retrieve an array with the stations max available gas fuel associated to stations id [lhs, rhs]
FilteredStationFuel get_stations_id_and_fuels_array(uint32_t lhs, uint32_t rhs, uint32_t size) {
  
  uint32_t* fuel = malloc(sizeof(uint32_t) * size);
  uint32_t* station = malloc(sizeof(uint32_t) * size);
  
  if (!fuel || !station) {
    printf("Malloc failed in <get_stations_fuels_array>\n");
    exit(1);
  }

  uint32_t dest_idx = 0;
  uint32_t start = get_station_index_in_array_from_id(lhs);
  uint32_t end = get_station_index_in_array_from_id(rhs);
  for (uint32_t i=start; i<g_stations_array.size && i <= end; i+=2) {
    fuel[dest_idx] = g_stations_array.arr[i+1]; //save the max fuel
    station[dest_idx++] = g_stations_array.arr[i]; //save the station id
  }

  FilteredStationFuel ans = {
    fuel, station
  };
  return ans;
}

//the fuel index is to be intended coming from the array containing only the fuels, used in the dp computation
static inline uint32_t get_station_id_from_fuel_index(uint32_t idx) {
  if (idx*2 >= g_stations_array.size) {
    // printf("index %d overflow in <get_station_id_from_fuel_index>\n", idx*2);
    return INT_MAX;
  }
  //g_stations_array:[1,2,3,4,5,6] -> with fuel_idx = 2(fuel 6) the original station id associated 5 
  return g_stations_array.arr[idx * 2];
}

static inline uint32_t get_stations_id_distance_diff_from_fuel_index(uint32_t a, uint32_t b) {
  return get_station_id_from_fuel_index(b) - get_station_id_from_fuel_index(a);
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
  return ((get_station_index_in_array_from_id(rhs) - get_station_index_in_array_from_id(lhs)) / 2) - 1;
}

DpArrayRes compute_min_path_dp(int start_station, int end_station) {
  DpArrayRes ret = {0, 0, 0, 0};
  //use bst to seach as log(n)
  BstStationsListNode *start_station_node = stations_list_search(g_stations_bst, start_station);
  BstStationsListNode *end_station_node = stations_list_search(g_stations_bst, end_station);
  if (!start_station_node || !end_station_node) {
    return ret; 
  }

  uint32_t arr_size = get_stations_between(start_station, end_station) + 2; //add the extremes
  //filter out only the needed stations max fuels
  FilteredStationFuel filtered = get_stations_id_and_fuels_array(start_station, end_station, arr_size);
  uint32_t* dp = calloc(arr_size, sizeof(uint32_t));

  // printf("arr size: %d\n", arr_size);
  // for (int i=0; i<arr_size; i++) {
  //   printf("%d ", arr[i]);
  // }
  // printf("\n");

  for(int idx=arr_size-2; idx>=0; idx--){ //do not use uint32_t as this will go negative
    int steps = filtered.fuels[idx];

    int min = INT_MAX;
    int min_idx = -1;
    if(steps > 0){
      for(int i=1; i<=steps; i++){
        if (get_station_id_from_fuel_index(idx) + steps < get_station_id_from_fuel_index(idx+i)) {
          break;
        }
        if(idx+i < arr_size) {
          if (dp[idx+i] <= min) {
              min = dp[idx+i];
              min_idx = idx+i;
          }
        }
      }
    }
    bool can_reach_end = filtered.fuels[idx] >= get_stations_id_distance_diff_from_fuel_index(idx, min_idx);
    if (can_reach_end) {
      dp[idx] = min == INT_MAX ? min : min+1;
    }
    else {
      dp[idx] = 0;
    }
  }

  // printf("dp size: %d\n", arr_size);
  // for (int i=0; i<arr_size; i++) {
  //   printf("%d ", dp[i]);
  // }
  // printf("\n");
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

int main() {
  g_stations_bst = stations_list_insert_node(g_stations_bst, 10, 20, 1);
  g_stations_bst = stations_list_insert_node(g_stations_bst, 20, 30, 5);
  g_stations_bst = stations_list_insert_node(g_stations_bst, 30, 10, 1);
  g_stations_bst = stations_list_insert_node(g_stations_bst, 40, 20, 1);
  g_stations_bst = stations_list_insert_node(g_stations_bst, 50, 10, 1);
  g_stations_bst = stations_list_insert_node(g_stations_bst, 60, 0, 100);
  // print_stations_list_bst(g_stations_bst);

  stations_list_bst_to_array();
  // print_station_array(&g_stations_array);

  DpArrayRes dp_res = compute_min_path_dp(10, 60);
  // for (uint32_t i=0; i<dp_res.size; ++i) {
  //   printf("%d ", dp_res.filtered.stations[i]);
  // }
  // printf("\n\n");
  backtrack_best_route(dp_res);
  free(dp_res.dp);
  free(dp_res.filtered.fuels);
  free(dp_res.filtered.stations);

  stations_list_free(g_stations_bst);
  free(g_stations_array.arr);

  return 0;
}

