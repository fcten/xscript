/* 
 * File:   wbt_timer.c
 * Author: fcten
 *
 * Created on 2016年2月28日, 下午6:33
 */

#include "wbt_timer.h"
#include "wbt_error.h"
#include "wbt_heap.h"

wbt_status wbt_timer_init(wbt_heap_t *wbt_timer) {
    /* 初始化事件超时队列 */
    if(wbt_heap_init(wbt_timer, WBT_EVENT_LIST_SIZE) != WBT_OK) {
        wbt_error("create heap failed\n");

        return WBT_ERROR;
    }

    /* TODO 添加一个使用时间轮算法实现的子定时器 */

    return WBT_OK;
}

wbt_status wbt_timer_exit(wbt_heap_t *wbt_timer) {
    if(wbt_timer->size > 0) {
        wbt_timer_t *p = wbt_heap_get(wbt_timer);
        while(wbt_timer->size > 0) {
            /* 移除超时事件 */
            wbt_heap_delete(wbt_timer);
            /* 尝试调用回调函数 */
            if( p->on_timeout != NULL ) {
                p->on_timeout(p);
            }
            p = wbt_heap_get(wbt_timer);
        }
    }

    wbt_heap_destroy(wbt_timer);
    
    return WBT_OK;
}

wbt_status wbt_timer_add(wbt_heap_t *wbt_timer, wbt_timer_t *timer) {
    if( !timer->on_timeout ) {
        return WBT_OK;
    }

    if( !timer->heap_idx ) {
        return wbt_heap_insert(wbt_timer, timer);
    }
    
    return WBT_ERROR;
}

wbt_status wbt_timer_del(wbt_heap_t *wbt_timer, wbt_timer_t *timer) {
    if( timer->heap_idx ) {
        if( wbt_heap_remove(wbt_timer, timer->heap_idx) == WBT_OK ) {
            timer->heap_idx = 0;
            timer->on_timeout = NULL;
            timer->timeout = 0;
            return WBT_OK;
        } else {
            return WBT_ERROR;
        }
    }
    
    return WBT_OK;
}

wbt_status wbt_timer_mod(wbt_heap_t *wbt_timer, wbt_timer_t *timer) {
    if( !timer->heap_idx ) {
        return wbt_timer_add(wbt_timer, timer);
    } else {
        return wbt_heap_update(wbt_timer, timer->heap_idx);
    }
}

// 检查并执行所有的超时事件
// 返回下一次事件超时剩余时间，如果不存在则返回 0
extern time_t wbt_cur_mtime;

time_t wbt_timer_process(wbt_heap_t *wbt_timer) {
    if( wbt_timer->size == 0 ) {
        return -1;
    }

    wbt_timer_t *p = wbt_heap_get(wbt_timer);
    while(p && p->timeout <= wbt_cur_mtime ) {
        /* 移除超时事件 */
        wbt_heap_delete(wbt_timer);
        //p->heap_idx = 0;
        /* 尝试调用回调函数 */
        if(p->on_timeout != NULL) {
            p->on_timeout(p);
        }
        p = wbt_heap_get(wbt_timer);
    }

    if(wbt_timer->size > 0) {
        return p->timeout - wbt_cur_mtime;
    } else {
        return -1;
    }
}
