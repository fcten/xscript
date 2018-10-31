/* 
 * File:   wbt_event.c
 * Author: Fcten
 *
 * Created on 2014年9月1日, 上午11:54
 */

#include "wbt_event.h"
#include "../common/wbt_string.h"
#include "../common/wbt_heap.h"
#include "../common/wbt_log.h"
#include "../common/wbt_connection.h"
#include "../common/wbt_module.h"
#include "../common/wbt_time.h"
#include "../common/wbt_config.h"
#include "../common/wbt_file.h"

wbt_module_t wbt_module_event = {
    wbt_string("event"),
    wbt_event_init,
    wbt_event_exit
};

extern wbt_atomic_t wbt_wating_to_exit;

wbt_fd_t wbt_lock_accept;

/* 事件队列 */
static wbt_event_pool_t wbt_events;

static wbt_event_t **event_index;
static unsigned int nevents;

static fd_set read_fd_set;
static fd_set write_fd_set;

static void wbt_event_repair_fd_sets();

/* 初始化事件队列 */
wbt_status wbt_event_init() {
    wbt_list_init(&wbt_events.node.list);
    
    wbt_events.node.pool = wbt_malloc( WBT_EVENT_LIST_SIZE * sizeof(wbt_event_t) );
    if( wbt_events.node.pool == NULL ) {
        return WBT_ERROR;
    }

    wbt_events.available = wbt_malloc( WBT_EVENT_LIST_SIZE * sizeof(wbt_event_t *) );
    if( wbt_events.available == NULL ) {
         return WBT_ERROR;
    }

    event_index = wbt_malloc( FD_SETSIZE * sizeof( wbt_event_t * ) );
    if( event_index == NULL ) {
        return WBT_ERROR;
    }
    nevents = 0;

    wbt_events.max = WBT_EVENT_LIST_SIZE;
    wbt_events.top = WBT_EVENT_LIST_SIZE - 1;
    
    /* 把所有可用内存压入栈内 */
    int i;
    for(i=0 ; i<WBT_EVENT_LIST_SIZE ; i++) {
        wbt_events.available[i] = wbt_events.node.pool + i;
    }

    /* 根据端口号创建锁文件 */
    wbt_str_t lock_file;
    lock_file.len = sizeof("/tmp/.wbt_accept_lock_00000");
    lock_file.str = wbt_malloc(lock_file.len);
    if (lock_file.str == NULL) {
        return WBT_ERROR;
    }
    snprintf(lock_file.str, lock_file.len, "/tmp/.wbt_accept_lock_%d", wbt_conf.listen_port);

    if ((wbt_lock_accept = wbt_lock_create(lock_file.str)) <= 0) {
        wbt_log_add("create lock file failed\n");
        wbt_free(lock_file.str);
        return WBT_ERROR;
    }
    wbt_free(lock_file.str);

    return WBT_OK;
}

wbt_status wbt_event_exit() {
    // 可以在这里删除所有残留事件
    
    return WBT_OK;
}

/*
void wbt_event_print() {
    printf( "events: " );
    for( unsigned int i = 0; i < nevents; ++i ) {
        printf( "[%d] ", event_index[i]->fd );
    }
    printf( "\n" );
    printf( "read_fd_set: " );
    for( unsigned int i = 0; i < read_fd_set.fd_count; i++ ) {
        printf( "[%d] ", read_fd_set.fd_array[i] );
    }
    printf( "\n" );
    printf( "write_fd_set: " );
    for( unsigned int i = 0; i < write_fd_set.fd_count; i++ ) {
        printf( "[%d] ", write_fd_set.fd_array[i] );
    }
    printf( "\n" );
}
*/

wbt_status wbt_event_resize() {
    wbt_event_pool_node_t *new_node = wbt_calloc(sizeof(wbt_event_pool_node_t));
    if( new_node == NULL ) {
        return WBT_ERROR;
    }

    new_node->pool = wbt_malloc( WBT_EVENT_LIST_SIZE * sizeof(wbt_event_t) );
    if( new_node->pool == NULL ) {
        wbt_free(new_node);
        return WBT_ERROR;
    }
    
    void * tmp = wbt_realloc(wbt_events.available, (wbt_events.max + WBT_EVENT_LIST_SIZE) * sizeof(wbt_event_t *));
    if( tmp == NULL ) {
        wbt_free(new_node->pool);
        wbt_free(new_node);
        return WBT_ERROR;
    }
    wbt_events.available = tmp;
    
    /* 将新的内存块加入到链表末尾 */
    wbt_list_add_tail(&new_node->list, &wbt_events.node.list);

    /* 把新的可用内存压入栈内 */
    int i;
    for(i=0 ; i<WBT_EVENT_LIST_SIZE ; i++) {
        wbt_events.available[++wbt_events.top] = new_node->pool + i;
    }

    wbt_events.max += WBT_EVENT_LIST_SIZE;
    
    return WBT_OK;
}

