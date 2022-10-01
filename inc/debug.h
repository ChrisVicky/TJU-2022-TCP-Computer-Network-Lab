#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "stdio.h"
#include "trace.h"

#include "pthread.h"
extern pthread_mutex_t thread_print_lock;
// Debug Message -- format:
// [File:Line:Func]():Msg
// if(0/1) Toggle
#define _debug_(...) \
    do {                 \
        if(1){           \
        pthread_mutex_lock(&thread_print_lock); \
        long time_time_print_debug=get_current_time();\
        fprintf(stderr, "(DEBUG) [%ld] [%-20s: %-4d: %-22s] \t", time_time_print_debug, __FILE__, \
                __LINE__, __func__); \
        fprintf(stderr, __VA_ARGS__);}             \
        pthread_mutex_unlock(&thread_print_lock); \
        } while (0)

#endif //__DEBUG_H__
