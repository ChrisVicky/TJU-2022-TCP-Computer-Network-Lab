#ifndef __TRACE_H__
#define __TRACE_H__
#include <stdio.h>
#include <sys/time.h>
#include "global.h"
#include "debug.h"
#include "tju_packet.h" 

void init_trace();

long get_current_time();

void trace_send(uint32_t seq, uint32_t ack, uint32_t flag);
void trace_recv(uint32_t seq, uint32_t ack, uint32_t flag);

void trace_cwnd(uint32_t type, uint32_t size);
void trace_rwnd(uint32_t size); // Recv windows buffer size 

void trace_swnd(uint32_t size); // sender windows size
void trace_rtts(double s, double e, double d, double t); // Sample, Est, Deviation Rtt Timeout Interval Difference
void trace_delv(uint32_t seq, uint32_t size); // Recv Window diliver data to recv buff

#endif // !__TRACE_H__
