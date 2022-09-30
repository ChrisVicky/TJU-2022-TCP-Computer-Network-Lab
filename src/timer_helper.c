#include "../inc/timer_helper.h"

void push_timer(timer_node* node, timer_list* list){
  list->size ++;
  node->id = list->total ++;
  node->next = NULL;
  if(list->head == NULL){
    list->head = node;
  }else{
    list->tail->next = node;
    list->tail = list->tail->next;
  }
}

uint32_t set_timer_without_mutex(struct timer_list *list,
                                 uint32_t sec,
                                 uint64_t nano_sec,
                                 void *(*callback)(void *),
                                 void *args) {

  struct timespec *timeout = malloc(sizeof(struct timespec));
  struct timespec *now = malloc(sizeof(struct timespec));
  clock_gettime(CLOCK_REALTIME, timeout);
  clock_gettime(CLOCK_REALTIME, now);
  struct timer_node *node = malloc(sizeof(struct timer_node));
  memset(node, 0, sizeof(struct timer_node));
  timeout->tv_sec += sec;
  timeout->tv_nsec += nano_sec;
  node->event->timeout_at = timeout;
  node->event->create_at = now;
  node->event->callback = callback;
  node->event->args = args;
  push_timer(node, list);
  return node->id;
}

uint32_t set_timer(struct timer_list *list, uint32_t sec, uint64_t nano_sec, void *(*callback)(void *), void *args){
  pthread_mutex_unlock(list->lock);
  uint32_t ret = set_timer_without_mutex(list, sec, nano_sec, callback, args);
  pthread_mutex_lock(list->lock);
  return ret;
}


void free_timer_node(timer_node* node){
  free(node->event->timeout_at);
  free(node->event->create_at);
  free(node->event);
  free(node);
  node = NULL;
}



struct timer_list* init_timer_list(){
  timer_list* ret = (timer_list*)malloc(sizeof(timer_list));
  ret->head = NULL;
  ret->tail = ret->head;
  ret->size = 0;
  ret->total = 1;
  pthread_mutex_init(ret->lock, NULL);
  return ret;
}

int check_timer(struct timer_list *list){
  pthread_mutex_lock(list->lock);
  struct timer_node *iter = list->head;
  struct timer_node *prev = NULL;
  DEBUG("Iter through all Timer in the list\n");
  while(iter!=NULL && iter!=list->tail){
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    uint64_t current_time = TIMESPEC2NANO(now);
    uint64_t timeout = TIMESPEC2NANO((*(iter->event->timeout_at)));
    if (timeout <= current_time) {
      DEBUG("Currenttime: %lu, timeout: %lu\n", current_time, timeout);
      if (prev == NULL) {
        list->head = iter->next;
      } else {
        prev->next = iter->next;
      }
      if (iter == list->tail) {
        list->tail = prev;
      }
      if (list->head == NULL) {
        list->tail = NULL;
      }
      struct timer_node *tmp = iter;
      DEBUG("timer %d timeout\n", tmp->id);
      void *result = tmp->event->callback(tmp->event->args);
      free_timer_node(tmp);
      list->size--;
      if(prev!=NULL) iter = prev->next;
      else iter = list->head;
    }else{
      prev = iter;
      iter = iter->next;
    }
  }
  pthread_mutex_unlock(list->lock);
  return 0;
}


/**
* @brief Get the first Timer's Timeout. (More like a queue)
*
* @param list
*
* @return 
*/
uint32_t get_recent_timeout(struct timer_list *list) {
  pthread_mutex_lock(list->lock);
  if (list->head == NULL) {
    pthread_mutex_unlock(list->lock);
    return 0;
  }

  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  uint32_t current_time = TIMESPEC2NANO(now);
  uint32_t timeout = TIMESPEC2NANO((*(list->head->event->timeout_at))) - current_time;

  pthread_mutex_unlock(list->lock);
  return timeout;
}

int destroy_timer(struct timer_list *list, uint32_t id, int destroy, void (*des)(void *)) {
  DEBUG("destroy timer %d\n", id);
  pthread_mutex_lock(list->lock);
  struct timer_node *iter= list->head;
  struct timer_node *prev = NULL;
  while (iter != NULL) {
    if (iter->id == id) {
      if (prev == NULL) {
        list->head = iter->next;
      } else {
        prev->next = iter->next;
      }
      if (iter == list->tail) {
        list->tail = prev;
      }
      if (destroy) {
        des(&iter->event);
      }
      free_timer_node(iter);
      list->size--;
      DEBUG("timer %d canceled\n", id);
      pthread_mutex_unlock(list->lock);
      return 0;
    }
    prev = iter;
    iter = iter->next;
  }
  pthread_mutex_unlock(list->lock);

  return -1;
}
