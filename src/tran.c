#include "../inc/tran.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

uint32_t ack_id_hash[100000000];

pthread_cond_t packet_available = PTHREAD_COND_INITIALIZER;
pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;

timer_list * timers = NULL;

void init_retransmit_timer() {
  timers = init_timer_list();
  memset(ack_id_hash, 0, sizeof(ack_id_hash));
  start_work_thread(timers);
}

void update_rtt(double rtt, tju_tcp_t *tju_tcp) {
  double srtt = tju_tcp->window.wnd_send->estmated_rtt;
  tju_tcp->window.wnd_send->estmated_rtt = (RTT_ALPHA * srtt) + (1 - RTT_ALPHA) * rtt;
  tju_tcp->window.wnd_send->rto = min(RTT_UBOUND, max(RTT_LBOUND, RTT_BETA * srtt));
}

void *retransmit(transmit_arg_t *args) {
  tju_tcp_t *tju_tcp = (tju_tcp_t *) args->sock;
  tju_packet_t *pkt = args->pkt;
  uint32_t id = set_timer_without_mutex(timers,0,SEC2NANO(tju_tcp->window.wnd_send->rto),(void *(*)(void *)) retransmit,args);
  uint16_t dlen = pkt->header.plen - pkt->header.hlen;
  ack_id_hash[pkt->header.seq_num + dlen + 1] = id;
  _debug_("transmit : set timer %d expecting ack: %d, timeout at %f\n",id,pkt->header.seq_num + dlen + 1,tju_tcp->window.wnd_send->rto);
  safe_packet_sender(pkt);
  return NULL;
}

uint32_t send_with_retransmit(tju_tcp_t *sock, tju_packet_t *pkt, int requiring_ack){
  if (requiring_ack) {
    uint32_t id = 0;
    transmit_arg_t *args = malloc(sizeof(transmit_arg_t));
    args->pkt = pkt;
    args->sock = sock;
    id = set_timer(timers, 0, SEC2NANO(sock->window.wnd_send->rto), (void *(*)(void *)) retransmit, args);
    uint16_t dlen = pkt->header.plen - pkt->header.hlen;
    ack_id_hash[pkt->header.seq_num + dlen + 1] = id;
    _debug_("set timer %d expecting ack: %d, timeout at %f\n", id, pkt->header.seq_num + dlen + 1, sock->window.wnd_send->rto);
  }
  safe_packet_sender(pkt);
  _debug_("Sending Packet: ack:%d, seq:%d\n", pkt->header.ack_num, pkt->header.seq_num);
  return 0;
}

void free_retrans_arg(void *arg) {
  _debug_("FREE 1\n");
  timer_event *ptr = (timer_event *) arg;
  _debug_("FREE 2\n");
  struct timespec now;
  _debug_("FREE 3\n");
  clock_gettime(CLOCK_REALTIME, &now);
  _debug_("FREE 4\n");
  uint64_t current_time = TIMESPEC2NANO(now);
  _debug_("FREE 5\n");
  uint64_t create_time = TIMESPEC2NANO((*ptr->create_at));
  _debug_("FREE 6\n");
  //WARN: ptr->args may be NULL (Debug is needed)
  // update_rtt((current_time - create_time) / 1000000000.0, ((transmit_arg_t *) ptr->args)->sock);
  _debug_("FREE 7\n");
  // Free Error
  // free(((transmit_arg_t *) ptr->args)->pkt);
  _debug_("FREE 8\n");
}

void received_ack(uint32_t ack, tju_tcp_t *sock) {
  // TODO: an ack should remove all the packets with seq_num+length < ack
  _debug_("ack received: %d\n", ack);
  if (ack_id_hash[ack] != 0) {
    uint32_t tmp = sock->window.wnd_send->base;
    while (ack_id_hash[++tmp] == 0);
    if (tmp == ack) {
      sock->window.wnd_send->base = ack;
    }
    destroy_timer(timers, ack_id_hash[ack], 1, free_retrans_arg);
    ack_id_hash[ack] = 0;
  }
}

void *transit_work_thread(timer_list *list) {
  struct timespec spec;
  spec = NANO2TIMESPEC(get_recent_timeout(list));
  while(TRUE){
    pthread_mutex_lock(&cond_mutex);
    pthread_cond_timedwait(&packet_available, &cond_mutex, &spec);
    pthread_mutex_unlock(&cond_mutex);
    check_timer(list);
    spec = NANO2TIMESPEC(get_recent_timeout(list));
  }
}

pthread_t start_work_thread(timer_list* list) {
  pthread_t work_thread;
  pthread_create(&work_thread, NULL, (void *(*)(void *)) transit_work_thread, list);
  return work_thread;
}
