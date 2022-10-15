#include "../inc/timer_helper.h"

extern void free_retrans_arg(timer_event *ptr, tju_tcp_t *sock);

extern void update_rtt(double rtt, tju_tcp_t *tju_tcp);

extern uint32_t get_ack_id_hash(int id);

extern void set_ack_id_hash(int id, uint32_t value);

extern void congestion_handler(tju_tcp_t* sock);

uint64_t get_create_time(timer_list* list, int id) {
  timer_node* ptr = list->head;
  while(ptr!=NULL){
    if(ptr->id==id){
      return TIMESPEC2NANO((*ptr->event->create_at));
    }
    ptr = ptr->next;
  }
  return 0;
}

void print_timers(timer_list* list){
  timer_node* iter = list->head;
  _line_("total Size: %d, current: %ld" ,list->size, get_current_time());
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  uint64_t cur = TIMESPEC2NANO(now);
  while(iter!=NULL){
    struct timespec a = *(iter->event->timeout_at);
    uint64_t timeout = TIMESPEC2NANO(a);
    if(timeout > cur){
      _print_("%d(%d: %ld) -> ",iter->event->args->header.seq_num,iter->id,TIMESPEC2NANO(a)-cur);
    }else{
      _print_("%d(%d: -%ld) -> ",iter->event->args->header.seq_num,iter->id,cur - timeout);
    }
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

/**
* @brief Insert at front (In seq order)
*
* @param node
* @param list
*/
void insert_timer(timer_node* node, timer_list* list){
  list->size ++;
  node->id = list->total ++;
  node->next = NULL;
  if(list->head == NULL){
    // list->head = malloc(sizeof(timer_node));
    list->head = node;
    list->tail = node;
  }else{
    timer_node* iter = list->head;
    timer_node* pre = NULL;
    while(iter!=NULL){
      if(iter->event->args->header.seq_num > node->event->args->header.seq_num){
        if(pre==NULL){
          node->next = iter;
          list->head = node;
        }else{
          pre->next = node;
          node->next = iter;
        }
        return;
      }
      pre = iter;
      iter = iter->next;

    }
    list->tail->next = node;
    list->tail = list->tail->next;
  }
}
/**
* @brief Insert at the front ==> Used for first timeout
*
* @param list
* @param sec
* @param nano_sec
* @param callback
* @param args
*
* @return 
*/
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
  insert_timer(node, list);
  return node->id;

}


/**
* @brief Used for first send --> Push to the last
*
* @param list
* @param sec
* @param nano_sec
* @param callback
* @param args
*
  * @return 
*/
uint32_t set_timer(struct timer_list *list, uint32_t sec, uint64_t nano_sec, void *(*callback)(void *, void *), void *args){
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
  _debug_("Set timer :%d, timeout: %f\n",node->id,NANO2SEC(nano_sec));
  return node->id;
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
  ret->queue = init_q();
  pthread_mutex_init(&ret->lock, NULL);
  return ret;
}

int check_timer(tju_tcp_t *sock){
  timer_list* list = sock->timers;
  while(pthread_mutex_lock(&list->queue->q_lock)!=0);
  while(!is_empty_q(list->queue)){
    pair* p = pop_q(list->queue);
    _debug_("p->id: %d\n" ,p->id);
    int id = get_ack_id_hash(p->id);
    _debug_("p->id: %d, id: %d\n" ,p->id, id);
    if(p->timeout!=0){
      struct timespec now;
      clock_gettime(CLOCK_REALTIME, &now);
      uint64_t current_time = TIMESPEC2NANO(now);
      uint64_t create_at = get_create_time(sock->timers, id);
      if(create_at != 0){
        update_rtt(NANO2SEC(current_time-create_at), sock);
      }
    }
    destroy_timer(sock, id);
    print_timers(list);
    set_ack_id_hash(p->id, 0);
    free(p); p = NULL;
  }
  pthread_mutex_unlock(&list->queue->q_lock);

  struct timer_node *iter = list->head;
  struct timer_node *prev = NULL;
  int time_out_flag = 0;
  while(iter!=NULL){ // 能过
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    uint64_t current_time = TIMESPEC2NANO(now);
    uint64_t timeout = TIMESPEC2NANO((*(iter->event->timeout_at)));
    uint64_t time_create = TIMESPEC2NANO((*(iter->event->create_at)));
    if (timeout <= current_time) {
      time_out_flag = 1;
      print_timers(list);
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
      iter = iter->next;
      _debug_("timer %d timeout\n", tmp->id);
      tmp->event->callback(tmp->event->args, sock);
      free_timer_node(tmp);
    }else{
      break;
    }
  }
  // CHECK TIME_OUT for CONGESTION WINDOW
  if(time_out_flag && sock->state==ESTABLISHED){
    while(pthread_mutex_lock(&sock->window.wnd_send->con->lock)!=0);
    sock->window.wnd_send->con->con_state = TIME_OUT;
    congestion_handler(sock);
    pthread_mutex_unlock(&sock->window.wnd_send->con->lock);
  }
  return 0;
}

int destroy_timer(tju_tcp_t* sock, uint32_t id) {
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
      _debug_("DestroyIng timer: %d, seq: %d\n",id,iter->event->args->header.seq_num);
      if(iter->event->args->header.flags & FIN_FLAG_MASK){
        _debug_("flags FIN\n");
        if(sock->state == FIN_WAIT_1){
          sock->state = FIN_WAIT_2;
          _debug_line_("Sock State: %d" ,FIN_WAIT_2);
        }else if(sock->state == LAST_ACK){
          sock->state = CLOSED;
          _debug_line_("Sock State: %d" ,CLOSED);
        }else{
          _debug_line_("!! NOT HIT sock state: %d" ,sock->state);
        }

      }
      free_retrans_arg(iter->event, sock);
      free_timer_node(iter);
      list->size--;
      return 0;
    }
    prev = iter;
    iter = iter->next;
  }
  return -1;
}

int get_list_size(struct timer_list* list){
  int ret = list->size;
  return ret;
}
