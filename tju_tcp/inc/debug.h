#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "stdio.h"
#include "trace.h"
#include <arpa/inet.h>
#include "global.h"
#include "pthread.h"
#define DEBUG_FLAG 0
extern pthread_mutex_t thread_print_lock;
#define OUTPUT_FILE 1 
extern FILE* debug_file;
// Debug Message -- format:
// [File:Line:Func]():Msg
// if(0/1) Toggle

// #define LOCKIT
#ifdef LOCKIT 

#define _print_(...) \
do{\
  if(DEBUG_FLAG){\
    fprintf(stderr, __VA_ARGS__);\
    fprintf(debug_file, __VA_ARGS__); \
  }\
}while(0)

#define _debug_(...) \
  do {                 \
    if(DEBUG_FLAG){           \
      pthread_mutex_lock(&thread_print_lock); \
      long time_time_print_debug=get_current_time();\
      fprintf(debug_file, "(DEBUG) [%ld] [%-20s: %-4d: %-22s] \t", time_time_print_debug, __FILE__, \
                __LINE__, __func__); \
      fprintf(debug_file, __VA_ARGS__);             \
      fprintf(stderr, "(DEBUG) [%ld] [%-20s: %-4d: %-22s] \t", time_time_print_debug, __FILE__, \
                __LINE__, __func__); \
      fprintf(stderr, __VA_ARGS__);             \
    pthread_mutex_unlock(&thread_print_lock);}\
  } while (0)

#define _debug_line_(...) \
  do {                 \
    if(DEBUG_FLAG){           \
      pthread_mutex_lock(&thread_print_lock); \
      long time_time_print_debug=get_current_time();\
      fprintf(debug_file, "(DEBUG) [%ld] [%-20s: %-4d: %-22s] \t------ ", time_time_print_debug, __FILE__, \
                __LINE__, __func__); \
      fprintf(debug_file, __VA_ARGS__);             \
      fprintf(debug_file, " ------\n"); \
      fprintf(stderr, "(DEBUG) [%ld] [%-20s: %-4d: %-22s] \t------ ", time_time_print_debug, __FILE__, \
                __LINE__, __func__); \
      fprintf(stderr, __VA_ARGS__);             \
      fprintf(stderr, " ------\n"); \
    pthread_mutex_unlock(&thread_print_lock);}\
  } while (0)

#define _line_(...) \
  do {                 \
    if(DEBUG_FLAG){           \
      pthread_mutex_lock(&thread_print_lock); \
      long time_time_print_debug=get_current_time();\
      fprintf(debug_file, "============================= ");\
      fprintf(debug_file, __VA_ARGS__);             \
      fprintf(debug_file, " =============================\n"); \
      fprintf(stderr, "============================= ");\
      fprintf(stderr, __VA_ARGS__);             \
      fprintf(stderr, " =============================\n"); \
    pthread_mutex_unlock(&thread_print_lock);}\
  } while (0)

#define _trace_(...) \
  do {                 \
    if(DEBUG_FLAG){           \
      pthread_mutex_lock(&thread_print_lock); \
      long time_time_print_debug=get_current_time();\
      fprintf(debug_file, "(TRACE) [%ld] [%-20s: %-4d: %-22s] \t", time_time_print_debug, __FILE__, \
                __LINE__, __func__); \
      fprintf(debug_file, __VA_ARGS__);             \
      fprintf(stderr, "(TRACE) [%ld] [%-20s: %-4d: %-22s] \t", time_time_print_debug, __FILE__, \
                __LINE__, __func__); \
      fprintf(stderr, __VA_ARGS__);             \
    pthread_mutex_unlock(&thread_print_lock);}\
  } while (0)

#else

#define _debug_(...) \
  do {                 \
    if(DEBUG_FLAG){           \
      long time_time_print_debug=get_current_time();\
      fprintf(debug_file, "(DEBUG) [%ld] [%-20s: %-4d: %-22s] \t", time_time_print_debug, __FILE__, \
                __LINE__, __func__); \
      fprintf(debug_file, __VA_ARGS__);             \
      fprintf(stderr, "(DEBUG) [%ld] [%-20s: %-4d: %-22s] \t", time_time_print_debug, __FILE__, \
                __LINE__, __func__); \
      fprintf(stderr, __VA_ARGS__);             \
      } \
  } while (0)

#define _debug_line_(...) \
  do {                 \
    if(DEBUG_FLAG){           \
      long time_time_print_debug=get_current_time();\
      fprintf(debug_file, "(DEBUG) [%ld] [%-20s: %-4d: %-22s] \t------ ", time_time_print_debug, __FILE__, \
                __LINE__, __func__); \
      fprintf(debug_file, __VA_ARGS__);             \
      fprintf(debug_file, " ------\n"); \
      fprintf(stderr, "(DEBUG) [%ld] [%-20s: %-4d: %-22s] \t------ ", time_time_print_debug, __FILE__, \
                __LINE__, __func__); \
      fprintf(stderr, __VA_ARGS__);             \
      fprintf(stderr, " ------\n"); \
      } \
  } while (0)

#define _line_(...) \
  do {                 \
    if(DEBUG_FLAG){           \
      long time_time_print_debug=get_current_time();\
      fprintf(debug_file, "============================= ");\
      fprintf(debug_file, __VA_ARGS__);             \
      fprintf(debug_file, " =============================\n"); \
      fprintf(stderr, "============================= ");\
      fprintf(stderr, __VA_ARGS__);             \
      fprintf(stderr, " =============================\n"); \
      } \
  } while (0)

#define _trace_(...) \
  do {                 \
    if(DEBUG_FLAG){           \
      long time_time_print_debug=get_current_time();\
      fprintf(debug_file, "(TRACE) [%ld] [%-20s: %-4d: %-22s] \t", time_time_print_debug, __FILE__, \
                __LINE__, __func__); \
      fprintf(debug_file, __VA_ARGS__);             \
      fprintf(stderr, "(TRACE) [%ld] [%-20s: %-4d: %-22s] \t", time_time_print_debug, __FILE__, \
                __LINE__, __func__); \
      fprintf(stderr, __VA_ARGS__);             \
    } \
  } while (0)

#define _print_(...) \
do{\
  if(DEBUG_FLAG){\
    fprintf(stderr, __VA_ARGS__);\
    fprintf(debug_file, __VA_ARGS__); \
  }\
}while(0)

#endif

#define _ip_port_(tju_addr) \
  do{ \
  uint32_t ip = tju_addr.ip; \
  uint32_t s1 = (ip & 0xFF000000) >> 24; \
  uint32_t s2 = (ip & 0x00FF0000) >> 16; \
  uint32_t s3 = (ip & 0x0000FF00) >> 8; \
  uint32_t s4 = (ip & 0x000000FF) >> 0; \
  ip = s1 | (s2<<8) | (s3<<16) | (s4<<24); \
  struct in_addr net; \
  net.s_addr = ip; \
  _debug_("addr %s:%d\n" ,inet_ntoa(net),tju_addr.port); \
}while(0)


#endif //__DEBUG_H__


