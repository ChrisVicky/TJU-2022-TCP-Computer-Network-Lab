#include "../inc/trace.h"
#include <pthread.h>


FILE *trace_file;
FILE *debug_file;
pthread_mutex_t trace_mutex = PTHREAD_MUTEX_INITIALIZER;
/**
* @brief  Touch a new Trace File 
*         while Removing the old one.
*/
void init_trace() {
  char filename[256];
  memset(filename, 0, sizeof(filename));
  char *tmp = getcwd(NULL, 256);
  _debug_(">%s<\n" ,tmp);
  char hostname[8];
  gethostname(hostname, 8);
  _debug_("host: %s\n",hostname);
  strcat(filename, hostname);
  _debug_("create file: %s\n",filename);
  strcat(filename, ".event.trace");
  _debug_("create file: %s\n",filename);
  remove(filename);
  trace_file = NULL;
  trace_file = fopen(filename, "w");
  if(trace_file==NULL){
    _debug_("Open File error\n");
    exit(1);
  }
  _debug_("create file: %s\n",filename);
  //BUG: 在手动测试：可以正常生成
  //BUG: 在脚本测试时，只能找到 client.event.trace 找不到 server.event.trace
}

long get_current_time(){
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec*1000000 + tv.tv_usec;
}


void trace_send(uint32_t seq, uint32_t ack, uint32_t flag, uint32_t len){
  _debug_("pthread_lock\n");
  pthread_mutex_lock(&trace_mutex);
  fprintf(trace_file, "[%ld] [SEND] [seq:%d ack:%d flag:%d length:%d]\n" ,get_current_time(), seq, ack, flag, len);
  _trace_("[%ld] [SEND] [seq:%d ack:%d flag:%d length:%d]\n" ,get_current_time(), seq, ack, flag, len);
  pthread_mutex_unlock(&trace_mutex);
  return;
}

void trace_recv(uint32_t seq, uint32_t ack, uint32_t flag, uint32_t len){
  _debug_("pthread_lock\n");
  pthread_mutex_lock(&trace_mutex);
  fprintf(trace_file, "[%ld] [RECV] [seq:%d ack:%d flag:%d length:%d]\n" ,get_current_time(), seq, ack, flag, len);
  _trace_("[%ld] [RECV] [seq:%d ack:%d flag:%d length:%d]\n" ,get_current_time(), seq, ack, flag, len);
  pthread_mutex_unlock(&trace_mutex);
  return;
}

void trace_cwnd(uint32_t type, uint32_t size){
  _debug_("pthread_lock\n");
  pthread_mutex_lock(&trace_mutex);
  size = size * 1375;
  fprintf(trace_file, "[%ld] [CWND] [type:%d size:%d]\n",get_current_time(), type, size);
  _trace_("[%ld] [CWND] [type:%d size:%d]\n",get_current_time(), type, size);
  pthread_mutex_unlock(&trace_mutex);
}

// Recv windows buffer size 
void trace_rwnd(uint32_t size){
  _debug_("pthread_lock\n");
  pthread_mutex_lock(&trace_mutex);
  size = size * 1375;
  fprintf(trace_file, "[%ld] [RWND] [size:%d]\n",get_current_time(),size);
  _trace_("[%ld] [RWND] [size:%d]\n",get_current_time(),size);
  pthread_mutex_unlock(&trace_mutex);
}

// sender windows size
void trace_swnd(uint32_t size){
  _debug_("pthread_lock\n");
  pthread_mutex_lock(&trace_mutex);
  size = size * 1375;
  fprintf(trace_file, "[%ld] [SWND] [size:%d]\n",get_current_time(),size);
  _trace_("[%ld] [SWND] [size:%d]\n",get_current_time(),size);
  pthread_mutex_unlock(&trace_mutex);
}

// Sample, Est, Deviation Rtt Timeout Interval Difference
void trace_rtts(double s, double e, double d, double t){
  // TODO: shoule be ms instead of s
  _debug_("pthread_lock\n");
  pthread_mutex_lock(&trace_mutex);
  fprintf(trace_file, "[%ld] [RTTS] [SampleRTT:%f EstablishedRTT:%f DeviationRtt:%f TimeoutInterval:%f]\n",get_current_time(), s,e,d,t);
  _trace_("[%ld] [RTTS] [SampleRTT:%f EstablishedRTT:%f DeviationRtt:%f TimeoutInterval:%f]\n",get_current_time(), s,e,d,t);
  pthread_mutex_unlock(&trace_mutex);
} 

// Recv Window diliver data to recv buff
void trace_delv(uint32_t seq, uint32_t size){
  _debug_("pthread_lock\n");
  pthread_mutex_lock(&trace_mutex);
  fprintf(trace_file, "[%ld] [DELV] [seq:%d size:%d]\n",get_current_time(),seq,size);
  _trace_("[%ld] [DELV] [seq:%d size:%d]\n",get_current_time(),seq,size);
  pthread_mutex_unlock(&trace_mutex);
}


