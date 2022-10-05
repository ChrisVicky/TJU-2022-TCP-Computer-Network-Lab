#include "../inc/tju_tcp.h"
#include <string.h>
#include <signal.h>
#include <stdio.h>

#define MIN_LEN 1000
#define EACHSIZE 10*MIN_LEN
#define MAXSIZE 50*MIN_LEN*MIN_LEN

int t_times = 5000;
char allbuf[MAXSIZE] = {'\0'}; //设置全局变量

void fflushbeforeexit(int signo){
    printf("意外退出server\n");

    FILE *wfile;
    wfile = fopen("./rdt_recv_file.txt","w");
    if(wfile == NULL){
        printf("Error opening file\n");
        return;
    }
    size_t ret = fwrite(allbuf, sizeof(char), sizeof(allbuf), wfile);
    fclose(wfile);

    exit(0);
}

void sleep_no_wake(int sec){  
    do{        
        printf("Interrupted\n");
        sec =sleep(sec);
    }while(sec > 0);             
}

int main(int argc, char **argv) {
    signal(SIGHUP, fflushbeforeexit);
    signal(SIGINT, fflushbeforeexit);
    signal(SIGQUIT, fflushbeforeexit);

    // 开启仿真环境 
    startSimulation();
    
    tju_tcp_t* my_server = tju_socket();
    
    tju_sock_addr bind_addr;
    bind_addr.ip = inet_network("172.17.0.3");
    bind_addr.port = 1234;
    
    tju_bind(my_server, bind_addr);

    tju_listen(my_server);

    tju_tcp_t* new_conn = tju_accept(my_server);

    sleep_no_wake(8);

    int alllen = 0;
    int print_s = 0;
    while(alllen < t_times*EACHSIZE){
        char *buf = malloc(EACHSIZE);
        memset(buf, 0, EACHSIZE);
        int len = tju_recv(new_conn, (void*)buf, EACHSIZE);
        if(len<0){
            printf("tju_recv error!\n");
            break;
        }
        
        // strcat(allbuf, buf);
        memcpy(allbuf+alllen, buf, len);
        alllen += len;
        free(buf);
        
        if(print_s+EACHSIZE <= alllen){
            char tmpbuf[EACHSIZE] = {'\0'};
            memcpy(tmpbuf, allbuf+print_s, EACHSIZE);
            printf("[RDT TEST] server recv %s\n", tmpbuf);
            print_s += EACHSIZE;
        }
        fflush(stdout);
    }

    FILE *wfile;
    wfile = fopen("./rdt_recv_file.txt","w");
    if(wfile == NULL){
        printf("Error opening file\n");
        return -1;
    }
    size_t ret = fwrite(allbuf, sizeof(char), sizeof(allbuf), wfile);
    fclose(wfile);

    sleep_no_wake(100);
    
    return EXIT_SUCCESS;
}
