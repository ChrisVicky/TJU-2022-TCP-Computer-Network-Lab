#include "../inc/tju_tcp.h"
#include <arpa/inet.h>
#include <stdio.h>

// Multi Threads use the same debug function to show debug information
pthread_mutex_t thread_print_lock = PTHREAD_MUTEX_INITIALIZER;

/*
创建 TCP socket 
初始化对应的结构体
设置初始状态为 CLOSED
*/
tju_tcp_t* tju_socket(){
  init_retransmit_timer();
  init_trace();
  tju_tcp_t *sock = (tju_tcp_t *) malloc(sizeof(tju_tcp_t));
  sock->state = CLOSED;

  pthread_mutex_init(&(sock->send_lock), NULL);
  sock->sending_queue = init_q();
  sock->sending_buf = NULL;
  sock->sending_len = 0;

  pthread_mutex_init(&(sock->recv_lock), NULL);
  sock->received_buf = NULL;
  sock->received_len = 0;

  if (pthread_cond_init(&sock->wait_cond, NULL) != 0) {
    perror("ERROR condition variable not set\n");
    exit(-1);
  }

  sock->window.wnd_send = (sender_window_t*)   malloc(sizeof(sender_window_t));
  sock->window.wnd_recv = (receiver_window_t*) malloc(sizeof(receiver_window_t));

  sock->window.wnd_send->rto = INIT_RTT;
  sock->window.wnd_send->estmated_rtt = INIT_RTT;
  sock->window.wnd_send->window_size = INIT_WINDOW_SIZE;
  sock->window.wnd_send->prev_ack_count = 0;
  sock->window.wnd_send->prev_ack = 0;

  sock->window.wnd_recv->buff_tree = NULL; 

  sock->half_queue = init_q();
  sock->full_queue = init_q();

  return sock;
}

/*
绑定监听的地址 包括ip和端口
*/
int tju_bind(tju_tcp_t* sock, tju_sock_addr bind_addr){
  if(bhash[bind_addr.port]){
    _debug_("端口 %d 已经被占用, bind 失败\n",bind_addr.port);
    return -1;
  }
  bhash[bind_addr.port] = 1;
  sock->bind_addr = bind_addr;
  return 0;
}

/*
被动打开 监听bind的地址和端口
设置socket的状态为LISTEN
注册该socket到内核的监听socket哈希表
*/
int tju_listen(tju_tcp_t* sock){
  sock->state = LISTEN;
  int hashval = cal_hash(sock->bind_addr.ip, sock->bind_addr.port, 0, 0);
  listen_socks[hashval] = sock;
  _debug_("HashVal: %d\n" ,hashval);
  return 0;
}

/*
接受连接 
返回与客户端通信用的socket
这里返回的socket一定是已经完成3次握手建立了连接的socket
因为只要该函数返回, 用户就可以马上使用该socket进行send和recv
*/
tju_tcp_t* tju_accept(tju_tcp_t* listen_sock){

  while(!listen_sock->full_queue->size); // NOTE: 当 listen 的全连接中没有新来的时候阻塞
  _debug_("Full Queue Received\n");
  tju_tcp_t * new_connection = pop_q(listen_sock->full_queue);

  tju_sock_addr local_addr = new_connection->established_local_addr;
  tju_sock_addr remote_addr = new_connection->established_remote_addr;

  pthread_mutex_init(&(new_connection->recv_lock), NULL);
  // 将新的conn放到内核建立连接的socket哈希表中
  int hashval = cal_hash(local_addr.ip, local_addr.port, remote_addr.ip, remote_addr.port);
  established_socks[hashval] = new_connection;
  _debug_("New Tcp Connection accepted! establish hash: %d\n" ,hashval);
  return new_connection;
}


