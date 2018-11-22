/* 
 * File:   wbt_event.h
 * Author: Fcten
 *
 * Created on 2014年9月1日, 上午11:13
 */

#ifndef __WBT_EVENT_H__
#define	__WBT_EVENT_H__

#ifdef	__cplusplus
extern "C" {
#endif

#define _GNU_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

//#include <openssl/ssl.h>

#include "../webit.h"
#include "../common/wbt_list.h"
#include "../common/wbt_heap.h"
#include "../common/wbt_timer.h"

#ifndef WIN32
#include "wbt_event_epoll.h"
#else
#include "wbt_event_select.h"
#endif

enum {
    WBT_PROTOCOL_UNKNOWN,
    WBT_PROTOCOL_HTTP,
    WBT_PROTOCOL_BMTP,
    WBT_PROTOCOL_WEBSOCKET,
    WBT_PROTOCOL_LENGTH
};

typedef struct {
    // 缓冲区
    void * buf;
    // 缓冲区内容长度
    size_t len;
    // 缓冲区已处理位置
    size_t offset;
    // 缓冲区可用长度
    size_t size;
} wbt_event_buf_t;

typedef struct wbt_event_s {
    wbt_timer_t timer;                              /* 超时事件 */
    wbt_socket_t fd;                                /* 事件句柄 */
    unsigned int events;                            /* 事件类型 */
    wbt_status (*on_recv)(struct wbt_event_s *);    /* 触发回调函数 */
    wbt_status (*on_send)(struct wbt_event_s *);    /* 触发回调函数 */
    /* 事件数据缓存 */
    wbt_event_buf_t recv;
    wbt_event_buf_t send;
    /* 自定义数据 */
    void * ctx;
} wbt_event_t;

typedef struct wbt_event_pool_node_s {
    wbt_event_t * pool;
    wbt_list_t list;
} wbt_event_pool_node_t;

typedef struct wbt_event_pool_s {
    wbt_heap_t timer;
    int epoll_fd;
    wbt_event_pool_node_t node;                 /* 一次性申请的大内存块 */
    wbt_event_t ** available;                   /* 栈，保存可用的内存块 */
    unsigned int max;                           /* 当前可以容纳的事件数 */
    unsigned int top;                           /* 当前栈顶的位置 */
} wbt_event_pool_t; 



/* 初始化事件池 */
wbt_event_pool_t * wbt_event_init();
/* 释放事件池 */
wbt_status wbt_event_cleanup(wbt_event_pool_t *pool);
/* 添加事件 */
wbt_event_t * wbt_event_add(wbt_event_pool_t *pool, wbt_event_t *ev);
/* 删除事件 */
wbt_status wbt_event_del(wbt_event_pool_t *pool, wbt_event_t *ev);
/* 修改事件 */
wbt_status wbt_event_mod(wbt_event_pool_t *pool, wbt_event_t *ev);
/* 事件循环 */
wbt_status wbt_event_dispatch(wbt_event_pool_t *pool);
wbt_status wbt_event_wait(wbt_event_pool_t *pool, time_t timeout);

#ifdef	__cplusplus
}
#endif

#endif	/* __WBT_EVENT_H__ */

