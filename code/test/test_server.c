#include "tju_tcp.h"
#include <string.h>

int main(int argc, char **argv) { 
    startSimulation();

    tju_tcp_t* my_server = tju_socket();
    
    tju_sock_addr bind_addr;
    bind_addr.ip = inet_network("172.17.0.3");
    bind_addr.port = 1234;

    tju_bind(my_server, bind_addr);

    tju_listen(my_server);

    tju_tcp_t* new_conn = tju_accept(my_server);

    return EXIT_SUCCESS;
}
