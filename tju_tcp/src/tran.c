#include "../inc/tran.h"
#include <arpa/inet.h>

#define abs_minus(x,y) ((x)<(y)?((y)-(x)):((x)-(y)))

uint32_t ack_id_hash[100000000];
static uint32_t last_ack;

pthread_cond_t packet_available = PTHREAD_COND_INITIALIZER;
pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;


timer_list* init_retransmit_timer() {
  timer_list* timers = init_timer_list();
  memset(ack_id_hash, 0, sizeof(ack_id_hash));
  return timers;
}

void update_rtt(double rtt, tju_tcp_t *tju_tcp) {
  _debug_("Start Update rtt\n");
  double ertt = tju_tcp->window.wnd_send->estmated_rtt;
  double srtt = rtt; 
  double dev = tju_tcp->window.wnd_send->dev_rtt;
  tju_tcp->window.wnd_send->estmated_rtt = (RTT_ALPHA * srtt) + (1 - RTT_ALPHA) * ertt;
  tju_tcp->window.wnd_send->dev_rtt = RTT_BETA * dev + (1-RTT_BETA) * abs_minus(srtt, ertt);
  double candidate = tju_tcp->window.wnd_send->estmated_rtt + 4*tju_tcp->window.wnd_send->dev_rtt;
  tju_tcp->window.wnd_send->rto = min(RTT_UBOUND, max(RTT_LBOUND, candidate));
  // "[SampleRTT:%f EstimatedRTT:%f DeviationRTT:%f TimeoutInterval:%f]\n"
  trace_rtts(srtt, tju_tcp->window.wnd_send->estmated_rtt, 
             tju_tcp->window.wnd_send->dev_rtt ,tju_tcp->window.wnd_send->rto);
  _debug_("update rtt: min(%d,max(%f,%f))=%f\n",RTT_UBOUND, RTT_LBOUND, candidate, tju_tcp->window.wnd_send->rto);
}

void *retransmit(tju_packet_t* pkt, tju_tcp_t* tju_tcp) {
  _debug_("1 seq: %d, transmit : timeout at %f\n",pkt->header.seq_num,tju_tcp->window.wnd_send->rto);
  uint32_t id = set_timer_without_mutex(tju_tcp->timers,0,SEC2NANO(tju_tcp->window.wnd_send->rto),(void *(*)(void *, void*)) retransmit, pkt);
  uint16_t dlen = pkt->header.plen - pkt->header.hlen;
  ack_id_hash[pkt->header.seq_num + dlen] = id;
  _debug_("2 transmit : set timer %d seq: %d, expecting ack: %d, timeout at %f\n",id,pkt->header.seq_num,pkt->header.seq_num + dlen + 1,tju_tcp->window.wnd_send->rto);
  safe_packet_sender(pkt);
  return NULL;
}

uint32_t send_with_retransmit(tju_tcp_t *sock, tju_packet_t *pkt, int requiring_ack){
  if (requiring_ack) {
    uint32_t id = 0;
    id = set_timer(sock->timers, 0, SEC2NANO(sock->window.wnd_send->rto), (void *(*)(void *, void*)) retransmit, pkt);
    uint16_t dlen = pkt->header.plen - pkt->header.hlen;
    ack_id_hash[pkt->header.seq_num + dlen] = id;
    _debug_("set timer %d expecting ack: %d, timeout at %f\n", id, pkt->header.seq_num + dlen + 1, sock->window.wnd_send->rto);
  }
  // trace_send(pkt->header.seq_num, pkt->header.ack_num, pkt->header.flags);
  safe_packet_sender(pkt);
  return 0;
}

void free_retrans_arg(timer_event* ptr, tju_tcp_t* sock) {
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  uint64_t current_time = TIMESPEC2NANO(now);
  uint64_t create_time = TIMESPEC2NANO((*ptr->create_at));
  _debug_("update RTTTTTT\n");
  update_rtt((current_time - create_time) / 1000000000.0, sock);
  _debug_("Finish Update\n");
  free(ptr->args);
}

/**
* @brief resovlve ack => destroy corresponding timer
*
* @param ack
* @param sock
*
* @return  1: first time to receive ack for this
*          0: not first time
*/
int received_ack(uint32_t ack, tju_tcp_t *sock) {

  for(last_ack;last_ack<=ack;last_ack++){
  if (ack_id_hash[last_ack] != 0) {
    _debug_("Start Destroying timer: %d\n" ,ack_id_hash[last_ack]);
    uint32_t tmp = sock->window.wnd_send->base;
    destroy_timer(sock, ack_id_hash[last_ack], 1, (void(*)(void*, void*))free_retrans_arg);
    ack_id_hash[last_ack] = 0;
    return 1;
  }
  }
  return 0;
}

void *transit_work_thread(tju_tcp_t* sock) {
  struct timespec spec;
  spec = NANO2TIMESPEC(get_recent_timeout(sock->timers));
  while(TRUE){
    pthread_mutex_lock(&cond_mutex);
    pthread_cond_timedwait(&packet_available, &cond_mutex, &spec); // Wait till the next timeout
    pthread_mutex_unlock(&cond_mutex);
    check_timer(sock);
    spec = NANO2TIMESPEC(get_recent_timeout(sock->timers));
  }
}

pthread_t start_work_thread(tju_tcp_t *sock) {
  pthread_t work_thread;
  pthread_create(&work_thread, NULL, (void *(*)(void *)) transit_work_thread, sock);
  return work_thread;
}

/**
* @brief A Thread for Sending pkt
*
* @param sock
*
* @return 
*/
void * send_work_thread(tju_tcp_t* sock){
  while(TRUE){
    if(!is_empty_q(sock->sending_queue)) {
      sock->window.wnd_send->swnd = min(sock->window.wnd_send->rwnd, sock->window.wnd_send->cwnd);
      int size = sock->timers->size; 
      int swnd = sock->window.wnd_send->swnd;

      if(size >= sock->window.wnd_send->swnd) 
        continue; // Make sure the other side has enough space 

      trace_swnd(swnd);
      tju_packet_t *pkt = pop_q(sock->sending_queue);
      send_with_retransmit(sock, pkt, TRUE);
    }
  }
}
