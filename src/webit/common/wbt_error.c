/* 
 * File:   wbt_error.c
 * Author: Fcten
 *
 * Created on 2018年10月18日, 下午4:25
 */

#include <stdarg.h>
#include <pthread.h>

#include "wbt_error.h"

wbt_status wbt_error(const char *fmt, ...) {
    va_list   args;
    char buff[256];
    int len;

    va_start(args, fmt);
    len = vsnprintf(buff, 256, fmt, args);
    va_end(args);

    fprintf(stdout, "%.*s", len, buff);
    
    return WBT_OK;
}

wbt_status _wbt_debug(const char *fmt, ...) {
    va_list   args;

    time_t now;
    struct tm *timenow;
    char time_str[18];
    char buff[256];
    int len;

    now = time(NULL);
    timenow = localtime(&now);
    strftime(time_str, 18, "%x %X", timenow);

    printf("[%s] [%lu] ", time_str, pthread_self());

    va_start(args, fmt);
    len = vsnprintf(buff, 256, fmt, args);
    va_end(args);

    fprintf(stdout, "%.*s", len, buff);

    printf("\n");
    
    return WBT_OK;
}