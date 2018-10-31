/* 
 * File:   wbt_error.c
 * Author: Fcten
 *
 * Created on 2018年10月18日, 下午4:25
 */

#include <stdarg.h>

#include "wbt_error.h"

wbt_status wbt_error(const char *fmt, ...) {
    va_list   args;

    va_start(args, fmt);
    fprintf(stdout, fmt, args);
    va_end(args);
    
    return WBT_OK;
}