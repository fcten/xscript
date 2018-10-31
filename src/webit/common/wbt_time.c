/* 
 * File:   wbt_time.c
 * Author: Fcten
 *
 * Created on 2015年1月9日, 上午10:51
 */

#include "../webit.h"
#include "wbt_time.h"

/* 当前时间戳，单位毫秒 */
time_t wbt_cur_mtime;

wbt_status wbt_time_update() {
    struct timeval cur_utime;
    if (gettimeofday(&cur_utime, NULL) != 0) {
        return WBT_ERROR;
    }
    wbt_cur_mtime = 1000 * (time_t)cur_utime.tv_sec + (time_t)cur_utime.tv_usec / 1000;

    return WBT_OK;
}
