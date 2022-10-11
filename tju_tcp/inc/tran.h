
#ifndef __TRAN_H__
#define __TRAN_H__
#include "global.h"
#include "tju_packet.h"
#include "tju_tcp.h"
#include "timer_helper.h"
#include "debug.h"

uint32_t send_with_retransmit(tju_tcp_t * sock, tju_packet_t *pkt, int requiring_ack);
int received_ack(uint32_t ack, tju_tcp_t *sock);
void *retransit_thread(timer_list *list);
pthread_t start_work_thread(tju_tcp_t*);
timer_list* init_retransmit_timer();

void free_retrans_arg(timer_event* ptr, tju_tcp_t* sock) ;
void * send_work_thread(tju_tcp_t* sock);
#endif //__TRAN_H__
