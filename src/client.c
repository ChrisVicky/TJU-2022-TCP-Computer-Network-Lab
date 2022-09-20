#include "../inc/tju_tcp.h"
#include <string.h>


int main(int argc, char **argv) {
  // 开启仿真环境 
  startSimulation();
  printf("Simulation Started\n");

  tju_tcp_t* my_socket = tju_socket();
  // printf("my_tcp state %d\n", my_socket->state);

  tju_sock_addr target_addr;
  target_addr.ip = inet_network("172.17.0.3");
  target_addr.port = 1234;

  printf("准备开始 connect\n");
  tju_connect(my_socket, target_addr);
  printf("my_socket state %d\n", my_socket->state);      

  // uint32_t conn_ip;
  // uint16_t conn_port;

  // conn_ip = my_socket->established_local_addr.ip;
  // conn_port = my_socket->established_local_addr.port;
  // printf("my_socket established_local_addr ip %d port %d\n", conn_ip, conn_port);

  // conn_ip = my_socket->established_remote_addr.ip;
  // conn_port = my_socket->established_remote_addr.port;
  // printf("my_socket established_remote_addr ip %d port %d\n", conn_ip, conn_port);

  sleep(3);

  tju_send(my_socket, "hello world", 12);
  tju_send(my_socket, "hello tju", 10);

  char buf[2021];
  tju_recv(my_socket, (void*)buf, 12);
  printf("client recv %s\n", buf);

  tju_recv(my_socket, (void*)buf, 10);
  printf("client recv %s\n", buf);

  return EXIT_SUCCESS;
}
