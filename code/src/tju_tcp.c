#include "../inc/tju_tcp.h"
#include <arpa/inet.h>
#include <stdio.h>

// Multi Threads use the same debug function to show debug information
pthread_mutex_t thread_print_lock = PTHREAD_MUTEX_INITIALIZER;
extern sender_window_t global_sender;
extern FILE* debug_file;

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
  sock->window.wnd_send->rwnd = 32*MAX_DLEN;
  sock->window.wnd_recv->buff_tree = init_tree(); 
  sock->window.wnd_recv->rwnd = 32*MAX_DLEN;
  sock->window.wnd_send->dev_rtt = 0;

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
  tju_tcp_t * new_connection = pop_q(listen_sock->full_queue);

  tju_sock_addr local_addr = new_connection->established_local_addr;
  tju_sock_addr remote_addr = new_connection->established_remote_addr;

  pthread_mutex_init(&(new_connection->recv_lock), NULL);
  // 将新的conn放到内核建立连接的socket哈希表中
  int hashval = cal_hash(local_addr.ip, local_addr.port, remote_addr.ip, remote_addr.port);
  established_socks[hashval] = new_connection;
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

  send_with_retransmit(sock, pkt, TRUE);  // TODO: check 一下需不需要重传

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
  pthread_mutex_lock(&sock->send_lock);
  int count = len / MAX_DLEN;
  sock->window.wnd_send->window_size = sock->window.wnd_send->rwnd;
  trace_swnd(sock->window.wnd_send->window_size);
  for (int i = 0; i <= count; ++i) {
    int send_len = (i == count) ? len % MAX_DLEN : MAX_DLEN;
    if(send_len==0) break;
    tju_packet_t *pkt = create_packet(sock->established_local_addr.port, sock->established_remote_addr.port,
                                      sock->window.wnd_send->nextseq, 0,
                                      DEFAULT_HEADER_LEN, DEFAULT_HEADER_LEN + send_len, NO_FLAG, 1, 0,
                                      (char *) buffer + i * MAX_DLEN, send_len);
    push_q(sock->sending_queue, pkt);
    sock->window.wnd_send->nextseq += pkt->header.plen - DEFAULT_HEADER_LEN + 1;
  }

  while (!is_empty_q(sock->sending_queue)) {
    tju_packet_t *pkt = pop_q(sock->sending_queue);
    while ((sock->window.wnd_send->base + sock->window.wnd_send->window_size * MAX_DLEN) < (sock->window.wnd_send->nextseq)); // Make sure the other side has enough space 
    send_with_retransmit(sock, pkt, TRUE);
  }
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
  sock->window.wnd_recv->rwnd = 32*MAX_DLEN - sock->received_len;
  // NOTE: 此处为 Server 端更新，rwnd 信息应在 client 端输出
  // trace_rwnd(sock->window.wnd_recv->rwnd);
  pthread_mutex_unlock(&(sock->recv_lock)); // 解锁

  return read_len;
}

