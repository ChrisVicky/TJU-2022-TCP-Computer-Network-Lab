#ifndef _TJU_PACKET_H_
#define _TJU_PACKET_H_

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

#include "global.h"
#include "debug.h"

#define DEFAULT_HEADER_LEN 20
#define SYN_FLAG_MASK 0x8
#define ACK_FLAG_MASK 0x4
#define FIN_FLAG_MASK 0x2

// TCP 报文 header部分 的结构定义
typedef struct {
	uint16_t source_port;		//2 bytes 源端口
	uint16_t destination_port;	//2 bytes 目的端口
	uint32_t seq_num; 			//4 bytes sequence number
	uint32_t ack_num; 			//4 bytes ack number
	uint16_t hlen;				//2 bytes 包头长 这个项目里全是20
	uint16_t plen;				//2 bytes 包总长 包括包头和包携带的数据 20+数据长度 注意总长度不能超过MAX_LEN(1400) 防止IP层分片
	uint8_t flags;				//1 byte  标志位 比如 SYN FIN ACK 等
	uint16_t advertised_window; //2 bytes 接收方发送给发送方的建议窗口大小 用于流量控制
    uint8_t ext;				//1 byte  一些额外的数据 在这个项目里是为了将header的大小凑整到20bytes 没有实际意义
} tju_header_t;

// TCP 报文的结构定义
typedef struct {
	tju_header_t header;
	struct timeval sent_time;
	char* data;
} tju_packet_t;


/*
 输入header所有字段 和 TCP包数据内容及其长度
 构造tju_packet_t
 返回其指针
 */
tju_packet_t* create_packet(uint16_t src, uint16_t dst, uint32_t seq, 
    uint32_t ack, uint16_t hlen, uint16_t plen, uint8_t flags, 
    uint16_t adv_window, uint8_t ext, char* data, int len);

/*
 输入header所有字段 和 TCP包数据内容及其长度
 构造tju_packet_t 
 返回其对应的字符串
 */
char* create_packet_buf(uint16_t src, uint16_t dst, uint32_t seq, uint32_t ack,
    uint16_t hlen, uint16_t plen, uint8_t flags, uint16_t adv_window, 
    uint8_t ext, char* data, int len);

/*
 清除一个tju_packet_t的内存占用
 */
void free_packet(tju_packet_t* packet);

/*
 下面的函数全部都是从一个packet的字符串中
 根据各个字段的偏移量
 找到并返回对应的字段
*/ 
uint16_t get_src(char* msg);
uint16_t get_dst(char* msg);
uint32_t get_seq(char* msg);
uint32_t get_ack(char* msg);
uint16_t get_hlen(char* msg);
uint16_t get_plen(char* msg);
uint8_t get_flags(char* msg);
uint16_t get_advertised_window(char* msg);
uint8_t get_ext(char* msg);


/*############################################## 下面是实现上面函数功能的辅助函数 用户没必要调用 ##############################################*/
char* packet_to_buf(tju_packet_t* packet);
char* header_in_char(uint16_t src, uint16_t dst, uint32_t seq, uint32_t ack,
    uint16_t hlen, uint16_t plen, uint8_t flags, uint16_t adv_window, 
    uint8_t ext);


void print_tju_packet(tju_packet_t p);

tju_packet_t *buf_to_packet(char *buf);

#endif
