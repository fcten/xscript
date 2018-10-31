/* 
 * File:   wbt_timer.h
 * Author: fcten
 *
 * Created on 2016年2月28日, 下午6:33
 */

#ifndef WBT_TIMER_H
#define	WBT_TIMER_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../webit.h"
#include "wbt_time.h"

typedef struct wbt_timer_s {
    wbt_status (*on_timeout)(struct wbt_timer_s *); /* 超时回调函数 */
    time_t timeout;                                 /* 事件超时时间 */
    unsigned int heap_idx;                          /* 在超时队列中的位置 */
} wbt_timer_t;

typedef struct wbt_heap_s wbt_heap_t;

/**
 * 用于通过 wbt_timer_t 指针获得完整结构体的指针
 * @ptr:                wbt_timer_t 结构体指针
 * @type:               想要获取的完整结构体类型
 * @member:         wbt_timer_t 成员在完整结构体中的名称
 */
#define wbt_timer_entry(ptr, type, member) \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

wbt_status wbt_timer_init(wbt_heap_t *wbt_timer);
wbt_status wbt_timer_exit(wbt_heap_t *wbt_timer);

wbt_status wbt_timer_add(wbt_heap_t *wbt_timer, wbt_timer_t *timer);
wbt_status wbt_timer_del(wbt_heap_t *wbt_timer, wbt_timer_t *timer);
wbt_status wbt_timer_mod(wbt_heap_t *wbt_timer, wbt_timer_t *timer);

time_t wbt_timer_process(wbt_heap_t *wbt_timer);

#ifdef	__cplusplus
}
#endif

#endif	/* WBT_TIMER_H */

