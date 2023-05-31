#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

typedef struct QueueNodeStruct{
  Pair p;
  struct QueueNodeStruct *n;
} QueueNode;

typedef struct {
  uint32_t size;
  QueueNode* h;
  QueueNode* t;
} Queue;

QueueNode* create_queue_node(int32_t idx, int32_t jumps, uint32_t buff_size) {
  QueueNode* node = malloc(sizeof(QueueNode));
  node->n = 0;
  node->p.idx = idx;
  node->p.jumps = jumps;
  node->p.buff_idx = 0;
  node->p.buff = malloc(sizeof(uint32_t) * buff_size);
  if (!node->p.buff) {
    printf("Malloc error\n");
    exit(1);
  }
  return node;
}

Queue* create_queue() {
  Queue* queue = malloc(sizeof(Queue));
  queue->size = 0;
  queue->h = 0;
  queue->t = 0;
  return queue;
}

void add_queue_node(Queue* queue, int32_t idx, int32_t jumps, uint32_t buff_size) {
  if (!queue) {
    return;
  }

  printf("before add head %p tail %p\n", queue->h, queue->t);

  QueueNode* node = create_queue_node(idx, jumps, buff_size);
  if (!queue->h) {
    printf("found null head\n");
    queue->h = node;
  }
  if (!queue->t) {
    queue->t = node;
  } else {
    queue->t->n = node;
    queue->t = node;
  }
  queue->size++;

  printf("after add head %p tail %p\n", queue->h, queue->t);
}

Pair queue_front(Queue* queue) {
  if (!queue || !queue->h) {
    Pair p = {INT_MAX, INT_MAX};
    return p;
  }
  return queue->h->p;
}

void queue_pop(Queue* queue) {
  if (!queue->h) {
    return;
  }
  QueueNode* tmp = queue->h;
  queue->h = queue->h->n;
  free(tmp->p.buff);
  free(tmp);
  queue->size--;
}

void print_queue(Queue* queue) {
  QueueNode* tmp = queue->h;

  if (queue->size < 4) {
    return;
  }
  printf("1: %d\n", queue->h->p.buff_idx);
  printf("2: %d\n", queue->h->n->p.buff_idx);
  printf("3: %d\n", queue->h->n->n->n->p.buff_idx);
  printf("is 4 null %d\n", queue->h->n->n->n->n == 0);
  // printf("4: %d\n", queue->h->n->n->n->n->p.buff_idx);

  while (tmp) {
    printf("tmp address %p", tmp);
    printf("q inner buf size: %d\n", tmp->p.buff_idx);
    printf("queue inner buf elements:\n");
    for (uint32_t i=0; i<tmp->p.buff_idx; ++i) {
      printf("%d ", tmp->p.buff[i]);
    }
    printf("\n");
    
    tmp = tmp->n;
  }
  printf("end queue print\n");
}

void free_queue(Queue* queue) {
  if (!queue) {
    return;
  }
  QueueNode* tmp = queue->h;
  while (tmp) {
    tmp = tmp->n;
    free(queue->h->p.buff);
    free(queue->h);
    queue->h = tmp;
  }
  free(queue);
}

int main() {
  return 0;
}
