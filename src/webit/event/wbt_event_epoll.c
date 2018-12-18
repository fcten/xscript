/* 
 * File:   wbt_event.c
 * Author: Fcten
 *
 * Created on 2014年9月1日, 上午11:54
 */

#include "wbt_event.h"
#include "../common/wbt_string.h"
#include "../common/wbt_heap.h"
#include "../common/wbt_time.h"
#include "../common/wbt_error.h"

extern wbt_atomic_t wbt_wating_to_exit;

/* 初始化事件队列 */
wbt_event_pool_t * wbt_event_init() {
    wbt_event_pool_t *pool = (wbt_event_pool_t *)wbt_malloc(sizeof(wbt_event_pool_t));
    if (!pool) {
        return NULL;
    }

    if (wbt_timer_init(&pool->timer) != WBT_OK) {
        goto init_error;
    }

    wbt_list_init(&pool->node.list);
    
    pool->node.pool = (wbt_event_t *)wbt_malloc( WBT_EVENT_LIST_SIZE * sizeof(wbt_event_t) );
    if( pool->node.pool == NULL ) {
        goto init_error;
    }

    pool->available = (wbt_event_t **)wbt_malloc( WBT_EVENT_LIST_SIZE * sizeof(wbt_event_t *) );
    if( pool->available == NULL ) {
        goto init_error;
    }

    pool->max = WBT_EVENT_LIST_SIZE;
    pool->top = WBT_EVENT_LIST_SIZE - 1;
    
    /* 把所有可用内存压入栈内 */
    int i;
    for(i=0 ; i<WBT_EVENT_LIST_SIZE ; i++) {
        pool->available[i] = pool->node.pool + i;
    }

    /* 初始化 EPOLL */
    pool->epoll_fd = epoll_create(WBT_MAX_EVENTS);
    if(pool->epoll_fd <= 0) {
        wbt_error("create epoll failed\n");
        goto init_error;
    }
    
    return pool;

init_error:
    wbt_event_cleanup(pool);
    wbt_free(pool);
    return NULL;
}

wbt_status wbt_event_resize(wbt_event_pool_t *pool) {
    wbt_event_pool_node_t *new_node = (wbt_event_pool_node_t *)wbt_calloc(sizeof(wbt_event_pool_node_t));
    if( new_node == NULL ) {
        return WBT_ERROR;
    }

    new_node->pool = (wbt_event_t *)wbt_malloc( WBT_EVENT_LIST_SIZE * sizeof(wbt_event_t) );
    if( new_node->pool == NULL ) {
        wbt_free(new_node);
        return WBT_ERROR;
    }
    
    wbt_event_t **tmp = (wbt_event_t **)wbt_realloc(pool->available, (pool->max + WBT_EVENT_LIST_SIZE) * sizeof(wbt_event_t *));
    if( tmp == NULL ) {
        wbt_free(new_node->pool);
        wbt_free(new_node);
        return WBT_ERROR;
    }
    pool->available = tmp;
    
    /* 将新的内存块加入到链表末尾 */
    wbt_list_add_tail(&new_node->list, &pool->node.list);

    /* 把新的可用内存压入栈内 */
    int i;
    for(i=0 ; i<WBT_EVENT_LIST_SIZE ; i++) {
        pool->available[++pool->top] = new_node->pool + i;
    }

    pool->max += WBT_EVENT_LIST_SIZE;
    
    return WBT_OK;
}

/* 添加事件 */
wbt_event_t * wbt_event_add(wbt_event_pool_t *pool, wbt_event_t *ev) {
    if( pool->top == 0 ) {
        /* 事件池已满,尝试动态扩充 */
        if(wbt_event_resize(pool) != WBT_OK) {
            wbt_error("event pool overflow\n");
            return NULL;
        }
    }
    
    //wbt_error("event add, fd %d, addr %p, max %u top %u\n", ev->fd ,ev, pool->max, pool->top);
    
    /* 添加到事件池内 */
    wbt_event_t *t = *(pool->available + pool->top);
    pool->top --;
    
    t->fd      = ev->fd;
    t->on_recv = ev->on_recv;
    t->on_send = ev->on_send;
    t->events  = ev->events;
    
    t->timer.heap_idx   = 0;
    t->timer.on_timeout = ev->timer.on_timeout;
    t->timer.timeout    = ev->timer.timeout;

    t->recv.buf = NULL;
    t->recv.len = 0;
    t->recv.offset = 0;

    t->send.buf = NULL;
    t->send.len = 0;
    t->send.offset = 0;

    t->ctx  = NULL;

    /* 注册epoll事件 */
    if(t->fd >= 0) {
        struct epoll_event epoll_ev;
        epoll_ev.events   = t->events;
        epoll_ev.data.ptr = t;
        if (epoll_ctl(pool->epoll_fd, EPOLL_CTL_ADD, t->fd, &epoll_ev) == -1) { 
            wbt_error("epoll_ctl:add failed\n");
            return NULL;
        }
    }
    
    /* 如果存在超时事件，添加到超时队列中 */
    if(wbt_timer_add(&pool->timer, &t->timer) != WBT_OK) {
        return NULL;
    }

    return t;
}

