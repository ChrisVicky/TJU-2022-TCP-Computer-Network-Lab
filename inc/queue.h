#ifndef __QUEUE_H__ 
#define __QUEUE_H__
#include "tju_tcp.h"
#include <stdio.h>

typedef struct myNode{
  void *data;
  struct myNode *next;
}myNode;

typedef struct myQueue{
  int size;
  struct myNode *head;
  struct myNode *tail;
}myQueue;

myQueue* init_q();

int push_q(myQueue* q, void *data);

void *pop_q(myQueue* q);

#endif // !__QUEUE_H__
