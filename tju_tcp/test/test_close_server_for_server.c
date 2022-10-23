#include "tju_tcp.h"
#include <string.h>

int TEST_TYPE;  //0 测试先后断开
                //1 测试同时断开

int main(int argc, char **argv) { 
    startSimulation();

    tju_tcp_t* my_server = tju_socket();
    
    tju_sock_addr bind_addr;
    bind_addr.ip = inet_network("127.17.0.3");
    bind_addr.port = 1234;

    tju_bind(my_server, bind_addr);

    tju_listen(my_server);

    tju_tcp_t* new_conn = tju_accept(my_server);


    if (argc==2){
        TEST_TYPE = atoi(argv[1]);
        if (TEST_TYPE==0){
            printf("[服务端] 测试双方先后断开连接的情况\n");
            while(new_conn->state != CLOSED){}
        }
        else if (TEST_TYPE==1){
            printf("[服务端] 测试双方同时断开连接的情况\n");
            tju_close(new_conn);
        }
        else{
            printf("[服务端] 未知测试类型\n");
        }
    }
    

    printf("STATE TRANSFORM TO CLOSED\n");
    return EXIT_SUCCESS;
}
