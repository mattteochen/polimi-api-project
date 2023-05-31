#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

typedef struct {
  int max_fuel; //node value is node max_fuel
  int station_distance;
} PriorityQueueNode;

typedef struct {
  PriorityQueueNode* heap;
  int capacity;
  int size;
} PriorityQueue;

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

// example of priority queue usage
int main() {
  PriorityQueue* pq = create_priority_queue(10);

  enqueue_priority_queue(pq, 3, 30);
  enqueue_priority_queue(pq, 1, 10);
  enqueue_priority_queue(pq, 5, 50);
  enqueue_priority_queue(pq, 2, 20);

  print_priority_queue(pq);

  dequeue_priority_queue(pq, 5);

  print_priority_queue(pq);

  enqueue_priority_queue(pq, 5, 50);

  print_priority_queue(pq);

  dequeue_priority_queue(pq, 3);

  print_priority_queue(pq);

  fee_priority_queue(pq);
  return 0;
}
