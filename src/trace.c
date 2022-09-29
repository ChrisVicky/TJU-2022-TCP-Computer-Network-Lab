#include "../inc/trace.h"


FILE *trace_file;

/**
* @brief  Touch a new Trace File 
*         while Removing the old one.
*/
void init_trace() {
  char hostname[256];
  gethostname(hostname, 256);
  strcat(hostname, ".event.trace");
  remove(hostname);
  trace_file = fopen(hostname, "w");
}

int get_current_time(){
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec*100 + tv.tv_usec/1000;
}
