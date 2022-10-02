#include "tju_tcp.h"
#include <string.h>
#include <fcntl.h>

#define MIN_LEN 1000
#define EACHSIZE 10*MIN_LEN
#define MAXSIZE 50*MIN_LEN*MIN_LEN

// 全局变量
int t_times = 5000;

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

    int fd =  open("./rdt_send_file.txt",O_RDWR);
    if(-1 == fd) {
        return 1;
    }
    struct stat st;
    fstat(fd, &st);
    char* file_buf  = (char *)malloc(sizeof(char)*st.st_size);
    read(fd, (void *)file_buf, st.st_size );
    close(fd);

    for(int i=0; i<t_times; i++){
        char *buf = malloc(EACHSIZE);
        memset(buf, 0, EACHSIZE);
        if(i<10){
            sprintf(buf , "START####%d#", i);
        }
        else if(i<100){
            sprintf(buf , "START###%d#", i);
        }
        else if(i<1000){
            sprintf(buf , "START##%d#", i);
        }
        else if(i<10000){
            sprintf(buf , "START#%d#", i);
        }

        strcat(buf, file_buf);
        tju_send(my_socket, buf, EACHSIZE);
        free(buf);
    }

    free(file_buf);
        
    sleep_no_wake(100);

    return EXIT_SUCCESS;
}
