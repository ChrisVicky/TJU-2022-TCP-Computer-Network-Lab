#ifndef __LIST_H__
#define __LIST_H__
//  定义一个 LIST 类型（链表实现）
#include "global.h"
typedef struct timer_event{
  struct timespec * create_at;
  struct timespec * timeout_at;
  void * args;                // 调用函数传递的参数
  void * (*callback)(void *); // 调用的函数
}timer_event;

typedef struct timer_node{
  struct timer_event * event;
  struct timer_node *next;
  int id;
}timer_node;

typedef struct timer_list{
  timer_node* head;
  timer_node* tail;
  int size;
  int total;
  pthread_mutex_t* lock;
}timer_list;

struct timer_list* init_timer_list();
uint32_t get_recent_timeout(struct time_list *list);
uint32_t set_timer(struct time_list *list, uint32_t sec, uint64_t nano_sec, void *(*callback)(void *), void *args);
void *check_timer(struct time_list *list);
uint32_t set_timer_without_mutex(struct time_list *list,uint32_t sec,uint64_t nano_sec,void *(*callback)(void *),void *args);
int cancel_timer(struct time_list *list, uint32_t id, int destroy, void (*des)(void *));

#endif // !__LIST_H__
