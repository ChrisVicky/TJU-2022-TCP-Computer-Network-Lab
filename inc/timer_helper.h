#ifndef __LIST_H__
#define __LIST_H__
//  定义一个 LIST 类型（链表实现）
#include "global.h"
#include "debug.h"

//TODO: Not yet Finish IMPLEMENTATION
#define SEC2NANO(x) (uint64_t)(x * 1000000000)
#define NANO2TIMESPEC(nano) (struct timespec){.tv_sec = (time_t)(nano / 1000000000), .tv_nsec = (long)(nano % 1000000000)}
#define TIMESPEC2NANO(timespec) (uint64_t)(timespec.tv_sec * 1000000000 + timespec.tv_nsec)

typedef struct timer_event{
  struct timespec * create_at;
  struct timespec * timeout_at;
  void * args;                // 调用函数传递的参数
  void * (*callback)(void *); // 调用的函数
}timer_event;

typedef struct timer_node{
  struct timer_event *event;
  struct timer_node *next;
  int id;
}timer_node;

typedef struct timer_list{
  timer_node* head;
  timer_node* tail;
  int size;
  int total;
  pthread_mutex_t lock;
}timer_list;

struct timer_list* init_timer_list();
uint32_t get_recent_timeout(struct timer_list *list);
uint32_t set_timer(struct timer_list *list, uint32_t sec, uint64_t nano_sec, void *(*callback)(void *), void *args);
int check_timer(struct timer_list *list);
uint32_t set_timer_without_mutex(struct timer_list *list,uint32_t sec,uint64_t nano_sec,void *(*callback)(void *),void *args);
int destroy_timer(struct timer_list *list, uint32_t id, int destroy, void (*des)(void *));

#endif // !__LIST_H__