/*
连接到服务端
该函数以一个socket为参数
调用函数前, 该socket还未建立连接
函数正常返回后, 该socket一定是已经完成了3次握手, 建立了连接
因为只要该函数返回, 用户就可以马上使用该socket进行send和recv
*/
int tju_connect(tju_tcp_t* sock, tju_sock_addr target_addr){

  tju_sock_addr local_addr;
  local_addr.ip = inet_network("172.17.0.2");
  local_addr.port = 5678; // 连接方进行connect连接的时候 内核中是随机分配一个可用的端口
  sock->established_local_addr = local_addr;
  sock->established_remote_addr = target_addr;

  sock->window.wnd_send->nextseq = INIT_CLIENT_SEQ;
  sock->window.wnd_send->base = INIT_CLIENT_SEQ;
  // 将建立了连接的socket放入内核 已建立连接哈希表中
  int hashval = cal_hash(local_addr.ip, local_addr.port, 0, 0);
  listen_socks[hashval] = sock;

  tju_packet_t* pkt = create_packet(
      sock->established_local_addr.port, sock->established_remote_addr.port,
      CLIENT_ISN, 0 ,
      DEFAULT_HEADER_LEN, DEFAULT_HEADER_LEN,
      SYN_FLAG_MASK, 1, 0, NULL, 0);

  send_with_retransmit(sock, pkt, FALSE);

  sock->window.wnd_send->nextseq++;
  sock->state = SYN_SENT;
  _debug_("waiting FOR establish\n");
  while(sock->state != ESTABLISHED); // 等待对方将我方的状态转换成 ESTABLISHED

  listen_socks[hashval] = NULL;
  hashval = cal_hash(local_addr.ip, local_addr.port, target_addr.ip, target_addr.port);
  established_socks[hashval] = sock;
  return 0;
}


/**
* @brief Safely send tju_packet_t (with free)
*
* @param packet
*/
void safe_packet_sender(tju_packet_t * packet){
  char *pkt = packet_to_buf(packet);
  sendToLayer3(pkt, packet->header.plen);
  free(pkt);
  return ;
}

int tju_send(tju_tcp_t* sock, const void *buffer, int len){
  _debug_("message to send:>%s<\n", (char *) buffer);
  pthread_mutex_lock(&sock->send_lock);
  int count = len / MAX_DLEN;
  for (int i = 0; i <= count; ++i) {
    _debug_("Slide %d\n", i);
    int send_len = (i == count) ? len % MAX_DLEN : MAX_DLEN;
    if(send_len==0) break;
    tju_packet_t *pkt = create_packet(sock->established_local_addr.port, sock->established_remote_addr.port,
                                      sock->window.wnd_send->nextseq, 0,
                                      DEFAULT_HEADER_LEN, DEFAULT_HEADER_LEN + send_len, NO_FLAG, 1, 0,
                                      (char *) buffer + i * MAX_DLEN, send_len);
    push_q(sock->sending_queue, pkt);
    sock->window.wnd_send->nextseq += pkt->header.plen - DEFAULT_HEADER_LEN + 1;
  }

  // _debug_("Push Finished\n");

  while (!is_empty_q(sock->sending_queue)) {
    tju_packet_t *pkt = pop_q(sock->sending_queue);
    while ((sock->window.wnd_send->base + sock->window.wnd_send->window_size * MAX_DLEN) < (sock->window.wnd_send->nextseq)); // Make sure the other side has enough space 
    _debug_("Send with Retransmit\n");
    send_with_retransmit(sock, pkt, TRUE);
    _debug_("Trace Send\n");
    trace_send(pkt->header.seq_num, pkt->header.ack_num, pkt->header.flags);
  }
  _debug_("Sent end\n");
  pthread_mutex_unlock(&sock->send_lock);
  return 0;
}

int tju_recv(tju_tcp_t* sock, void *buffer, int len){
  while(sock->received_len<=0){
    // 阻塞
  }
  while(pthread_mutex_lock(&(sock->recv_lock)) != 0); // 加锁

  int read_len = 0;
  if (sock->received_len >= len){ // 从中读取len长度的数据
    read_len = len;
  }else{
    read_len = sock->received_len; // 读取sock->received_len长度的数据(全读出来)
  }

  memcpy(buffer, sock->received_buf, read_len);

  if(read_len < sock->received_len) { // 还剩下一些
    char* new_buf = malloc(sock->received_len - read_len);
    memcpy(new_buf, sock->received_buf + read_len, sock->received_len - read_len);
    free(sock->received_buf);
    sock->received_len -= read_len;
    sock->received_buf = new_buf;
  }else{
    free(sock->received_buf);
    sock->received_buf = NULL;
    sock->received_len = 0;
  }
  pthread_mutex_unlock(&(sock->recv_lock)); // 解锁

  return 0;
}

