#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define PATH_NOT_FOUND -1;

const char AGGIUNTA[] = "aggiunta";
const char NON_AGGIUNTA[] = "non aggiunta";
const char DEMOLITA[] = "demolita";
const char NON_DEMOLITA[] = "non demolita";
const char ROTTAMATA[] = "rottamata";
const char NON_ROTTAMATA[] = "non rottamata";
const char NP[] = "nessun percorso";

typedef struct {
  int size;
  int* arr; //flatten 2d array ([station,fuel]) into 1d array
} StationArray;

typedef struct BstNode {
  int station_dist;
  int max_fuel;
  struct BstNode* left;
  struct BstNode* right;
} BstNode;

typedef struct {
  int max_fuel; //node value is node max_fuel
  int station_distance;
} PriorityQueueNode;

typedef struct {
  PriorityQueueNode* heap;
  int capacity;
  int size;
} PriorityQueue;

int stations_size = 0;
BstNode *stations_bst = 0;
StationArray stations_array = {0, 0};

//------------------------max_fuel QUEUE------------------------

//This station_dist structure is used to compute the minimum path between two stations_bst

PriorityQueue* create_priority_queue(int capacity) {
  PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
  pq->heap = (PriorityQueueNode*)malloc((capacity) * sizeof(PriorityQueueNode));
  pq->capacity = capacity;
  pq->size = 0;
  return pq;
}

void swap(PriorityQueueNode* a, PriorityQueueNode* b) {
  PriorityQueueNode temp = *a;
  *a = *b;
  *b = temp;
}

void heapify(PriorityQueue* pq, int index) {
  PriorityQueueNode* array = pq->heap; 
  int size = pq->size;
  if (size > 1) {
    // Find the largest among root, left child and right child
    int largest = index;
    int l = 2 * index + 1;
    int r = 2 * index + 2;
    if (l < size && array[l].max_fuel > array[largest].max_fuel)
      largest = l;
    if (r < size && array[r].max_fuel > array[largest].max_fuel)
      largest = r;

    // Swap and continue heapifying if root is not largest
    if (largest != index) {
      swap(&array[index], &array[largest]);
      heapify(pq, largest);
    }
  }
}

void enqueue_priority_queue(PriorityQueue* pq, int max_fuel, int station_dist) {
  PriorityQueueNode* array = pq->heap;
  if (pq->size == 0) {
    array[0].max_fuel = max_fuel;
    array[0].station_distance = station_dist;
    pq->size++;
  } else {
    array[pq->size].max_fuel = max_fuel;
    array[pq->size].station_distance = station_dist;
    pq->size++;
    for (int i=(pq->size / 2 - 1); i >= 0; i--) {
      heapify(pq, i);
    }
  }
}

//currently, a not present node dequeue is not supported
void dequeue_priority_queue(PriorityQueue* pq, int priority) {
  PriorityQueueNode* array = pq->heap;
  int i;
  for (i = 0; i < pq->size; i++) {
    if (priority == array[i].max_fuel)
      break;
  }

  swap(&array[i], &array[pq->size - 1]);
  pq->size--;
  for (int i=(pq->size / 2 - 1); i >= 0; i--) {
    heapify(pq, i);
  }
}

PriorityQueueNode* front_priority_queue(PriorityQueue* pq) {
  if (pq->size == 0) {
    printf("PriorityQueue is empty. No front_priority_queue element.\n");
    return 0;
  }

  return &(pq->heap[0]);
}

int is_priority_queue_empty(PriorityQueue* pq) {
  return pq->size == 0;
}

void print_priority_queue(PriorityQueue* pq) {
  for (int i = 0; i < pq->size; ++i)
    printf("f: %d - s: %d\n", pq->heap[i].max_fuel, pq->heap[i].station_distance);
  printf("\n\n");
}

void fee_priority_queue(PriorityQueue* pq) {
  free(pq->heap);
  free(pq);
}

//------------------------BINARY SEARCH TREE------------------------

//This station_dist structure is used to store the stations_bst with the max available car autonomy in a sorted manner

BstNode* create_bst_node(int station_dist, int max_fuel) {
  BstNode* newNode = (BstNode*)malloc(sizeof(BstNode));
  newNode->station_dist = station_dist;
  newNode->max_fuel = max_fuel;
  newNode->left = NULL;
  newNode->right = NULL;
  return newNode;
}

BstNode* insert_bst_node(BstNode* root, int station_dist, int max_fuel) {
  if (root == NULL) {
    stations_size++;
    return create_bst_node(station_dist, max_fuel);
  }

  if (station_dist < root->station_dist) {
    root->left = insert_bst_node(root->left, station_dist, max_fuel);
  } else {
    root->right = insert_bst_node(root->right, station_dist, max_fuel);
  }

  return root;
}

BstNode* find_min_bst_node(BstNode* node) {
  while (node->left != NULL) {
    node = node->left;
  }
  return node;
}

BstNode* remove_bst_node(BstNode* root, int station_dist) {
  if (root == NULL) {
    return root;
  }

  if (station_dist < root->station_dist) {
    root->left = remove_bst_node(root->left, station_dist);
  } else if (station_dist > root->station_dist) {
    root->right = remove_bst_node(root->right, station_dist);
  } else {
    stations_size--;

    // Case 1: No child or one child
    if (!root->left && !root->right) {
      free(root);
      return NULL;
    }

    // Case 2: One child
    if (root->left == NULL || root->right == NULL) {
      BstNode* temp = root->right;
      temp = !root->left ? root->right : root->left;
      free(root);
      return temp;
    }

    // Case 3: Two children
    BstNode* minRight = find_min_bst_node(root->right);
    root->station_dist = minRight->station_dist;
    root->right = remove_bst_node(root->right, minRight->station_dist);
  }

  return root;
}

