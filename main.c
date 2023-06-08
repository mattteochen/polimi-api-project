#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG_DP_ARRAY 0
#define DEBUG_FILTER 0
#define DEBUG_INPUT 0
#define DEBUG_PATH_CMP 0
#define DEBUG_QUEUE_PTR 0
#define DEBUG_MAX_QUEUE 0
#define USE_DFS 0
#define USE_BSF 0
#define USE_DP 1

#if USE_DFS || USE_BSF
#include <math.h>
#endif

#define TO_BE_DELETED 1
#define NOT_BE_DELETED 2

#define MAX_FUEL_DIFFERENT_COUNT 513

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
  uint32_t size; //used to track how much buffer has been used, after a flatten (from a bst) action this must be equal to the max buf size by design
  uint32_t* arr; //flatten 2d array ([station,fuel]) into 1d array
} StationArray;

typedef struct {
  uint32_t max_reach;
  uint32_t steps;
} Dp;

typedef struct {
  uint32_t size;
  Dp* dp;
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

#if USE_BFS
typedef struct {
  int32_t idx;
  int32_t jumps;
  uint32_t* buff;
  uint32_t buff_idx;
} Pair;
#endif

uint32_t g_stations_size = 0;
BstStationsListNode* g_stations_bst = 0;
BstStationsListNode* g_last_created_station_node = 0;
BstStationsListNode* g_last_modified_station_node = 0;
uint8_t g_map_changed = 0;
StationArray g_stations_array = {0, 0};

int32_t compare( const void* a, const void* b) {
  uint32_t int_a = * ( (uint32_t*) a );
  uint32_t int_b = * ( (uint32_t*) b );

  if ( int_a == int_b ) return 0;
  else if ( int_a < int_b ) return -1;
  else return 1;
}

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

// //------------------------BINARY SEARCH TREE - FUEL LIST------------------------

// //This structure is used to store the all the fuels in a sorted manner for a given station

static inline FuelListNode* fuel_list_create_node(uint32_t fuel_level, uint32_t count) {
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

uint8_t fuel_list_update_count(FuelListNode* root, uint32_t fuel_level, int8_t delta) {
  FuelListNode* t = fuel_list_search(root, fuel_level);
  if (!t) {
    return 0;
  }

  if (delta == -1 && t->count == 1) {
    return TO_BE_DELETED;
  } else {
    t->count += delta;  
    return NOT_BE_DELETED;
  }
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

void fuel_list_bst_to_array_rec(FuelListNode* root, uint32_t* buff, uint32_t* idx) {
  if (root == NULL) {
    return;
  }

  fuel_list_bst_to_array_rec(root->left, buff, idx);
  buff[*idx] = root->fuel_level;
  (*idx)++;
  buff[*idx] = root->count;
  (*idx)++;
  fuel_list_bst_to_array_rec(root->right, buff, idx);
}

uint32_t* fuel_list_bst_to_array(FuelListNode* root) {
  if (!root) {
    return 0;
  }
  uint32_t* buff = malloc(sizeof(uint32_t) * MAX_FUEL_DIFFERENT_COUNT * 2);
  memset(buff, -1, sizeof(uint32_t) * MAX_FUEL_DIFFERENT_COUNT * 2); //max 512 different fuel levels
  uint32_t idx = 0;
  fuel_list_bst_to_array_rec(root, buff, &idx);
  return buff;
}

//------------------------BINARY SEARCH TREE - STATION LIST------------------------

//This station_id structure is used to store the g_stations_bst with the max available car autonomy in a sorted manner

static inline BstStationsListNode* stations_list_create_node(int station_id, uint32_t max_fuel, uint32_t count) {
  BstStationsListNode* node = calloc(1, sizeof(BstStationsListNode));
  node->station_id = station_id;
  node->fuels = fuel_list_insert_node(node->fuels, max_fuel, count);
  g_last_created_station_node = node;
  return node;
}

BstStationsListNode* stations_list_insert_node(BstStationsListNode* root, uint32_t station_id, uint32_t max_fuel, uint32_t count) {
  if (root && root->station_id == station_id) {
    return root;
  }
  if (!root) {
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
    // Case 1: No child
    if (!root->left && !root->right) {
      if (g_last_modified_station_node == root) {
        g_last_modified_station_node = 0;
      }
      fuel_list_free(root->fuels); 
      free(root);
      root = 0;
      g_stations_size--;
      return NULL;
    }

    // Case 2: One child
    if (root->left == NULL || root->right == NULL) {
      BstStationsListNode* temp = !root->left ? root->right : root->left;
      if (g_last_modified_station_node == root) {
        g_last_modified_station_node = 0;
      }
      fuel_list_free(root->fuels); 
      free(root);
      root = 0;
      g_stations_size--;
      return temp;
    }

    // Case 3: Two children, we have to remap the moved station id fuel list too!
    BstStationsListNode* minRight = stations_list_find_min_node(root->right);
    root->station_id = minRight->station_id;
    fuel_list_free(root->fuels);
    root->fuels = 0;
    uint32_t* fuels_to_copy = fuel_list_bst_to_array(minRight->fuels);
    if (!fuels_to_copy) {
      printf("Fuels to copy is null\n");
      exit(1);
    }
    for (uint32_t i=0; i<MAX_FUEL_DIFFERENT_COUNT*2 && fuels_to_copy[i] != -1; i+=2) {
      root->fuels = fuel_list_insert_node(root->fuels, fuels_to_copy[i], fuels_to_copy[i+1]);
    }
    free(fuels_to_copy);
    g_map_changed = 1;
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
  //TODO: cache the max node
  FuelListNode* max = fuel_list_find_max_node(root->fuels);
  g_stations_array.arr[g_stations_array.size++] = max ? max->fuel_level : 0; 
  stations_list_bst_to_array_recursive(root->right);
}

void stations_list_bst_to_array() {
  if (g_stations_size < 1) {
    return;
  }
  //overwrite old buffer if buffer exist
  if (g_stations_array.arr && g_stations_size*2 <= g_stations_array.size) {
    g_stations_array.size = 0;
  } else if (g_stations_array.arr && g_stations_size*2 > g_stations_array.size) {
    //realloc
    uint32_t* new_buf = realloc(g_stations_array.arr, sizeof(uint32_t) * g_stations_size * 2);
    if (!new_buf) {
      printf("Realloc failed\n");
      exit(1);
    }
    g_stations_array.size = 0;
    g_stations_array.arr = new_buf;
  //first allocation
  } else if (!g_stations_array.arr) {
    g_stations_array.arr = (uint32_t*)malloc(sizeof(uint32_t) * (g_stations_size * 2));
    g_stations_array.size = 0;
  }
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

//binary search
int32_t find_sorted_arr(uint32_t* arr, uint32_t t, int32_t low, int32_t high) {
  while (low <= high) {
    int32_t mid = low + (high - low) / 2;

    if (arr[mid] == t) {
      return mid;
    }

    if (arr[mid] < t) {
      low = mid + 1;
    } else {
      high = mid - 1;
    }
  }

  return -1;
}

int find_closest_sorted_arr(uint32_t* arr, int size, int t) {
	if (t <= arr[0]) {
		return 0;
  }
	if (t >= arr[size - 1]) {
		return size - 1;
  }

	int i = 0, j = size, mid = 0;
	while (i < j) {
		mid = (i + j) / 2;

		if (arr[mid] == t) {
			return mid;
    }

		if (t < arr[mid]) {
			if (mid > 0 && t > arr[mid - 1]) {
				return mid;
      }
			j = mid;
		} else {
			if (mid < size - 1 && t < arr[mid + 1]) {
				return mid+1;
      }
			i = mid + 1;
		}
	}

	return mid;
}

//the caller must check that start and end stations do exist in the current map
DpArrayRes compute_min_path_dp(int start_station, int end_station) {
  DpArrayRes ret = {0, 0, {0, 0}};

  uint32_t arr_size = get_stations_between(start_station, end_station) + 2; //add the extremes
  //filter out only the needed stations max fuels
  FilteredStationFuel filtered =
    get_stations_id_and_fuels_array(start_station, end_station, arr_size, start_station < end_station ? 1 : -1);
  Dp* dp = calloc(arr_size, sizeof(Dp));

#if DEBUG_FILTER
  printf("arr size: %d\n", arr_size);
  for (int i=0; i<arr_size; i++) {
    printf("s: %d f: %d m %d\n", filtered.stations[i], filtered.fuels[i], start_station > end_station ? filtered.stations[i]-filtered.fuels[i] : filtered.stations[i] + filtered.fuels[i]);
  }
  printf("\n");
#endif

#if USE_DP
  if (start_station < end_station) {
#endif
    dp[arr_size-1].steps = 0;
    dp[arr_size-1].max_reach = arr_size-1;
    for(int idx=(arr_size-2); idx>=0; idx--){ //do not use uint32_t as this will go negative
      uint32_t steps = filtered.fuels[idx];

      uint32_t min = INT_MAX;
      int32_t max_reach = INT_MIN;
      if(steps > 0){
        for(uint32_t i=idx+1; i<arr_size; ++i){
          if ((start_station < end_station && filtered.stations[idx] + steps < filtered.stations[i]) ||
              (start_station > end_station && steps < filtered.stations[idx] && filtered.stations[idx] - steps > filtered.stations[i])) {
            break;
          }
          if ((start_station < end_station && filtered.stations[idx] + steps >= filtered.stations[i] && dp[i].steps <= min) ||
              (start_station > end_station && steps >= filtered.stations[idx] && dp[i].steps <= min) ||
              (start_station > end_station && filtered.stations[idx] - steps <= filtered.stations[i] && dp[i].steps <= min)) {
            min = dp[i].steps;
            max_reach = i;
          }
        }
      }
      dp[idx].steps = min == INT_MAX ? min : min+1;
      dp[idx].max_reach = max_reach == INT_MIN ? 0 : max_reach; //null pointer signals that max reach is station[idx] + fuels[idx] 
    }
#if USE_DP
  }
#endif

#if (DEBUG_DP_ARRAY)
  for (uint32_t i=0; i<arr_size; ++i) {
    printf("station %d dp %d mr %d\n", filtered.stations[i], dp[i].steps,
           dp[i].max_reach ? filtered.stations[dp[i].max_reach] : start_station < end_station ? filtered.stations[i] + filtered.fuels[i] : filtered.stations[i] - filtered.fuels[i]);
  }
#endif

#if USE_DP
  //other methods like bfs and dfs uses the dp_res to backtrack the best path indipendently from the route direction.
  //here with the dp table backtrack we need a turnaround in case start_station > end_station
  if (start_station > end_station) {
    FilteredStationFuel forward_filter =
      get_stations_id_and_fuels_array(start_station, end_station, arr_size, 1);
    
    Dp* forward_dp = calloc(arr_size, sizeof(Dp));
    for (uint32_t i=0; i<arr_size; i++) {
      forward_dp[i].steps = INT_MAX;
    }
    forward_dp[arr_size-1].steps = 0;
    forward_dp[arr_size-1].max_reach = arr_size-1;

    int32_t curr_idx = arr_size-1;
    int32_t curr_idx_rhs = arr_size-1;
    uint32_t curr_dp_steps = 0;
    while (curr_idx >= 0) {
      //fill dp[x] = 1
      if (curr_dp_steps == 0) {
        //get the leftmost bound
        int32_t m_r_limit = forward_filter.stations[curr_idx] - forward_filter.fuels[curr_idx]; 
        if (m_r_limit < 0) {
          m_r_limit = 0;
        }
        int32_t new_idx_rhs = -1;
        for (uint32_t i=find_closest_sorted_arr(forward_filter.stations, arr_size, m_r_limit); i<arr_size-1; i++) {
          if (new_idx_rhs == -1) {
            new_idx_rhs = i;
          }
          forward_dp[i].steps = curr_dp_steps+1;
          forward_dp[i].max_reach = arr_size-1;
        }
        curr_idx = new_idx_rhs - 1;
        curr_idx_rhs = new_idx_rhs;
        curr_dp_steps++;
      //fill others
      } else {
        //get the max step_left_limit across all the dp[curr_dp_steps]
        int32_t max_step_left_limit = INT_MAX;
        //don't stop after the dp[i] changes value, we could have a dp sequence like: ...7776565444...
        //yes, this O(n) is quite pricy as it is run every time, maybe consider a map saving the rhs and lhs
        for (uint32_t i=curr_idx_rhs; i<arr_size; i++) {
          if (forward_dp[i].steps != curr_dp_steps) {
            continue;
          }
          int32_t m_r_limit = forward_filter.stations[i] - forward_filter.fuels[i]; 
          if (m_r_limit < 0) {
            m_r_limit = 0;
          }
          if (m_r_limit < max_step_left_limit) {
            max_step_left_limit = m_r_limit;
          }
        }
        int32_t new_idx_rhs = -1;
        for (uint32_t i=find_closest_sorted_arr(forward_filter.stations, arr_size, max_step_left_limit); i<curr_idx_rhs; i++) {
          if (new_idx_rhs == -1) {
            new_idx_rhs = i;
          }
          //now this index i can be reached cause it is <= max_step_left_limit, find the min parent
          for (uint32_t j=curr_idx_rhs; i<arr_size; j++) {
            if (forward_dp[j].steps != curr_dp_steps) {
              continue;
            }
            int32_t m_r_limit = forward_filter.stations[j] - forward_filter.fuels[j]; 
            if (m_r_limit < 0) {
              m_r_limit = 0;
            }
            if (forward_filter.stations[i] >= m_r_limit) {
              forward_dp[i].steps = curr_dp_steps + 1;
              forward_dp[i].max_reach = j;
              break; //found the min, exit
            }
          }
        }
        curr_idx = new_idx_rhs - 1;
        curr_idx_rhs = new_idx_rhs;
        curr_dp_steps++;
      }
    }

    if (forward_dp[0].steps != INT_MAX) {
      //create the solution
      int32_t* buff = calloc(arr_size, sizeof(int32_t));
      memset(buff, -1, sizeof(int32_t) * arr_size);
      uint32_t buff_idx = 0;
      Dp* curr_ptr = forward_dp;
      buff[buff_idx++] = forward_filter.stations[0];

      while (curr_ptr < &forward_dp[arr_size-1]) {
        //save the id
        buff[buff_idx++] = forward_filter.stations[curr_ptr->max_reach];
        //move to to the next ptr
        curr_ptr = &forward_dp[curr_ptr->max_reach];
      }
      for (int32_t i=buff_idx-1; i>=0; i--) {
        if (i == 0) {
          printf("%d\n", buff[i]);
        } else {
          printf("%d ", buff[i]);
        }
      }
      free(buff);
    } else {
      printf("%s\n", NP);
    }

    free(forward_dp);
    free(forward_filter.stations);
    free(forward_filter.fuels);
  } else if (start_station < end_station && dp[0].steps != INT_MAX) {
    Dp* ptr = dp;
    printf("%d ", filtered.stations[0]);
    while (ptr < &(dp[arr_size-1])) {
      int32_t min_idx = ptr->max_reach;
      if (ptr->max_reach == arr_size-1) {
        printf("%d\n", filtered.stations[arr_size-1]);
        break;
      } else {
        //go backwards to find the min idx with the same dp result
        for (int32_t i=ptr->max_reach-1; i>=0; i--) {
          if (dp[i].steps == dp[ptr->max_reach].steps) {
            min_idx = i;
          }
        }
        //min_idx is the new max_reach
        printf("%d ", filtered.stations[min_idx]);
      }
      ptr = &(dp[min_idx]);
    }
  } else {
    printf("%s\n", NP);
  }
#endif

  ret.dp = dp;
  ret.filtered = filtered;
  ret.size = arr_size;
  return ret;
}

#if USE_BSF
//given a best dp array, backtrack to the best path
void backtrack_bfs_best_route(DpArrayRes in, uint32_t is_forward) {

  const uint32_t* fuels = in.filtered.fuels;
  const uint32_t* stations = in.filtered.stations;
  const Dp* dp = in.dp;
  const uint32_t size = in.size; 

  uint32_t* storage_buf = 0;
  uint32_t storage_buf_idx = 0;
 
  uint32_t queue_front = 0;
  uint32_t queue_back = 0;
  uint32_t queue_size = 100 * pow(2,20);
  Pair* queue = calloc(queue_size, sizeof(Pair)); //TODO: dynamic size
  queue[queue_back].idx = 0;
  queue[queue_back].jumps = dp[0].steps;
  queue[queue_back].buff_idx = 0;
  queue[queue_back++].buff = calloc(size, sizeof(uint32_t)); //size is the max upperbound
  
#if DEBUG_MAX_QUEUE 
  uint32_t max = 0;
#endif

  while (queue_back != queue_front) {
#if DEBUG_QUEUE_PTR
    printf("qf: %d qb:%d\n", queue_front, queue_back);
#endif
    Pair p = queue[queue_front++]; //increment front pointer
#if DEBUG_MAX_QUEUE 
    if (queue_back > max) max = queue_back;
#endif
    
    if (is_forward && p.jumps == 0) {
      printf("%d ", stations[0]);
      for (uint32_t i=0; i<p.buff_idx; i++) {
        if (i != p.buff_idx-1) {
          printf("%d ", stations[p.buff[i]]);
        } else {
          printf("%d", stations[p.buff[i]]);
        }
      }
      printf("\n");
      break;
    }

    if (p.jumps == 0) {
      if (!storage_buf) {
        storage_buf = p.buff;
        storage_buf_idx = p.buff_idx;
      } else {
#if DEBUG_PATH_CMP
        printf("comparing ");
        printf("%d ", stations[0]);
        for (uint32_t i=0; i<storage_buf_idx; i++) {
          if (i != storage_buf_idx-1) {
            printf("%d ", stations[storage_buf[i]]);
          } else {
            printf("%d", stations[storage_buf[i]]);
          }
        }
        printf("\n");
        printf("with      ");
        printf("%d ", stations[0]);
        for (uint32_t i=0; i<p.buff_idx; i++) {
          if (i != p.buff_idx-1) {
            printf("%d ", stations[p.buff[i]]);
          } else {
            printf("%d", stations[p.buff[i]]);
          }
        }
        printf("\n");
#endif
        int8_t w = 0;
        for (int32_t i=p.buff_idx-1; i>=0; i--) {
#if DEBUG_PATH_CMP
          printf("%d vs %d\n", stations[storage_buf[i]], stations[p.buff[i]]);
#endif
          if (stations[storage_buf[i]] < stations[p.buff[i]]) {
            w = 1;
            break;
          } else if (stations[storage_buf[i]] > stations[p.buff[i]]) {
            w = 2;
            break;
          }
        }

#if DEBUG_PATH_CMP
        printf("winner: %d\n", w);
#endif
        //assign winner path
        if (w == 2) {
          storage_buf = p.buff;
          storage_buf_idx = p.buff_idx;
        }
      }
    }

    for (uint32_t i=p.idx+1; i<size; ++i) {
      if (p.jumps-1 == dp[i].steps) {
        //check if fuel can reach next dp station
        if (is_forward && stations[p.idx] + fuels[p.idx] < stations[i]) {
          continue;
        }
        if (!is_forward && fuels[p.idx] < stations[p.idx] && stations[p.idx] - fuels[p.idx] > stations[i]) {
          continue;
        }

        //TODO: no space left
        if (queue_back >= queue_size) {
          printf("********need to realloc queue********\n");
          printf("filter size is %d and queue size is %d\n", size, queue_size);
          exit(1);
          uint32_t old_size = queue_size;
          Pair* new_queue = realloc(queue, queue_size * 2 * sizeof(Pair));
          if (!new_queue) {
            printf("Realloc failed\n");
            exit(1);
          }
          // memset(&queue[queue_back], 0, sizeof(Pair) * queue_size); //this gives memory issues
          uint32_t new_size = queue_size * 2;
          queue_size *= 2;
          queue = new_queue;
        }

        queue[queue_back].idx = i;
        queue[queue_back].jumps = p.jumps-1;
        queue[queue_back].buff = calloc(size, sizeof(uint32_t));

        //copy the p buffer into the new node
        memcpy(queue[queue_back].buff, p.buff, sizeof(uint32_t) * p.buff_idx);
        queue[queue_back].buff_idx = p.buff_idx;
        queue[queue_back].buff[queue[queue_back].buff_idx++] = i;
        queue_back++;
      }
    }
  }

  if (!is_forward) {
    //TODO: use a single stdout
    printf("%d ", stations[0]);
    for (uint32_t i=0; i<storage_buf_idx; i++) {
      if (i != storage_buf_idx-1) {
        printf("%d ", stations[storage_buf[i]]);
      } else {
        printf("%d", stations[storage_buf[i]]);
      }
    }
    printf("\n");
  }

  for (uint32_t i=0; i<queue_size; ++i) {
    if (queue[i].buff) {
      free(queue[i].buff);
    }
  }
  free(queue);

#if DEBUG_MAX_QUEUE 
  printf("max %d\n", max);
#endif
}
#elif USE_DFS
uint8_t dfs_stop = 0;

void backtrack_dfs_best_route_rev(DpArrayRes dp_res, uint32_t last_dp, uint32_t index, uint32_t* buff, uint32_t buff_idx) {

  Dp* dp = dp_res.dp;
  uint32_t* stations = dp_res.filtered.stations;
  uint32_t* fuels = dp_res.filtered.fuels;
  uint32_t size = dp_res.size;

  buff[buff_idx] = index;

  if (stations[index] == stations[0] || index == 0) {
    for (int32_t i=buff_idx; i>=0; i--) {
      if (i == 0) {
        printf("%d\n", stations[buff[i]]);
      } else {
        printf("%d ", stations[buff[i]]);
      }
    }
    dfs_stop = 1;
    return;
  }

  for (int32_t i=index-1; i>=0; i--) {
    if (dp[i].steps == last_dp+1 && ((fuels[i] >= stations[i])||(stations[i] - fuels[i] <= stations[index]))) {
      if (!dfs_stop) {
        backtrack_dfs_best_route_rev(dp_res, dp[i].steps, i, buff, buff_idx+1);
      }
    }
  }
}

void backtrack_dfs_best_route_for(DpArrayRes dp_res, uint32_t last_dp, uint32_t index, uint32_t* buff, uint32_t buff_idx) {

  Dp* dp = dp_res.dp;
  uint32_t* stations = dp_res.filtered.stations;
  uint32_t* fuels = dp_res.filtered.fuels;
  uint32_t size = dp_res.size;

  buff[buff_idx] = index;

  if (stations[index] == stations[size-1] || index == size-1) {
    for (uint32_t i=0; i<=buff_idx; i++) {
      if (i == buff_idx) {
        printf("%d\n", stations[buff[i]]);
      } else {
        printf("%d ", stations[buff[i]]);
      }
    }
    dfs_stop = 1;
    return;
  }

  for (int32_t i=index+1; i<size; i++) {
    if (dp[i].steps == last_dp-1 && ((stations[index] + fuels[index] >= stations[i]))) {
      if (!dfs_stop) {
        backtrack_dfs_best_route_for(dp_res, dp[i].steps, i, buff, buff_idx+1);
      }
    }
  }
}
#endif

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

  //get car nums
  storage_idx = 0;
  while (input_buf[idx] != ' ' && input_buf[idx] != '\n' && input_buf[idx] != '\0') {
    storage[storage_idx++] = input_buf[idx++];
  }
  idx++;
  storage[storage_idx] = '\0';

  str2int(&cars, storage, 10);
#if DEBUG_INPUT
  printf("cars: %d\n", cars);
#endif

  if (cars == 0) {
    uint32_t prev_count = g_stations_size;
    g_stations_bst = stations_list_insert_node(g_stations_bst, station_id, 0, 1);
    if (g_stations_size == prev_count) {
      printf("%s/n", NON_AGGIUNTA);
    } else {
      printf("%s\n", AGGIUNTA);
      //signal the map has changed
      g_map_changed = 1;
    }
    return;
  }

  uint32_t* fuels = calloc(cars, sizeof(uint32_t));
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

    fuels[i] = fuel;
  }
  
  qsort(fuels, cars, sizeof(uint32_t), compare);
  for (uint32_t i=0; i<cars; i++) {
    //first fuel ? if yes add station and fuel
    if (i == 0) {
      uint32_t prev_count = g_stations_size;
      g_stations_bst = stations_list_insert_node(g_stations_bst, station_id, fuels[i], 1);
      if (prev_count == g_stations_size) {
        printf("%s\n", NON_AGGIUNTA);
        free(fuels);
        return;
      }
    //else update the node fuel values
    } else {
      if (fuels[i] == fuels[i-1]) {
        fuel_list_update_count(g_last_created_station_node->fuels, fuels[i], 1); 
      } else {
        g_last_created_station_node->fuels = fuel_list_insert_node(g_last_created_station_node->fuels, fuels[i], 1);
      }
    }
  }

  printf("%s\n", AGGIUNTA);

  free(fuels);

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

  BstStationsListNode* station_node = 0;
  if (g_last_modified_station_node && g_last_modified_station_node->station_id == station_id) {
    station_node = g_last_modified_station_node;
  } else {
    station_node = stations_list_search(g_stations_bst, station_id);
  }
  if (!station_node) {
    g_last_modified_station_node = 0;
    printf("%s\n", NON_AGGIUNTA);
    return;
  }

  //update the last modified station node
  g_last_modified_station_node = station_node;

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

  //try to update an ond node count with the same fuel level
  uint8_t res = fuel_list_update_count(station_node->fuels, fuel, 1);
  if (!res) {
    station_node->fuels = fuel_list_insert_node(station_node->fuels, fuel, 1);
  }

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

  BstStationsListNode* station_node = 0;
  if (g_last_modified_station_node && g_last_modified_station_node->station_id == station_id) {
    station_node = g_last_modified_station_node;
  } else {
    station_node = stations_list_search(g_stations_bst, station_id);
  }
  if (!station_node) {
    g_last_modified_station_node = 0;
    printf("%s\n", NON_ROTTAMATA);
    return;
  }

  //update the last modified station node
  g_last_modified_station_node = station_node;

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

  int8_t res = fuel_list_update_count(station_node->fuels, fuel, -1);
  if (res) {
    printf("%s\n", ROTTAMATA);

    if (res == TO_BE_DELETED) {
      station_node->fuels = fuel_list_remove_node(station_node->fuels, fuel);
    }

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

  //early return if equal
  if(from == to) {
    printf("%d\n", from);
    return;
  }

  //compute the flatten road, if changed
  if (g_map_changed) {
    g_map_changed = 0;
    stations_list_bst_to_array();
  }
  //compute min path with dp and rebuild the path
  DpArrayRes dp_res = compute_min_path_dp(from, to);
#if USE_BSF || USE_DFS
  if (dp_res.dp[0].steps != INT_MAX) {
#if USE_BSF
    backtrack_bfs_best_route(dp_res, from < to);
#elif USE_DFS
    dfs_stop = 0;
    uint32_t* buff = calloc(dp_res.size, sizeof(uint32_t));
    if (from > to) {
      backtrack_dfs_best_route_rev(dp_res, 0, dp_res.size-1, buff, 0);
    } else {
      backtrack_dfs_best_route_for(dp_res, dp_res.dp[0].steps, 0, buff, 0);
    }
    free(buff);
#endif
  } else {
    printf("%s\n", NP);
  }
#endif
  free(dp_res.dp);
  free(dp_res.filtered.fuels);
  free(dp_res.filtered.stations);
}

void parse_cmd() {
  uint8_t buf[1024 * 100];
  memset(buf, 0, sizeof(uint8_t) * 1024 * 100);
  while(fgets((char*)buf, (1024 * 100 - 1), stdin)) {
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