/* 删除事件 */
wbt_status wbt_event_del(wbt_event_pool_t *pool, wbt_event_t *ev) {
    /* 这种情况不应该发生，如果发生则说明进行了重复的删除操作 */
    if( pool->top + 1 >= pool->max ) {
        wbt_error("try to del event from empty pool\n");
        
        return WBT_ERROR;
    }
    
    //wbt_error("event del, fd %d, addr %p, max %u top %u\n", ev->fd ,ev, pool->max, pool->top);

    /* 从事件池中移除 */
    pool->top ++;
    pool->available[pool->top] = ev;

    /* 删除超时事件 */
    wbt_timer_del(&pool->timer, &ev->timer);
    
    /* 释放可能存在的事件数据缓存 */
    if (ev->recv.buf) {
        wbt_free(ev->recv.buf);
        ev->recv.buf = NULL;
        ev->recv.len = 0;
    }
    if (ev->send.buf) {
        wbt_free(ev->send.buf);
        ev->send.buf = NULL;
        ev->send.len = 0;
    }

    /* 注意： ev->ctx 需要应用自行在必要的时候释放 */

    /* 删除epoll事件 */
    if(ev->fd >= 0) {
        if (epoll_ctl(pool->epoll_fd, EPOLL_CTL_DEL, ev->fd, NULL) == -1) { 
            wbt_error("epoll_ctl:del failed\n");

            return WBT_ERROR;
        }
        // 不再自动 close fd
        //wbt_close_socket(ev->fd);
        //ev->fd = -1;
    }
    
    return WBT_OK;
}

/* 修改事件 */
wbt_status wbt_event_mod(wbt_event_pool_t *pool, wbt_event_t *ev) {
    //wbt_log_debug("event mod, fd %d, addr %p\n",ev->fd, ev);

    /* 修改epoll事件 */
    if(ev->fd >= 0) {
        struct epoll_event epoll_ev;
        epoll_ev.events   = ev->events;
        epoll_ev.data.ptr = ev;
        if (epoll_ctl(pool->epoll_fd, EPOLL_CTL_MOD, ev->fd, &epoll_ev) == -1) { 
            wbt_error("epoll_ctl:mod failed %d\n", ev->fd);

            return WBT_ERROR;
        }
    }

    /* 重新添加到超时队列中 */
    if(wbt_timer_mod(&pool->timer, &ev->timer) != WBT_OK) {
        return WBT_ERROR;
    }
    
    return WBT_OK;
}

wbt_status wbt_event_cleanup(wbt_event_pool_t *pool) {
    wbt_timer_exit(&pool->timer);

    while (!wbt_list_empty(&pool->node.list)) {
        wbt_event_pool_node_t *node = wbt_list_first_entry(&pool->node.list, wbt_event_pool_node_t, list);
        wbt_list_del(&node->list);
        wbt_free(node->pool);
        wbt_free(node);
    }

    if (pool->node.pool) {
        wbt_free(pool->node.pool);
        pool->node.pool = NULL;
    }

    if (pool->available) {
        wbt_free(pool->available);
        pool->available = NULL;
    }

    pool->max = pool->top = 0;

    return WBT_OK;
}

/* 事件循环 */
wbt_status wbt_event_dispatch(wbt_event_pool_t *pool) {;
    int timeout = 0, i = 0;
    struct epoll_event events[WBT_MAX_EVENTS];
    wbt_event_t *ev;
    
    while (!wbt_wating_to_exit) {
        int nfds = epoll_wait(pool->epoll_fd, events, WBT_MAX_EVENTS, timeout);

        /* 更新当前时间 */
        wbt_time_update();
        
        if (nfds == -1) {
            if (errno == EINTR) {
                // 被信号中断
                continue;
            } else {
                // 其他不可弥补的错误
                wbt_error("epoll_wait: %s\n", strerror(errno));
                return WBT_ERROR;
            }
        }

        for (i = 0; i < nfds; ++i) {
            ev = (wbt_event_t *)events[i].data.ptr;

            if ((events[i].events & WBT_EV_READ) && ev->on_recv) {
                if( ev->on_recv(ev) != WBT_OK ) {
                    wbt_error("call %p failed\n", ev->on_recv);
                    return WBT_ERROR;
                }
            }
            if ((events[i].events & WBT_EV_WRITE) && ev->on_send) {
                if( ev->on_send(ev) != WBT_OK ) {
                    wbt_error("call %p failed\n", ev->on_send);
                    return WBT_ERROR;
                }
            }
        }

        /* 检查并执行超时事件 */
        timeout = wbt_timer_process(&pool->timer);
    }

    return WBT_OK;
}

wbt_status wbt_event_wait(wbt_event_pool_t *pool, time_t timeout) {
    int i;
    struct epoll_event events[WBT_MAX_EVENTS];
    wbt_event_t *ev;

    int nfds = epoll_wait(pool->epoll_fd, events, WBT_MAX_EVENTS, timeout);

    wbt_time_update();
    
    if (nfds == -1) {
        if (errno == EINTR) {
            // 被信号中断
            //wbt_error("epoll_wait: signal\n");
            return WBT_OK;
        } else {
            // 其他不可弥补的错误
            wbt_error("epoll_wait: %s\n", strerror(errno));
            return WBT_ERROR;
        }
    }

    for (i = 0; i < nfds; ++i) {
        ev = (wbt_event_t *)events[i].data.ptr;

        if ((events[i].events & WBT_EV_READ) && ev->on_recv) {
            if( ev->on_recv(ev) != WBT_OK ) {
                wbt_error("call %p failed\n", ev->on_recv);
                return WBT_ERROR;
            }
        }
        if ((events[i].events & WBT_EV_WRITE) && ev->on_send) {
            if( ev->on_send(ev) != WBT_OK ) {
                wbt_error("call %p failed\n", ev->on_send);
                return WBT_ERROR;
            }
        }
    }

    return WBT_OK;
}