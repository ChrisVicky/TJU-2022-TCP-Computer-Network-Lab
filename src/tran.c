#include "../inc/tran.h"
#include <arpa/inet.h>

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))
#define abs_minus(x,y) ((x)<(y)?((y)-(x)):((x)-(y)))

uint32_t ack_id_hash[100000000];
sender_window_t global_sender;

pthread_cond_t packet_available = PTHREAD_COND_INITIALIZER;
pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;

timer_list * timers = NULL;

void init_retransmit_timer() {
  global_sender.estmated_rtt = INIT_RTT;
  global_sender.dev_rtt = 0;
  global_sender.rto = INIT_RTT;
  timers = init_timer_list();
  memset(ack_id_hash, 0, sizeof(ack_id_hash));
  start_work_thread(timers);
}

void update_rtt(double rtt, tju_tcp_t *tju_tcp) {
  _debug_("start to update rtt :\n");
  _ip_port_(tju_tcp->bind_addr);
  double ertt = global_sender.estmated_rtt;
  double srtt = rtt; 
  double dev = global_sender.dev_rtt;
  global_sender.estmated_rtt = (RTT_ALPHA * srtt) + (1 - RTT_ALPHA) * ertt;
  global_sender.dev_rtt = RTT_BETA * dev + (1-RTT_BETA) * abs_minus(srtt, ertt);
  global_sender.rto = min(RTT_UBOUND, max(RTT_LBOUND, global_sender.estmated_rtt + 4 * global_sender.dev_rtt));
// "[SampleRTT:%f EstimatedRTT:%f DeviationRTT:%f TimeoutInterval:%f]\n"
  trace_rtts(srtt, global_sender.estmated_rtt, 
             global_sender.dev_rtt ,global_sender.rto);
}

void *retransmit(transmit_arg_t *args) {
  // TODO: Check if there need another timer set
  tju_tcp_t *tju_tcp = (tju_tcp_t *) args->sock;
  tju_packet_t *pkt = args->pkt;
  tju_tcp->window.wnd_send->rto = global_sender.rto;
  tju_tcp->window.wnd_send->estmated_rtt = global_sender.estmated_rtt;
  tju_tcp->window.wnd_send->dev_rtt = global_sender.dev_rtt;
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
    sock->window.wnd_send->dev_rtt = global_sender.dev_rtt;
    sock->window.wnd_send->estmated_rtt = global_sender.estmated_rtt;
    sock->window.wnd_send->rto = global_sender.rto;
    args->pkt = pkt;
    args->sock = sock;
    id = set_timer(timers, 0, SEC2NANO(sock->window.wnd_send->rto), (void *(*)(void *)) retransmit, args);
    uint16_t dlen = pkt->header.plen - pkt->header.hlen;
    ack_id_hash[pkt->header.seq_num + dlen + 1] = id;
    // printf("set timer %d expecting ack: %d, timeout at %f\n", id, pkt->header.seq_num + dlen + 1, sock->window.wnd_send->rto);
  }
  trace_send(pkt->header.seq_num, pkt->header.ack_num, pkt->header.flags);
  safe_packet_sender(pkt);
  // _debug_("Sending Packet: ack:%d, seq:%d\n", pkt->header.ack_num, pkt->header.seq_num);
  return 0;
}

void free_retrans_arg(void *arg) {
  timer_event *ptr = (timer_event *) arg;
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  uint64_t current_time = TIMESPEC2NANO(now);
  uint64_t create_time = TIMESPEC2NANO((*ptr->create_at));
  update_rtt((current_time - create_time) / 1000000000.0, ((transmit_arg_t *) ptr->args)->sock);
  free(((transmit_arg_t *) ptr->args)->pkt);
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

