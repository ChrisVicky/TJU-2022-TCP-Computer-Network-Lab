
#ifndef __TRAN_H__
#define __TRAN_H__
#include "global.h"
#include "tju_packet.h"

uint32_t auto_retransmit(tju_tcp_t *, tju_packet_t *pkt, int requiring_ack);
void on_ack_received(uint32_t ack, tju_tcp_t *sock);
void *transit_work_thread(time_list *list);
pthread_t start_work_thread(time_list *list);
void init_retransmit_timer();

#endif //__TRAN_H__
