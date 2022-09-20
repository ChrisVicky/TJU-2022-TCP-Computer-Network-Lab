#ifndef _KERNEL_H_
#define _KERNEL_H_

#include "global.h"
#include "tju_packet.h"
#include <unistd.h>
#include "tju_tcp.h"

#define MAX_SOCK 32
tju_tcp_t* listen_socks[MAX_SOCK];
tju_tcp_t* established_socks[MAX_SOCK];

// NOTE: 添加了一个端口分配表
#define MAX_PORT 100000
int bhash[MAX_PORT];
 

/*
模拟Linux内核收到一份TCP报文的处理函数
*/
void onTCPPocket(char* pkt);


/*
以用户填写的TCP报文为参数
根据用户填写的TCP的目的IP和目的端口,向该地址发送数据报
*/
void sendToLayer3(char* packet_buf, int packet_len);


/*
开启仿真, 运行起后台线程
*/
void startSimulation();


/*
 使用UDP进行数据接收的线程
*/
void* receive_thread(void * in);

// 接受UDP的socket的标识符
int BACKEND_UDPSOCKET_ID;


/*
 linux内核会根据
 本地IP 本地PORT 远端IP 远端PORT 计算hash值 四元组 
 找到唯一的那个socket

 (实际上真正区分socket的是五元组
  还有一个协议字段
  不过由于本项目是TCP 协议都一样, 就没必要了)
*/
int cal_hash(uint32_t local_ip, uint16_t local_port, uint32_t remote_ip, uint16_t remote_port);

#endif
