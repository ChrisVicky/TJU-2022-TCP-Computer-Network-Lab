#include "pthread.h"
#include "stdio.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int num;
int main(){

  while(1){
    sleep(1);

    time_t seed;
    srand((unsigned)time(&seed));
    int t = rand()%100;
    printf("%d\n" ,t);
  }

}