/* 添加事件 */
wbt_event_t * wbt_event_add(wbt_event_t *ev) {
    if( wbt_events.top == 0 ) {
        /* 事件池已满,尝试动态扩充 */
        if(wbt_event_resize() != WBT_OK) {
            wbt_log_add("event pool overflow\n");

            return NULL;
        } else {
            wbt_log_add("event pool resize to %u\n", wbt_events.max);
        }
    }
    
    //wbt_log_debug( "event add, fd %d, type %d, %u events.\n", ev->fd, ev->events, wbt_events.max - wbt_events.top );

    /* 添加到事件池内 */
    wbt_event_t *t = *(wbt_events.available + wbt_events.top);
    wbt_events.top --;
    
    t->fd      = ev->fd;
    t->on_recv = ev->on_recv;
    t->on_send = ev->on_send;
    t->events  = ev->events;
    
    t->timer.heap_idx   = 0;
    t->timer.on_timeout = ev->timer.on_timeout;
    t->timer.timeout    = ev->timer.timeout;

    t->buff = NULL;
    t->buff_len = 0;
    t->data = NULL;
    t->ctx  = NULL;
    
    t->protocol = 0;
    t->ssl = NULL;
    
    t->is_exit = 0;

    /* 注册select事件 */
    if (((t->events & WBT_EV_READ) && read_fd_set.fd_count >= FD_SETSIZE) ||
        ((t->events & WBT_EV_WRITE) && write_fd_set.fd_count >= FD_SETSIZE)) {
        wbt_log_debug("maximum number of descriptors supported by select() is %d", FD_SETSIZE);
        return NULL;
    }

    if (t->events & WBT_EV_READ) {
        FD_SET(t->fd, &read_fd_set);
    }
    if (t->events & WBT_EV_WRITE) {
        FD_SET(t->fd, &write_fd_set);
    }

    event_index[nevents] = t;
    nevents++;
    
    /* 如果存在超时事件，添加到超时队列中 */
    if(wbt_timer_add(&t->timer) != WBT_OK) {
        return NULL;
    }

    return t;
}

/* 删除事件 */
wbt_status wbt_event_del(wbt_event_t *ev) {
    /* 这种情况不应该发生，如果发生则说明进行了重复的删除操作 */
    if( wbt_events.top+1 >= wbt_events.max ) {
        wbt_log_debug("try to del event from empty pool\n");
        
        return WBT_ERROR;
    }
    
    //wbt_log_debug( "event del, fd %d, type %d, %u events\n", ev->fd, ev->events, wbt_events.max - wbt_events.top - 2 );

    /* 从事件池中移除 */
    wbt_events.top ++;
    wbt_events.available[wbt_events.top] = ev;

    /* 删除超时事件 */
    wbt_timer_del(&ev->timer);
    
    /* 释放可能存在的事件数据缓存 */
    wbt_free(ev->buff);
    ev->buff = NULL;
    ev->buff_len = 0;

    wbt_free(ev->data);
    ev->data = NULL;
    
    ev->protocol = 0;
    
    ev->is_exit = 0;

    /* 删除select事件 */
    if (ev->events & WBT_EV_READ) {
        FD_CLR(ev->fd, &read_fd_set);

    }
    if (ev->events & WBT_EV_WRITE) {
        FD_CLR(ev->fd, &write_fd_set);
    }

    // TODO 可以在 wbt_ebent 中增加 idx，并将 event_index 放置到 wbt_events 中
    for( unsigned int i = 0 ; i < nevents; ++i ) {
        if( event_index[i] == ev ) {
            event_index[i] = event_index[--nevents];
            break;
        }
    }

    ev->fd = -1;
    
    return WBT_OK;
}

