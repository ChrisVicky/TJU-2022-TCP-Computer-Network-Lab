#include "tju_tcp.h"
#include <string.h>

void sleep_no_wake(int sec){  
    do{          
        sec =sleep(sec);
    }while(sec > 0);             
}

int main(int argc, char **argv) {
    // 开启仿真环境 
    startSimulation();

    tju_tcp_t* my_socket = tju_socket();
    
    tju_sock_addr target_addr;
    target_addr.ip = inet_network("172.17.0.3");
    target_addr.port = 1234;

    tju_connect(my_socket, target_addr);

    sleep_no_wake(8);

    for(int i=0;i<50;i++){
        char buf[16];
        sprintf(buf , "test message%d\n", i);
        tju_send(my_socket, buf, 16);
    }
    sleep_no_wake(100);

    return EXIT_SUCCESS;
}
