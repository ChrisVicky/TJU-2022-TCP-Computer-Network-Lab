#include "../inc/tran.h"
#include <arpa/inet.h>

#define abs_minus(x,y) ((x)<(y)?((y)-(x)):((x)-(y)))

uint32_t ack_id_hash[100000000];
static uint32_t last_ack = 0;

pthread_cond_t packet_available = PTHREAD_COND_INITIALIZER;
pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;

uint32_t get_ack_id_hash(int id){
  return ack_id_hash[id];
}

void set_ack_id_hash(int id, uint32_t value){
  ack_id_hash[id] = value;
}

timer_list* init_retransmit_timer() {
  timer_list* timers = init_timer_list();
  memset(ack_id_hash, 0, sizeof(ack_id_hash));
  return timers;
}

void update_rtt(double rtt, tju_tcp_t *tju_tcp) {
  double ertt = tju_tcp->window.wnd_send->estmated_rtt;
  double srtt = rtt; 
  double dev = tju_tcp->window.wnd_send->dev_rtt;
  tju_tcp->window.wnd_send->estmated_rtt = (RTT_ALPHA * srtt) + (1 - RTT_ALPHA) * ertt;
  tju_tcp->window.wnd_send->dev_rtt = RTT_BETA * dev + (1-RTT_BETA) * abs_minus(srtt, ertt);
  double candidate = tju_tcp->window.wnd_send->estmated_rtt + 4*tju_tcp->window.wnd_send->dev_rtt;
  _debug_("min(60, max(0.05, %f))\n" ,candidate);
  tju_tcp->window.wnd_send->rto = min(RTT_UBOUND, max(RTT_LBOUND, candidate));
  trace_rtts(srtt, tju_tcp->window.wnd_send->estmated_rtt, 
             tju_tcp->window.wnd_send->dev_rtt ,tju_tcp->window.wnd_send->rto);
}

void *retransmit(tju_packet_t* pkt, tju_tcp_t* tju_tcp) {
  uint16_t dlen = pkt->header.plen - pkt->header.hlen;
  if(ack_id_hash[pkt->header.seq_num + dlen]==0) return NULL;
  uint32_t id = set_timer(tju_tcp->timers,0,SEC2NANO(tju_tcp->window.wnd_send->rto),(void *(*)(void *, void*)) retransmit, pkt);
  ack_id_hash[pkt->header.seq_num + dlen] = id;
  safe_packet_sender(pkt);
  return NULL;
}

uint32_t send_with_retransmit(tju_tcp_t *sock, tju_packet_t *pkt, int requiring_ack){
  if (requiring_ack) {
    uint32_t id = 0;
    id = set_timer(sock->timers, 0, SEC2NANO(sock->window.wnd_send->rto), (void *(*)(void *, void*)) retransmit, pkt);
    uint16_t dlen = pkt->header.plen - pkt->header.hlen;
    if(pkt->header.flags!=NO_FLAG){
      ack_id_hash[pkt->header.seq_num + dlen + 1] = id;
    }else{
      ack_id_hash[pkt->header.seq_num + dlen] = id;
    }
  }
  safe_packet_sender(pkt);
  // trace_send(pkt->header.seq_num, pkt->header.ack_num, pkt->header.flags);
  return 0;
}

void free_retrans_arg(timer_event* ptr, tju_tcp_t* sock) {
  // struct timespec now;
  // clock_gettime(CLOCK_REALTIME, &now);
  // uint64_t current_time = TIMESPEC2NANO(now);
  // uint64_t create_time = TIMESPEC2NANO((*ptr->create_at));
  // update_rtt((current_time - create_time) / 1000000000.0, sock);
  free(ptr->args);
  ptr->args = NULL;
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
  _debug_("Received ack: %d\n" ,ack);
  int ret = 0;
  _debug_("LAST ACK: %d, ACK: %d\n" ,last_ack, ack);
  while(last_ack<=ack){
    if (ack_id_hash[last_ack] != 0) {
      pair *p = malloc(sizeof(pair));
      if(!ret && last_ack==ack){
        p->timeout = 1;
      }else{
        p->timeout = 0;
      }
      _debug_("Push Timer: %d\n", ack_id_hash[last_ack]);
      p->id = last_ack;
      while(pthread_mutex_lock(&sock->timers->queue->q_lock)!=0);
      push_q(sock->timers->queue, p); 
      pthread_mutex_unlock(&sock->timers->queue->q_lock);
      ret = 1;
    }
    last_ack++;
  }
  return ret;
}

tju_packet_t* mk_empty_pkt(tju_tcp_t* sock){
  return create_packet(sock->established_local_addr.port, sock->established_remote_addr.port,
                       sock->window.wnd_send->nextseq, 0, 
                       DEFAULT_HEADER_LEN, DEFAULT_HEADER_LEN, 
                       NO_FLAG, sock->window.wnd_recv->rwnd, 0, NULL, 0);
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
    check_timer(sock);
    while(pthread_mutex_lock(&sock->sending_queue->q_lock)!=0);
    if(!is_empty_q(sock->sending_queue)) {
      int pre = sock->window.wnd_send->swnd;
      sock->window.wnd_send->swnd = min(sock->window.wnd_send->rwnd, sock->window.wnd_send->cwnd);
      int size = sock->timers->size; 
      int swnd = sock->window.wnd_send->swnd;
      if (size < sock->window.wnd_send->swnd){
        _debug_("size %d vs %d\n" ,size, sock->window.wnd_send->swnd);
        if(pre != sock->window.wnd_send->swnd){
          trace_swnd(swnd);
        }
        tju_packet_t *pkt = pop_q(sock->sending_queue);
        send_with_retransmit(sock, pkt, TRUE);
      }else if(sock->window.wnd_send->swnd==0){
        tju_packet_t* pkt = mk_empty_pkt(sock);
        send_with_retransmit(sock, pkt, FALSE);
        _debug_("size %d vs %d\n" ,size, sock->window.wnd_send->swnd);
      }
    }
    pthread_mutex_unlock(&sock->sending_queue->q_lock);
  }
}