int tju_handle_packet(tju_tcp_t *sock, char *pkt) {

  uint32_t data_len = get_plen(pkt) - DEFAULT_HEADER_LEN;

  // 把收到的数据放到接受缓冲区
  pthread_mutex_lock(&(sock->recv_lock)); // 加锁

  _debug_("------------- start handle ----\n");
  uint8_t flag = get_flags(pkt);
  uint32_t seq = get_seq(pkt);
  uint32_t ack = get_ack(pkt);
  if (flag & ACK_FLAG_MASK) {
    received_ack(ack, sock);
  }
  uint16_t src_port = get_src(pkt);
  uint16_t dst_port = get_dst(pkt);

  // trace_recv(seq, ack, flag);

  tju_tcp_t *new_conn = NULL;

  _debug_("============== STATE:%d, start handle======\n", sock->state);

  switch (sock->state) {
    case LISTEN:
      if (flag == SYN_FLAG_MASK) {
        _debug_("SYN_FLAG RECEIVED : %d\n",flag);
        sock->state = SYN_RECV;
        sock->window.wnd_send->nextseq = INIT_SERVER_SEQ;
        sock->window.wnd_send->base = INIT_SERVER_SEQ;
        //SEND SYN ACK
        tju_packet_t *pkt = create_packet(dst_port,
                                          src_port,
                                          sock->window.wnd_send->nextseq,
                                          seq + 1,
                                          DEFAULT_HEADER_LEN,
                                          DEFAULT_HEADER_LEN,
                                          SYN_FLAG_MASK | ACK_FLAG_MASK,
                                          1,
                                          0,
                                          NULL,
                                          0);
        send_with_retransmit(sock, pkt, TRUE);
        sock->window.wnd_send->nextseq += 1;
        _debug_("SYN_ACK SENT\n");
        sock->window.wnd_recv->expect_seq = seq + 1;
        _debug_("expect_seq:%d\n", sock->window.wnd_recv->expect_seq);
      }
      break;
    case SYN_SENT:
      if (flag == (SYN_FLAG_MASK | ACK_FLAG_MASK)) {
        _debug_("SYN_ACK RECEIVED\n");
        sock->state = ESTABLISHED;

        //SEND ACK
        tju_packet_t *pkt = create_packet(dst_port, src_port, sock->window.wnd_send->nextseq, seq + 1,
                                          DEFAULT_HEADER_LEN, DEFAULT_HEADER_LEN, ACK_FLAG_MASK, 1, 0, NULL, 0);
        send_with_retransmit(sock, pkt, FALSE);
        _debug_("ACK SENT\n");
        sock->window.wnd_recv->expect_seq = seq + 1;
        _debug_("expect_seq:%d\n", sock->window.wnd_recv->expect_seq);
      }
      break;
    case SYN_RECV:
      if (flag == ACK_FLAG_MASK) {
        _debug_("ACK RECEIVED\n");
        new_conn = (tju_tcp_t *) malloc(sizeof(tju_tcp_t));
        memcpy(new_conn, sock, sizeof(tju_tcp_t));
        new_conn->established_local_addr = sock->bind_addr;
        new_conn->established_remote_addr.port = src_port;
        new_conn->established_remote_addr.ip = inet_network("172.17.0.2");
        new_conn->state = ESTABLISHED;
        push_q(sock->full_queue, new_conn);
      }
      break;
    case ESTABLISHED:
      if (flag == NO_FLAG) {
        _debug_("PKT received with seq: %d, dlen: %d\n", seq, data_len);
        tju_packet_t *ack_pkt = create_packet(dst_port, src_port, 0, seq + data_len + 1,
                                              DEFAULT_HEADER_LEN, DEFAULT_HEADER_LEN, ACK_FLAG_MASK, 1, 0, NULL, 0);
        send_with_retransmit(sock, ack_pkt, FALSE);
        free_packet(ack_pkt);
        if (data_len > 0) {
          //hashmap_set(sock->window.wnd_recv->received_map, buf_to_packet(pkt));
          insert_tree(sock->window.wnd_recv->buff_tree, seq, buf_to_packet(pkt));
          if (sock->window.wnd_recv->expect_seq == seq) {
            _debug_("expect_seq:%d updated!\n", sock->window.wnd_recv->expect_seq);
            if (sock->received_buf == NULL || sock->received_len == 0) {
              sock->received_buf = malloc(data_len);
            } else {
              sock->received_buf = realloc(sock->received_buf, sock->received_len + data_len);
            }
            memcpy(sock->received_buf + sock->received_len, pkt + DEFAULT_HEADER_LEN, data_len);
            sock->received_len += data_len;
            sock->window.wnd_recv->expect_seq += data_len + 1;
            _debug_("expect_seq:%d\n", sock->window.wnd_recv->expect_seq);
            tju_packet_t *tmp = NULL;
/*
            tmp = hashmap_get(sock->window.wnd_recv->received_map,
                              &(tju_packet_t) {.header.seq_num = sock->window.wnd_recv->expect_seq});
*/
            tmp = find_key(sock->window.wnd_recv->buff_tree, sock->window.wnd_recv->expect_seq)->value;
            while (tmp) {
              _debug_("Find buffered packet with seq: %d\n", tmp->header.seq_num);
              uint32_t next_data_len = tmp->header.plen - DEFAULT_HEADER_LEN;
              sock->received_buf = realloc(sock->received_buf, sock->received_len + next_data_len);
              memcpy(sock->received_buf + sock->received_len, tmp->data, next_data_len);
              sock->received_len += next_data_len;
              free(tmp->data);
              //hashmap_delete(sock->window.wnd_recv->received_map, tmp);
              free(tmp);
              sock->window.wnd_recv->expect_seq += next_data_len + 1;
              _debug_("expect_seq:%d\n", sock->window.wnd_recv->expect_seq);
              tmp = find_key(sock->window.wnd_recv->buff_tree, sock->window.wnd_recv->expect_seq)->value;
/*
              tmp = hashmap_get(sock->window.wnd_recv->received_map,
                                &(tju_packet_t) {.header.seq_num = sock->window.wnd_recv->expect_seq});
*/
            }
          }
        }
      } else if (flag == ACK_FLAG_MASK) {
        _debug_("new ACK RECEIVED\n");
      }
      break;
  }

/*
  if (data_len > 0) {
    if (sock->received_buf == NULL) {
      sock->received_buf = malloc(data_len);
    } else {
      if (sock->received_len == 0) {
        sock->received_buf = malloc(data_len);
      } else {
        sock->received_buf = realloc(sock->received_buf, sock->received_len + data_len);
      }
    }
    memset(sock->received_buf + sock->received_len, 0, data_len);
    memcpy(sock->received_buf + sock->received_len, pkt + DEFAULT_HEADER_LEN, data_len);
    sock->received_len += data_len;
  }
*/
  pthread_mutex_unlock(&(sock->recv_lock)); // 解锁


  return 0;
}
// // TODO: NOT IMPLEMENTED
// int tju_handle_packet(tju_tcp_t* sock, char* pkt){
//
//   uint32_t data_len = get_plen(pkt) - DEFAULT_HEADER_LEN;
//
//   // 把收到的数据放到接受缓冲区
//   while(pthread_mutex_lock(&(sock->recv_lock)) != 0); // 加锁
//
//   if(sock->received_buf == NULL){
//     sock->received_buf = malloc(data_len);
//   }else {
//     sock->received_buf = realloc(sock->received_buf, sock->received_len + data_len);
//   }
//   memcpy(sock->received_buf + sock->received_len, pkt + DEFAULT_HEADER_LEN, data_len);
//   sock->received_len += data_len;
//
//
//   int pkt_seq = get_seq(pkt);
//   int pkt_src = get_src(pkt);
//   int pkt_ack = get_ack(pkt);
//   int pkt_plen = get_plen(pkt);
//   int pkt_flag = get_flags(pkt);
//
//   _debug_("==> 开始 handle packet\n");
//
//   switch (sock->state) {
//     case LISTEN:
//       if(pkt_flag==SYN_FLAG_MASK){
//         // 收到 SYN_FLAG --> 连接第一次握手
//         _debug_("LISTEN 状态 收到 SYN_FLAG\n");
//         tju_tcp_t * new_sock = (tju_tcp_t*)malloc(sizeof(tju_tcp_t));
//         memcpy(new_sock, sock, sizeof(tju_tcp_t));
//
//         tju_sock_addr remote_addr, local_addr;
//         remote_addr.ip = inet_network("172.0.0.2"); // Listen 是 server 端的行为，所以远程地址就是 172.0.0.2 
//         remote_addr.port = pkt_src;
//         local_addr.ip = sock->bind_addr.ip;
//         local_addr.port = sock->bind_addr.port;
//
//         new_sock->established_local_addr = local_addr;
//         new_sock->established_remote_addr = remote_addr;
//
//         new_sock->state = SYN_RECV;
//         push_q(sock->half_queue, new_sock);
//         _debug_("新 socket 准备好，可以push\n");
//
//         tju_packet_t * ret_pack = create_packet(local_addr.port, remote_addr.port, SERVER_ISN, pkt_seq+1,
//                                                 DEFAULT_HEADER_LEN, DEFAULT_HEADER_LEN, ACK_FLAG_MASK | SYN_FLAG_MASK, 1 , 0 , NULL, 0);
//         char *msg = packet_to_buf(ret_pack);
//         sendToLayer3(msg, DEFAULT_HEADER_LEN);
//         _debug_("Server 接受到 client 的第一次握手，返回第二次握手\n");
//
//       }else if(pkt_flag==ACK_FLAG_MASK){
//         // 在 Listen 状态接收到 ACK 表明是第三次握手
//         // 检查对方 Ack Number 是否正确
//         if(pkt_ack==SERVER_ISN+1){
//           tju_tcp_t * established_conn = pop_q(sock->half_queue);
//           if(established_conn == NULL){
//             _debug_("Establshed 是 NULL\n");
//           }
//           established_conn->state = ESTABLISHED;
//           push_q(sock->full_queue, established_conn);
//           _debug_("收到正确的 Ack 三次握手建立成功（至少server 端这么认为）\n");
//
//           return 0;
//         }else{
//           _debug_("client 端发送的 ack 错误，期待>%d<，接收到>%d<\n",SERVER_ISN+1, pkt_ack);
//         }
//       }else{
//         _debug_("当前为 LISTEN，接收到其他 flag 的报文，丢弃之\n");
//       }
//       break;
//
//     case SYN_SENT:
//       if(pkt_ack == CLIENT_ISN + 1){
//         tju_packet_t * new_pack = create_packet(sock->established_local_addr.port, sock->established_remote_addr.port, pkt_ack, pkt_seq+1,
//                                                 DEFAULT_HEADER_LEN, DEFAULT_HEADER_LEN, ACK_FLAG_MASK, 1, 0, NULL, 0);
//         char *msg = packet_to_buf(new_pack);
//         sendToLayer3(msg, DEFAULT_HEADER_LEN);
//         _debug_("Client 端发送了第三次握手，建立成功\n");
//         sock->state = ESTABLISHED;
//       }else{
//         _debug_("Client 端接收到了不正确的ack\n");
//       }
//       break;
//     default:
//       _debug_("NOT IMPLEMENTED\n");
//   }
//
//   pthread_mutex_unlock(&(sock->recv_lock)); // 解锁
//   return 0;
// }

int tju_close (tju_tcp_t* sock){
  return 0;
}



