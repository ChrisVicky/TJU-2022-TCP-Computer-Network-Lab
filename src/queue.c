#include "../inc/queue.h"

myQueue* init_q(){
  myQueue* q = (myQueue*)malloc(sizeof(struct myQueue));
  if(q==NULL) return NULL;
  q->head = NULL;
  q->tail = NULL;
  q->size = 0;
  return q;
}

int push_q(myQueue* q, void *data){
  // _debug_("push_q start\n");
  struct myNode * new_node = (myNode*)malloc(sizeof(struct myNode));

  if(new_node == NULL) return q->size;

  new_node->data = data;
  new_node->next = NULL;

  if(q->size == 0){
    // size == 0 
    q->head = new_node;
    q->tail = new_node;
    q->size = 1;
  }else{
    q->tail->next = new_node;
    q->tail = new_node;
    q->size ++;
  }
  // _debug_("push_q end\n");
  return q->size;
}

void *pop_q(myQueue* q){
  if(q->size==0) return NULL;
  q->size -- ;
  void *data = q->head->data;
  myNode* to_free = q->head;
  q->head = q->head->next;
  free(to_free);
  return data;
}

int size_q(myQueue* q){
  return q->size;
}

int is_empty_q(myQueue *q){
  return q->size == 0;
}