int tju_handle_packet(tju_tcp_t *sock, char *pkt) {

  // 把收到的数据放到接受缓冲区

  _debug_line_("start handle");
  uint32_t data_len = get_plen(pkt) - DEFAULT_HEADER_LEN;
  uint8_t flag = get_flags(pkt);
  uint32_t seq = get_seq(pkt);
  uint32_t ack = get_ack(pkt);
  uint16_t rwnd_pkt = get_advertised_window(pkt);
  if (flag & ACK_FLAG_MASK) {
    received_ack(ack, sock);
    sock->window.wnd_send->rwnd = rwnd_pkt;
    trace_rwnd(rwnd_pkt);
  }
  uint16_t src_port = get_src(pkt);
  uint16_t dst_port = get_dst(pkt);

  trace_recv(seq, ack, flag);

  tju_tcp_t *new_conn = NULL;

  _debug_line_("STATE:%d, start handle", sock->state);

  switch (sock->state) {
    case LISTEN: // tju acc 之前 server 
      if (flag == SYN_FLAG_MASK) {
        _debug_("SYN_FLAG RECEIVED : %d\n",flag);
        sock->state = SYN_RECV;
        sock->window.wnd_send->nextseq = INIT_SERVER_SEQ;
        sock->window.wnd_send->base = INIT_SERVER_SEQ;
        //SEND SYN ACK
        tju_packet_t *pkt = create_packet(dst_port, src_port, sock->window.wnd_send->nextseq,seq + 1,
                                          DEFAULT_HEADER_LEN, DEFAULT_HEADER_LEN, SYN_FLAG_MASK | ACK_FLAG_MASK, 
                                          sock->window.wnd_recv->rwnd, 0, NULL, 0);
        send_with_retransmit(sock, pkt, TRUE);
        sock->window.wnd_send->nextseq += 1;
        _debug_("SYN_ACK SENT\n");
        sock->window.wnd_recv->expect_seq = seq + 1;
        _debug_("expect_seq:%d\n", sock->window.wnd_recv->expect_seq);
      }else{
        _debug_("Current Listen, Flag Error: %d\n",flag);
      }
      break;
    case SYN_SENT:
      if (flag == (SYN_FLAG_MASK | ACK_FLAG_MASK)) {
        sock->state = ESTABLISHED;
        //SEND ACK
        tju_packet_t *pkt = create_packet(dst_port, src_port, sock->window.wnd_send->nextseq, seq + 1,
                                          DEFAULT_HEADER_LEN, DEFAULT_HEADER_LEN, ACK_FLAG_MASK, sock->window.wnd_recv->rwnd, 0, NULL, 0);
        send_with_retransmit(sock, pkt, FALSE);
        free_packet(pkt);
        _debug_("client sent syn_flag_mask, ack_flag_mask\n");
        sock->window.wnd_recv->expect_seq = seq + 1;
        _debug_("expect_seq:%d\n", sock->window.wnd_recv->expect_seq);
      }
      break;
    case SYN_RECV:
      if (flag == ACK_FLAG_MASK) {
        sock->state = LISTEN;
        _debug_("ACK RECEIVED, Full Connection extablished\n");
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
        _debug_("PKT received with seq: %d, dlen: %d, expect seq: %d\n", seq, data_len, sock->window.wnd_recv->expect_seq);
        tju_packet_t *ack_pkt = create_packet(dst_port, src_port, 0, seq + data_len + 1,
                                              DEFAULT_HEADER_LEN, DEFAULT_HEADER_LEN, ACK_FLAG_MASK, sock->window.wnd_recv->rwnd, 0, NULL, 0);
        send_with_retransmit(sock, ack_pkt, FALSE);
        free_packet(ack_pkt);

        // NOTE: Check if seq is less than the last ack 
        // NOTE: (in which case the pkt is saved, and should get dropped);
        if(seq < sock->window.wnd_recv->expect_seq){
          _debug_("Pkt Received, Drop pkt\n");
        }else if (data_len > 0 ) {
          if (sock->window.wnd_recv->expect_seq == seq) {
            pthread_mutex_lock(&(sock->recv_lock)); // 加锁
            // _debug_("expect_seq:%d updated!\n", sock->window.wnd_recv->expect_seq);
            trace_delv(seq, data_len);
            if (sock->received_buf == NULL || sock->received_len == 0) {
              sock->received_buf = malloc(data_len);
            } else {
              sock->received_buf = realloc(sock->received_buf, sock->received_len + data_len);
            }
            memcpy(sock->received_buf + sock->received_len, pkt + DEFAULT_HEADER_LEN, data_len);
            sock->received_len += data_len;
            sock->window.wnd_recv->rwnd = 32*MAX_DLEN - sock->received_len;
            // NOTE: server 不展示自己的 rwnd
            // trace_rwnd(sock->window.wnd_recv->rwnd);
            sock->window.wnd_recv->expect_seq += data_len + 1;
            _debug_("seq: %d pkt in receive buff, waiting for collecting, expect_seq:%d\n",seq, sock->window.wnd_recv->expect_seq);

            // NOTE: Dealing with situations where older packets arrived earlier
            tju_packet_t *tmp = get_value(sock->window.wnd_recv->buff_tree, sock->window.wnd_recv->expect_seq);
            // print_tree(sock->window.wnd_recv->buff_tree);
            while (tmp != NULL) {
              _debug_("Find buffered packet with seq: %d\n", tmp->header.seq_num);
              uint32_t next_data_len = tmp->header.plen - DEFAULT_HEADER_LEN;
              sock->received_buf = realloc(sock->received_buf, sock->received_len + next_data_len);
              memcpy(sock->received_buf + sock->received_len, tmp->data, next_data_len);
              trace_delv(tmp->header.seq_num, next_data_len);
              sock->received_len += next_data_len;
              // NOTE: server 不展示自己的 rwnd
              // trace_rwnd(32*MAX_DLEN - sock->received_len);
              free(tmp->data);
              free(tmp);
              tmp = NULL;
              sock->window.wnd_recv->expect_seq += next_data_len + 1;
              _debug_("expect_seq:%d\n", sock->window.wnd_recv->expect_seq);
              tmp = get_value(sock->window.wnd_recv->buff_tree, sock->window.wnd_recv->expect_seq);
              // print_tree(sock->window.wnd_recv->buff_tree);
            }
            pthread_mutex_unlock(&(sock->recv_lock)); // 解锁
          }else{
            // NOTE: Only when seq mismatch, indicating that Older Packet arrived;
            // AVL Tree 自平衡二叉树 --> 插入，删除，查找 lg(n) 
            insert_key_value(sock->window.wnd_recv->buff_tree, seq, buf_to_packet(pkt));
          }
        }
      } else if (flag == ACK_FLAG_MASK) {
        _debug_("ACK RECEIVED done before here\n");
      }
      break;
  }

  return 0;
}

int tju_close (tju_tcp_t* sock){
  return 0;
}