/* 修改事件 */
wbt_status wbt_event_mod(wbt_event_t *ev) {
    //wbt_log_debug( "event mod, fd %d, type %d\n", ev->fd, ev->events );

    /* 修改select事件 */
    FD_CLR(ev->fd, &read_fd_set);
    FD_CLR(ev->fd, &write_fd_set);

    if (((ev->events & WBT_EV_READ) && read_fd_set.fd_count >= FD_SETSIZE) ||
        ((ev->events & WBT_EV_WRITE) && write_fd_set.fd_count >= FD_SETSIZE)) {
        wbt_log_debug("maximum number of descriptors supported by select() is %d", FD_SETSIZE);
        return WBT_ERROR;
    }

    if (ev->events & WBT_EV_READ) {
        FD_SET(ev->fd, &read_fd_set);
    }
    if (ev->events & WBT_EV_WRITE) {
        FD_SET(ev->fd, &write_fd_set);
    }

    /* 重新添加到超时队列中 */
    if(wbt_timer_mod(&ev->timer) != WBT_OK) {
        return WBT_ERROR;
    }
    
    return WBT_OK;
}

wbt_status wbt_event_cleanup();

/* 事件循环 */
wbt_status wbt_event_dispatch() {
    time_t timeout = 0;
    unsigned int i = 0;
    wbt_event_t *ev;

    fd_set work_read_fd_set;
    fd_set work_write_fd_set;

    
    while (!wbt_wating_to_exit) {
        int nfds;
        struct timeval tv;
        tv.tv_sec = (long)(timeout / 1000);
        tv.tv_usec = (long)((timeout % 1000) * 1000);

        if (read_fd_set.fd_count || write_fd_set.fd_count) {
            FD_ZERO(&work_read_fd_set);
            FD_ZERO(&work_write_fd_set);

            for (i = 0; i < read_fd_set.fd_count; i++) {
                FD_SET(read_fd_set.fd_array[i], &work_read_fd_set);
            }

            for (i = 0; i < write_fd_set.fd_count; i++) {
                FD_SET(write_fd_set.fd_array[i], &work_write_fd_set);
            }

            //wbt_log_debug( "work_read_fd_set: %d, work_write_fd_set: %d\n", work_read_fd_set.fd_count, work_write_fd_set.fd_count );
            nfds = select(0, &work_read_fd_set, &work_write_fd_set, NULL, &tv);
            //wbt_log_debug( "nfds: %d\n", nfds );

        } else {
            /*
            * Winsock select() requires that at least one descriptor set must be
            * be non-null, and any non-null descriptor set must contain at least
            * one handle to a socket.  Otherwise select() returns WSAEINVAL.
            */

            wbt_msleep((int)timeout);

            nfds = 0;
        }

        wbt_err_t err = (nfds == -1) ? wbt_socket_errno : 0;

        /* 更新当前时间 */
        wbt_time_update();
        
        if (err) {
            if (err == WSAENOTSOCK) {
                wbt_event_repair_fd_sets();
            }
        }

        //wbt_log_debug("%d event happened.\n",nfds);

        if (nfds > 0) {
            for( i = 0; i < nevents; ++i ) {
                ev = event_index[i];

                if ((ev->events & WBT_EV_READ) && ev->on_recv && FD_ISSET(ev->fd, &work_read_fd_set)) {
                    if (ev->on_recv(ev) != WBT_OK) {
                        wbt_error("call %p failed\n", ev->on_recv);
                        return WBT_ERROR;
                    }
                }
                if ((ev->events & WBT_EV_WRITE) && ev->on_send && FD_ISSET(ev->fd, &work_write_fd_set)) {
                    if (ev->on_send(ev) != WBT_OK) {
                        wbt_error("call %p failed\n", ev->on_send);
                        return WBT_ERROR;
                    }
                }
            }

        }

        /* 检查并执行超时事件 */
        timeout = wbt_timer_process();
        //wbt_log_debug( "time_out: %ld\n", timeout );
    }

    return WBT_OK;
}

static void wbt_event_repair_fd_sets() {
    int           n;
    u_int         i;
    int           len;
    wbt_err_t     err;
    wbt_socket_t  s;

    for (i = 0; i < read_fd_set.fd_count; i++) {

        s = read_fd_set.fd_array[i];
        len = sizeof(int);

        if (getsockopt(s, SOL_SOCKET, SO_TYPE, (char *)&n, &len) == -1) {
            err = wbt_socket_errno;

            //wbt_log_debug("invalid descriptor #%d in read fd_set\n", s);

            FD_CLR(s, &read_fd_set);
        }
    }

    for (i = 0; i < write_fd_set.fd_count; i++) {

        s = write_fd_set.fd_array[i];
        len = sizeof(int);

        if (getsockopt(s, SOL_SOCKET, SO_TYPE, (char *)&n, &len) == -1) {
            err = wbt_socket_errno;

            //wbt_log_debug("invalid descriptor #%d in write fd_set\n", s);

            FD_CLR(s, &write_fd_set);
        }
    }
}