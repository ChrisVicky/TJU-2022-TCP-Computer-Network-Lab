#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <netinet/in.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include <arpa/inet.h>

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))
// 单位是byte
#define SIZE32 4
#define SIZE16 2
#define SIZE8  1

#define INIT_SERVER_SEQ 0
#define INIT_CLIENT_SEQ 0

//RTT CALCULATION
#define RTT_ALPHA 0.125
#define RTT_BETA 0.25
#define INIT_RTT 0.05
#define RTT_UBOUND 60
#define RTT_LBOUND 0.05

#ifdef CUSTOM_WINDOW_SIZE
#define INIT_WINDOW_SIZE CUSTOM_WINDOW_SIZE
#else
#define INIT_WINDOW_SIZE 50
#endif

// 一些Flag
#define NO_FLAG 0
#define NO_WAIT 1
#define TIMEOUT 2
#define TRUE 1
#define FALSE 0

// 定义最大包长 防止IP层分片
#define MAX_DLEN 1375 	// 最大包内数据长度
#define MAX_LEN 1400 	// 最大包长度

// TCP socket 状态定义
#define CLOSED 0
#define LISTEN 1
#define SYN_SENT 2
#define SYN_RECV 3
#define ESTABLISHED 4
#define FIN_WAIT_1 5
#define FIN_WAIT_2 6
#define CLOSE_WAIT 7
#define CLOSING 8
#define LAST_ACK 9
#define TIME_WAIT 10

// TCP 拥塞控制状态
#define SLOW_START 0
#define CONGESTION_AVOIDANCE 1
#define FAST_RECOVERY 2

// TCP 接受窗口大小
#define TCP_RECVWN_SIZE 32*MAX_DLEN // 比如最多放32个满载数据包

// seq start for CLINET and SERVER
#define CLIENT_ISN 0
#define SERVER_ISN 0

// 定义 flags
#define SYN_FLAG 1
#define ACK_FLAG 2
#define FIN_FLAG 3
#define ACK_SYN_FLAG 4


// TCP 发送窗口
// 注释的内容如果想用就可以用 不想用就删掉 仅仅提供思路和灵感
typedef struct {
  uint32_t window_size;
  uint32_t seq;
  uint32_t base;
  uint32_t nextseq;
  uint32_t prev_ack;
  uint32_t prev_ack_count; // 记录冗余 ack 
  double estmated_rtt;
  double rto;
  double dev_rtt;
  uint16_t cwnd;
  uint16_t swnd;
  uint16_t rwnd;
} sender_window_t;

// TCP 接受窗口
// 注释的内容如果想用就可以用 不想用就删掉 仅仅提供思路和灵感
typedef struct {
	char received[TCP_RECVWN_SIZE];

//   received_packet_t* head;
//   char buf[TCP_RECVWN_SIZE];
//   uint8_t marked[TCP_RECVWN_SIZE];
  uint32_t expect_seq;
  struct myTree* buff_tree;
  uint16_t rwnd;
} receiver_window_t;

// TCP 窗口 每个建立了连接的TCP都包括发送和接受两个窗口
typedef struct {
	sender_window_t* wnd_send;
  	receiver_window_t* wnd_recv;
} window_t;

typedef struct {
	uint32_t ip;
	uint16_t port;
} tju_sock_addr;


// TJU_TCP 结构体 保存TJU_TCP用到的各种数据
typedef struct {
	int state; // TCP的状态

	tju_sock_addr bind_addr; // 存放bind和listen时该socket绑定的IP和端口
	tju_sock_addr established_local_addr; // 存放建立连接后 本机的 IP和端口
	tju_sock_addr established_remote_addr; // 存放建立连接后 连接对方的 IP和端口

	pthread_mutex_t send_lock; // 发送数据锁
	// char* sending_buf; // 发送数据缓存区
	// int sending_len; // 发送数据缓存长度
  struct myQueue* sending_queue; // 用 queue 将 sending thread 和 tju_send 连接
  struct timer_list* timers; // 用 timers 控制发送数据量

	pthread_mutex_t recv_lock; // 接收数据锁
  // char* received_hdr_buf; // 接收的头部缓冲区
  // int received_hdr_len; // 头部缓冲区长度
	char* received_buf; // 接收数据缓存区
	int received_len; // 接收数据缓存长度
  
  int curr_data_buf_len; // 当前数据缓冲区中数据长度
  char* curr_able_data_buf_point; // 数据buff中第一个空位置
  char* curr_able_hdr_buf_point; // 头部 buff 第一个空位置

	pthread_cond_t wait_cond; // 可以被用来唤醒recv函数调用时等待的线程

	window_t window; // 发送和接受窗口

  uint32_t seq; // 序号，发送完后立刻增加
  uint32_t ack; // 发送是直接使用

  struct myQueue *half_queue;
  struct myQueue *full_queue;

} tju_tcp_t;

typedef struct transmit_arg_t{
  tju_tcp_t * sock;
  void * pkt;
}transmit_arg_t;

typedef struct pair{
  int id;
  uint64_t timeout;
}pair;

#endif
