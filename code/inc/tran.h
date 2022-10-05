
#ifndef __TRAN_H__
#define __TRAN_H__
#include "global.h"
#include "tju_packet.h"
#include "tju_tcp.h"
#include "timer_helper.h"

uint32_t send_with_retransmit(tju_tcp_t * sock, tju_packet_t *pkt, int requiring_ack);
void received_ack(uint32_t ack, tju_tcp_t *sock);
void *retransit_thread(timer_list *list);
pthread_t start_work_thread(timer_list *list);
void init_retransmit_timer();

#endif //__TRAN_H__
