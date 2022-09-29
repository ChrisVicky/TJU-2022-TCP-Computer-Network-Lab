
#ifndef __TIME_HELPER_H__
#define __TIME_HELPER_H__
#include "global.h"

//TODO: Not yet Finish IMPLEMENTATION
#define SEC2NANO(x) (uint64_t)(x * 1000000000)
#define TO_TIMESPEC(nano) (struct timespec){.tv_sec = (time_t)(nano / 1000000000), .tv_nsec = (long)(nano % 1000000000)}
#define TO_NANO(timespec) (uint64_t)(timespec.tv_sec * 1000000000 + timespec.tv_nsec)

typedef struct timer_event {
  struct timespec *create_time;
  struct timespec *timeout;
  void *(*callback)(void *);
  void *args;
} timer_event;

typedef struct time_node {
  struct timer_event event;
  struct time_node *next;
  uint32_t id;
} time_node;

typedef struct time_list {
  struct time_node *head;
  struct time_node *tail;
  int size;
  uint32_t id_pool;
  pthread_mutex_t lock;
} time_list;

struct time_list *time_list_init();
uint32_t get_recent_timeout(struct time_list *list);
uint32_t set_timer(struct time_list *list, uint32_t sec, uint64_t nano_sec, void *(*callback)(void *), void *args);
void *check_timer(struct time_list *list);
uint32_t set_timer_without_mutex(struct time_list *list,
                                 uint32_t sec,
                                 uint64_t nano_sec,
                                 void *(*callback)(void *),
                                 void *args);
int cancel_timer(struct time_list *list, uint32_t id, int destroy, void (*des)(void *));
void hit_node(struct time_list *list, uint32_t id);
#endif //__TIME_HELPER_H__