BstNode* search_bst(BstNode* root, int station_dist) {
  if (root == NULL || root->station_dist == station_dist) {
    return root;
  }

  if (station_dist < root->station_dist) {
    return search_bst(root->left, station_dist);
  } else {
    return search_bst(root->right, station_dist);
  }
}

void print_stations_bst(BstNode* root) {
  if (root == NULL) {
    return;
  }

  print_stations_bst(root->left);
  printf("s: %d - f: %d\n", root->station_dist, root->max_fuel);
  print_stations_bst(root->right);
}

void free_bst(BstNode* root) {
  if (root == NULL) {
    return;
  }

  free_bst(root->left);
  free_bst(root->right);
  free(root);
}

void print_stations_bst_to_array(BstNode* root) {
  if (root == NULL) {
    return;
  }

  print_stations_bst_to_array(root->left);
  stations_array.arr[stations_array.size++] = root->station_dist; 
  stations_array.arr[stations_array.size++] = root->max_fuel; 
  print_stations_bst_to_array(root->right);
}

void bst_to_array() {
  if (stations_array.arr != NULL) {
    free(stations_array.arr);
    stations_array.arr = NULL;
    stations_array.size = 0;
  }

  stations_array.arr = (int*)malloc(sizeof(int) * (stations_size << 2));
  print_stations_bst_to_array(stations_bst);
}

void print_station_array(StationArray* s) {
  if (!s) {
    return;
  }

  for (int i=0; i<s->size; i+=2) {
    printf("s: %d - f: %d\n", s->arr[i], s->arr[i+1]);
  }
}

int compute_min_path(int start_station, int end_station) {
  BstNode *start_station_node = search_bst(stations_bst, start_station);
  BstNode *end_station_node = search_bst(stations_bst, end_station);
  if (!start_station_node || !end_station_node) {
    return PATH_NOT_FOUND; 
  }

  int start_station_fuel = start_station_node->max_fuel;
  int start_station_id = start_station_node->station_dist;
  int start_fuel = start_station_node->station_dist;
  // int end_station_fuel = end_station_node->max_fuel;
  int end_station_id = end_station_node->station_dist;
  int idx = 0, res = 0;
  PriorityQueue* pq = create_priority_queue(stations_size); //TODO: optimize size
  enqueue_priority_queue(pq, start_fuel, 0);

  while (!is_priority_queue_empty(pq)) {
    PriorityQueueNode* front = front_priority_queue(pq);
    int curr_fuel = front->max_fuel;
    int pos = front->station_distance;
    dequeue_priority_queue(pq, curr_fuel);
    // printf("pos: %d\n", pos);
    if (pos + curr_fuel >= end_station_id) {
      return res; 
    }

    while (idx/2 < stations_size && pos + curr_fuel >= stations_array.arr[idx]) {
      // printf("add to queue f: %d - s: %d\n", stations_array.arr[idx+1], stations_array.arr[idx]);
      enqueue_priority_queue(pq, stations_array.arr[idx+1], stations_array.arr[idx]);
      idx += 2;
    }
    res++;
  }

  return PATH_NOT_FOUND;
}

int main() {

  stations_bst = insert_bst_node(stations_bst, 10, 60);
  stations_bst = insert_bst_node(stations_bst, 20, 30);
  stations_bst = insert_bst_node(stations_bst, 30, 30);
  stations_bst = insert_bst_node(stations_bst, 60, 40);
  stations_bst = insert_bst_node(stations_bst, 100, 10);

  // print_stations_bst(stations_bst);

  bst_to_array();
  print_station_array(&stations_array);
  
  printf("res: %d\n", compute_min_path(10, 100));

  free_bst(stations_bst);
  free(stations_array.arr);

  return 0;
}

// example of priority queue usage
// int main() {
//   PriorityQueue* pq = create_priority_queue(10);

//   enqueue_priority_queue(pq, 3, 30);
//   enqueue_priority_queue(pq, 1, 10);
//   enqueue_priority_queue(pq, 5, 50);
//   enqueue_priority_queue(pq, 2, 20);

//   print_priority_queue(pq);

//   dequeue_priority_queue(pq, 5);

//   print_priority_queue(pq);

//   enqueue_priority_queue(pq, 5, 50);

//   print_priority_queue(pq);

//   dequeue_priority_queue(pq, 3);

//   print_priority_queue(pq);

//   fee_priority_queue(pq);
//   return 0;
// }

//example of bst usage
// int main() {
//     BstNode* root = NULL;

//     // Inserting elements into the BST
//     root = insert_bst_node(root, 5, 10);
//     root = insert_bst_node(root, 3, 10);
//     root = insert_bst_node(root, 7, 10);
//     root = insert_bst_node(root, 1, 10);
//     root = insert_bst_node(root, 4, 10);

//     // Inorder traversal of the BST (sorted list)
//     printf("Sorted list (inorder traversal): ");
//     print_stations_bst(root);
//     printf("\n");

//     // Freeing the memory allocated for the BST
//     free_bst(root);

//     return 0;
// }
