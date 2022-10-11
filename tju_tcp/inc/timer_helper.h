#ifndef __LIST_H__
#define __LIST_H__
//  定义一个 LIST 类型（链表实现）
#include "global.h"
#include "debug.h"
#include "queue.h"

//TODO: Not yet Finish IMPLEMENTATION
#define SEC2NANO(x) (uint64_t)(x * 1000000000)
#define NANO2SEC(x) (float)((float)((x)/1000000000.0))
#define NANO2TIMESPEC(nano) (struct timespec){.tv_sec = (time_t)(nano / 1000000000), .tv_nsec = (long)(nano % 1000000000)}
#define TIMESPEC2NANO(timespec) (uint64_t)(timespec.tv_sec * 1000000000 + timespec.tv_nsec)

typedef struct timer_event{
  struct timespec * create_at;
  struct timespec * timeout_at;
  tju_packet_t * args;                // 调用函数传递的参数
  void * (*callback)(void *, void *); // 调用的函数
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
  myQueue *queue;
}timer_list;

struct timer_list* init_timer_list();
uint32_t set_timer(struct timer_list *list, uint32_t sec, uint64_t nano_sec, void *(*callback)(void *, void*), void *args);
int check_timer(tju_tcp_t * sock);
uint32_t set_timer_without_mutex(struct timer_list *list,uint32_t sec,uint64_t nano_sec,void *(*callback)(void *, void*),void *args);
int destroy_timer(tju_tcp_t* sock, uint32_t id);
int get_list_size(struct timer_list*);
void print_timers(timer_list* list);
uint64_t get_create_time(timer_list* list, int id);
#endif // !__LIST_H__
