#include "../inc/timer_helper.h"

void print_timers(timer_list* list){
  timer_node* iter = list->head;
  _line_("total Size: %d" ,list->size);
  while(iter!=NULL){
    _print_("%d(%d) -> ",iter->event->args->header.seq_num,iter->id);
    iter = iter->next;
  }
  _print_("\n");
}
void push_timer(timer_node* node, timer_list* list){
  list->size ++;
  node->id = list->total ++;
  node->next = NULL;
  if(list->head == NULL){
    // list->head = malloc(sizeof(timer_node));
    list->head = node;
    list->tail = node;
  }else{
    list->tail->next = node;
    list->tail = list->tail->next;
  }
}

uint32_t set_timer_without_mutex(struct timer_list *list, uint32_t sec, uint64_t nano_sec, void *(*callback)(void *, void* ), void *args) {
  struct timespec *timeout = malloc(sizeof(struct timespec));
  struct timespec *now = malloc(sizeof(struct timespec));
  struct timer_node *node = malloc(sizeof(struct timer_node));
  memset(node, 0, sizeof(struct timer_node));
  node->event = malloc(sizeof(timer_event));
  clock_gettime(CLOCK_REALTIME, timeout);
  clock_gettime(CLOCK_REALTIME, now);
  timeout->tv_sec += sec;
  timeout->tv_nsec += nano_sec;
  node->event->timeout_at = timeout;
  node->event->create_at = now;
  node->event->callback = callback;
  node->event->args = args;
  push_timer(node, list);
  print_timers(list);
  return node->id;
}

uint32_t set_timer(struct timer_list *list, uint32_t sec, uint64_t nano_sec, void *(*callback)(void *, void *), void *args){
  _debug_("pthread_lock\n");
  pthread_mutex_lock(&list->lock);
  uint32_t ret = set_timer_without_mutex(list, sec, nano_sec, callback, args);
  pthread_mutex_unlock(&list->lock);
  return ret;
}


void free_timer_node(timer_node* node){
  _debug_("free\n");
  free(node->event->timeout_at);
  node->event->timeout_at = NULL;
  free(node->event->create_at);
  node->event->create_at = NULL;
  free(node->event);
  node->event = NULL;
  free(node);
  node = NULL;
}



struct timer_list* init_timer_list(){
  timer_list* ret = (timer_list*)malloc(sizeof(timer_list));
  ret->head = NULL;
  ret->tail = ret->head;
  ret->size = 0;
  ret->total = 1;
  pthread_mutex_init(&ret->lock, NULL);
  return ret;
}

int check_timer(tju_tcp_t *sock){
  timer_list* list = sock->timers;
  while(pthread_mutex_lock(&list->lock)!=0);
  struct timer_node *iter = list->head;
  struct timer_node *prev = NULL;
  // _debug_("Iter through all Timer in the list\n");
  // while(iter!=NULL && iter!=list->tail){ // Iteration through all 
  if(iter!=NULL){ // 能过
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    uint64_t current_time = TIMESPEC2NANO(now);
    uint64_t timeout = TIMESPEC2NANO((*(iter->event->timeout_at)));
    uint64_t time_create = TIMESPEC2NANO((*(iter->event->create_at)));
    _debug_("Current: %ld vs Timeout: %ld\n" ,current_time, timeout);
    print_timers(sock->timers);
    // _debug_("Current Head id: %d\n" ,iter->event->args->header.seq_num);
    if (timeout <= current_time) {
      _debug_("%ld + %ld <= %ld(timeout) %ld\n",time_create,timeout-time_create, current_time, current_time - timeout);
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
      list->size--;
      struct timer_node *tmp = iter;
      _debug_("timer %d timeout\n", tmp->id);
      tmp->event->callback(tmp->event->args, sock);
      free_timer_node(tmp);
      if(prev!=NULL) iter = prev->next;
      else iter = list->head;
    }else{
      prev = iter;
      iter = iter->next;
    }
  }
  pthread_mutex_unlock(&list->lock);
  return 0;
}


/**
* @brief Get the first Timer's Timeout. (More like a queue)
*
* @param list
*
* @return 
*/
uint64_t get_recent_timeout(struct timer_list *list) {
  // Get the top Timeout
  if (list->head == NULL) {
    return 0;
  }
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  uint64_t current_time = TIMESPEC2NANO(now);
  uint64_t timeout =TIMESPEC2NANO((*(list->head->event->timeout_at)));
  if(timeout < current_time){
    _debug_("%d recent_timeout: %d\n" ,list->head->event->args->header.seq_num,2);
    return 0;
  }else{
    _debug_("%d recent_timeout: %d\n" ,list->head->event->args->header.seq_num,1);
    return timeout - current_time;
  }
}

int destroy_timer(tju_tcp_t* sock, uint32_t id, void(*free_retrans_arg)(void *, void*)) {
  while(pthread_mutex_lock(&sock->timers->lock)!=0);
  struct timer_list *list = sock->timers;
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
      _debug_("start destroying\n");
      free_retrans_arg(iter->event, sock);
      free_timer_node(iter);
      list->size--;
      print_timers(sock->timers);
      _debug_("size: %d\n" ,list->size);
      pthread_mutex_unlock(&sock->timers->lock);
      return 0;
    }
    prev = iter;
    iter = iter->next;
  }
  pthread_mutex_unlock(&sock->timers->lock);
  return -1;
}

int get_list_size(struct timer_list* list){
  int ret = list->size;
  return ret;
}
