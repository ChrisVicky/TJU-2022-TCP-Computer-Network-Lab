#include "../inc/tju_tcp.h"
#include <arpa/inet.h>
#include <stdio.h>

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
    DEBUG("端口 %d 已经被占用, bind 失败\n",bind_addr.port);
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

  // TODO: 要不要进行 pthread_mutex_lock 的初始化

  tju_sock_addr local_addr = new_connection->established_local_addr;
  tju_sock_addr remote_addr = new_connection->established_remote_addr;

  // 将新的conn放到内核建立连接的socket哈希表中
  int hashval = cal_hash(local_addr.ip, local_addr.port, remote_addr.ip, remote_addr.port);
  established_socks[hashval] = new_connection;
  DEBUG("New Tcp Connection accepted! establish hash: %d\n" ,hashval);
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

  // 将建立了连接的socket放入内核 已建立连接哈希表中
  int hashval = cal_hash(local_addr.ip, local_addr.port, target_addr.ip, target_addr.port);
  established_socks[hashval] = sock;

  char *msg = create_packet_buf(sock->established_local_addr.port, sock->established_remote_addr.port, CLIENT_ISN, 0, DEFAULT_HEADER_LEN, DEFAULT_HEADER_LEN, SYN_FLAG_MASK, 1, 0, NULL, 0);

  tju_packet_t* pkt = create_packet(
      sock->established_local_addr.port, sock->established_remote_addr.port,
      CLIENT_ISN, 0 ,
      DEFAULT_HEADER_LEN, DEFAULT_HEADER_LEN,
      SYN_FLAG_MASK, 1, 0, NULL, 0);

  sendToLayer3(msg, DEFAULT_HEADER_LEN);
  sock->state = SYN_SENT;

  // 这里也不能直接建立连接 需要经过三次握手
  // 实际在linux中 connect调用后 会进入一个while循环
  // 循环跳出的条件是socket的状态变为ESTABLISHED 表面看上去就是 正在连接中 阻塞
  // 而状态的改变在别的地方进行 在我们这就是tju_handle_packet
  DEBUG("waiting 4 establish\n");


  while(sock->state != ESTABLISHED); // 等待对方将我方的状态转换成 ESTABLISHED

  // sock->state = ESTABLISHED;


  return 0;
}

int tju_send(tju_tcp_t* sock, const void *buffer, int len){
  // 这里当然不能直接简单地调用sendToLayer3
  char* data = malloc(len);
  memcpy(data, buffer, len);
  DEBUG("send to Client\n");

  char* msg;
  uint32_t seq = 464;
  uint16_t plen = DEFAULT_HEADER_LEN + len;

  // DEBUG("tju_send, 长度len:%d, 内容>%s<\n",len,data);

  msg = create_packet_buf(sock->established_local_addr.port, sock->established_remote_addr.port, seq, 0, 
                          DEFAULT_HEADER_LEN, plen, NO_FLAG, 1, 0, data, len);

  sendToLayer3(msg, plen);

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

int tju_handle_packet(tju_tcp_t* sock, char* pkt){

  uint32_t data_len = get_plen(pkt) - DEFAULT_HEADER_LEN;

  // 把收到的数据放到接受缓冲区
  while(pthread_mutex_lock(&(sock->recv_lock)) != 0); // 加锁

  if(sock->received_buf == NULL){
    sock->received_buf = malloc(data_len);
  }else {
    sock->received_buf = realloc(sock->received_buf, sock->received_len + data_len);
  }
  memcpy(sock->received_buf + sock->received_len, pkt + DEFAULT_HEADER_LEN, data_len);
  sock->received_len += data_len;


  int pkt_seq = get_seq(pkt);
  int pkt_src = get_src(pkt);
  int pkt_ack = get_ack(pkt);
  int pkt_plen = get_plen(pkt);
  int pkt_flag = get_flags(pkt);

  DEBUG("==> 开始 handle packet\n");

  switch (sock->state) {
    case LISTEN:

    if(pkt_flag==SYN_FLAG_MASK){
      // 收到 SYN_FLAG --> 连接第一次握手
      DEBUG("LISTEN 状态 收到 SYN_FLAG\n");
      tju_tcp_t * new_sock = (tju_tcp_t*)malloc(sizeof(tju_tcp_t));
      memcpy(new_sock, sock, sizeof(tju_tcp_t));

      tju_sock_addr remote_addr, local_addr;
      remote_addr.ip = inet_network("172.0.0.2"); // Listen 是 server 端的行为，所以远程地址就是 172.0.0.2 
      remote_addr.port = pkt_src;
      local_addr.ip = sock->bind_addr.ip;
      local_addr.port = sock->bind_addr.port;

      new_sock->established_local_addr = local_addr;
      new_sock->established_remote_addr = remote_addr;

      new_sock->state = SYN_RECV;
      push_q(sock->half_queue, new_sock);
      DEBUG("新 socket 准备好，可以push\n");

      tju_packet_t * ret_pack = create_packet(local_addr.port, remote_addr.port, SERVER_ISN, pkt_seq+1,
                                              DEFAULT_HEADER_LEN, DEFAULT_HEADER_LEN, ACK_FLAG_MASK | SYN_FLAG_MASK, 1 , 0 , NULL, 0);
      char *msg = packet_to_buf(ret_pack);
      sendToLayer3(msg, DEFAULT_HEADER_LEN);
      DEBUG("Server 接受到 client 的第一次握手，返回第二次握手\n");

    }else if(pkt_flag==ACK_FLAG_MASK){
      // 在 Listen 状态接收到 ACK 表明是第三次握手
      // 检查对方 Ack Number 是否正确
      if(pkt_ack==SERVER_ISN+1){
        tju_tcp_t * established_conn = pop_q(sock->half_queue);
        if(established_conn == NULL){
          DEBUG("Establshed 是 NULL\n");
        }
        established_conn->state = ESTABLISHED;
        push_q(sock->full_queue, established_conn);
        DEBUG("收到正确的 Ack 三次握手建立成功（至少server 端这么认为）\n");

        return 0;
      }else{
        DEBUG("client 端发送的 ack 错误，期待>%d<，接收到>%d<\n",SERVER_ISN+1, pkt_ack);
      }
    }else{
      DEBUG("当前为 LISTEN，接收到其他 flag 的报文，丢弃之\n");
    }
      break;
  
  }
  if(sock->state==LISTEN){
  }

  if(sock->state == SYN_SENT){
    if(pkt_ack == CLIENT_ISN + 1){
      tju_packet_t * new_pack = create_packet(sock->established_local_addr.port, sock->established_remote_addr.port, pkt_ack, pkt_seq+1,
                                              DEFAULT_HEADER_LEN, DEFAULT_HEADER_LEN, ACK_FLAG_MASK, 1, 0, NULL, 0);
      char *msg = packet_to_buf(new_pack);
      sendToLayer3(msg, DEFAULT_HEADER_LEN);
      DEBUG("Client 端发送了第三次握手，建立成功\n");
      sock->state = ESTABLISHED;
      return 0;
    }else{
      DEBUG("Client 端接收到了不正确的ack\n");
      return 0;
    }
  }

  pthread_mutex_unlock(&(sock->recv_lock)); // 解锁
  return 0;
}

int tju_close (tju_tcp_t* sock){
  return 0;
}



