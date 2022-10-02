#include "../inc/tju_tcp.h"
#include <string.h>


int main(int argc, char **argv) {
  // 开启仿真环境 
  startSimulation();
  _debug_("----------------- SIMULATION STARTED ----------------------\n");

  tju_tcp_t* my_socket = tju_socket();
  // _debug_("my_tcp state %d\n", my_socket->state);

  tju_sock_addr target_addr;
  target_addr.ip = inet_network("172.17.0.3");
  target_addr.port = 1234;

  tju_connect(my_socket, target_addr);
  _debug_("----------------- SOCK CONNECTED ----------------------\n");
  _debug_("my_socket state %d\n", my_socket->state);      

  // uint32_t conn_ip;
  // uint16_t conn_port;

  // conn_ip = my_socket->established_local_addr.ip;
  // conn_port = my_socket->established_local_addr.port;
  // _debug_("my_socket established_local_addr ip %d port %d\n", conn_ip, conn_port);

  // conn_ip = my_socket->established_remote_addr.ip;
  // conn_port = my_socket->established_remote_addr.port;
  // _debug_("my_socket established_remote_addr ip %d port %d\n", conn_ip, conn_port);

  sleep(3);

  _debug_("----------------- TURN ON TJU RECV ----------------------\n");
  char buf[2021];
  tju_recv(my_socket, (void*)buf, 12);
  _debug_("client recv %s\n", buf);

  tju_recv(my_socket, (void*)buf, 10);
  _debug_("client recv %s\n", buf);

  tju_send(my_socket, "hello world", 12);
  tju_send(my_socket, "hello tju", 10);

  return EXIT_SUCCESS;
}
