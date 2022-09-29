#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "stdio.h"
#include "trace.h"

// Debug Message -- format:
// [File:Line:Func]():Msg
// if(0/1) Toggle
#define DEBUG(...) \
    do {                 \
        if(1){           \
        int time_time_print_debug=get_current_time();\
        fprintf(stderr, "DEBUG [%d:%s:%d:%s](): ", time_time_print_debug, __FILE__, \
                __LINE__, __func__); \
        fprintf(stderr, __VA_ARGS__);}             \
        } while (0)

#endif //__DEBUG_H__
