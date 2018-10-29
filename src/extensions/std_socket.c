
#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "../common/str.h"
#include "../common/fun.h"
#include "../interpreter/coroutine.h"
#include "std_coroutine.h"

#define WBT_MAX_PROTO_BUF_LEN (64 * 1024)

extern time_t wbt_cur_mtime;

typedef struct {
    // 主线程
    lgx_vm_t *vm;
    // 工作线程
    wbt_thread_pool_t *pool;
    // on_request
    lgx_fun_t *on_request;
} lgx_server_t;

wbt_status std_socket_on_close(wbt_event_t *ev) {
    //wbt_log_debug("connection %d close.",ev->fd);
    
    lgx_server_t *server = ev->ctx;

    //if( wbt_module_on_close(ev) != WBT_OK ) {
        // 似乎并不能做什么
    //}

    wbt_event_del(server->vm->events, ev);
    
    //wbt_connection_count --;

    return WBT_OK;
}

int std_socket_on_yield(lgx_vm_t *vm) {
    lgx_co_t *co = vm->co_running;
    wbt_event_t *ev = co->ctx;

    if (co->status != CO_DIED) {
        return 0;
    }

    // 读取返回值
    if (vm->regs[1].type != T_STRING) {
        std_socket_on_close(ev);
        return WBT_OK;
    }
    lgx_str_t *str = vm->regs[1].v.str;

    // 生成响应
    ev->buff = xmalloc(str->length);
    if (!ev->buff) {
        std_socket_on_close(ev);
        return WBT_OK;
    }
    memcpy(ev->buff, str->buffer, str->length);
    ev->buff_len = str->length;
    ev->buff_offset = 0;

    // 修改事件
    ev->events = WBT_EV_WRITE | WBT_EV_ET;
    ev->timer.timeout = wbt_cur_mtime + 15 * 1000;

    if(wbt_event_mod(vm->events, ev) != WBT_OK) {
        std_socket_on_close(ev);
        return WBT_OK;
    }

    return 0;
}

wbt_status std_socket_on_recv(wbt_event_t *ev) {
    //printf("[%lu] on_recv %d \n", pthread_self(), ev->fd);

    lgx_server_t *server = ev->ctx;

    if (ev->buff == NULL) {
        ev->buff = xmalloc(WBT_MAX_PROTO_BUF_LEN);
        ev->buff_len = 0;
        
        if( ev->buff == NULL ) {
            /* 内存不足 */
            std_socket_on_close(ev);
            return WBT_OK;
        }
    }
    
    if (ev->buff_len >= WBT_MAX_PROTO_BUF_LEN) {
        /* 请求长度超过限制
         */
        //wbt_log_add("connection close: request length exceeds limitation\n");
        std_socket_on_close(ev);
        return WBT_OK;
    }

    int nread = recv(ev->fd, (unsigned char *)ev->buff + ev->buff_len,
        WBT_MAX_PROTO_BUF_LEN - ev->buff_len, 0);

    if (nread < 0) {
        wbt_err_t err = wbt_socket_errno;
        if (err == WBT_EAGAIN) {
            // 当前缓冲区已无数据可读
        } else if (err == WBT_ECONNRESET) {
            // 对方发送了RST
            //wbt_log_add("connection close: connection reset by peer\n");
            std_socket_on_close(ev);
            return WBT_OK;
        } else {
            // 其他不可弥补的错误
            //wbt_log_add("connection close: %d\n", err);
            std_socket_on_close(ev);
            return WBT_OK;
        }
    } else if (nread == 0) {
        //wbt_log_add("connection close: connection closed by peer\n");
        std_socket_on_close(ev);
        return WBT_OK;
    }
    
    ev->buff_len += nread;

    // 如果数据读完
    //printf("%.*s", (int)ev->buff_len, (char *)ev->buff);
    xfree(ev->buff);
    ev->buff = NULL;

    // 调用 xscript
    lgx_co_t *co = lgx_co_create(server->vm, server->on_request);
    co->on_yield = std_socket_on_yield;
    co->ctx = ev;

    return WBT_OK;
}

wbt_status std_socket_on_send(wbt_event_t *ev) {
    //printf("[%lu] on_send %d\n", pthread_self(), ev->fd);
    lgx_server_t *server = ev->ctx;

    int on = 1;
    /* TODO 发送大文件时，使用 TCP_CORK 关闭 Nagle 算法保证网络利用率 */
    //setsockopt( ev->fd, SOL_TCP, TCP_CORK, &on, sizeof ( on ) );
    /* 测试 keep-alive 性能时，使用 TCP_NODELAY 立即发送小数据，否则会阻塞 40 毫秒 */
    setsockopt(ev->fd, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on));

    if (ev->buff_len > ev->buff_offset) {
        int nwrite = send(ev->fd, ev->buff + ev->buff_offset,
            ev->buff_len - ev->buff_offset, 0);

        if (nwrite <= 0) {
            if( wbt_socket_errno == WBT_EAGAIN ) {
                return WBT_OK;
            } else {
                std_socket_on_close(ev);
                return WBT_OK;
            }
        }

        ev->buff_offset += nwrite;

        //wbt_log_debug("%d send, %d remain.\n", nwrite, http->response.len - http->resp_offset);
        if (ev->buff_len > ev->buff_offset) {
            /* 尚未发送完，缓冲区满 */
            return WBT_OK;
        }
    }

    // 发送完毕
    //std_socket_on_close(ev);
    xfree(ev->buff);
    ev->buff = NULL;
    ev->buff_len = 0;

    // 修改事件
    ev->events = WBT_EV_READ | WBT_EV_ET;
    ev->timer.timeout = wbt_cur_mtime + 15 * 1000;

    if (wbt_event_mod(server->vm->events, ev) != WBT_OK) {
        std_socket_on_close(ev);
        return WBT_OK;
    }

    return WBT_OK;
}

wbt_status std_socket_on_timeout(wbt_timer_t *timer) {
    wbt_event_t *ev = wbt_timer_entry(timer, wbt_event_t, timer);
    //printf("[%lu] on_timeout %d\n", pthread_self(), ev->fd);

    return std_socket_on_close(ev);
}

wbt_status std_socket_on_accept(wbt_event_t *ev) {
    lgx_server_t *server = ev->ctx;

    struct sockaddr_in remote;
    int addrlen = sizeof(remote);
    wbt_socket_t conn_sock;
#ifdef WBT_USE_ACCEPT4
    while((int)(conn_sock = accept4(ev->fd,(struct sockaddr *) &remote, (socklen_t *)&addrlen, SOCK_NONBLOCK)) >= 0) {
#else
    while((int)(conn_sock = accept(ev->fd,(struct sockaddr *) &remote, (int *)&addrlen)) >= 0) {
        wbt_nonblocking(conn_sock); 
#endif
        /* inet_ntoa 在 linux 下使用静态缓存实现，无需释放 */
        //wbt_log_add("%s\n", inet_ntoa(remote.sin_addr));
        //printf("[%lu] on_accept %d\n", pthread_self(),  conn_sock);

        if (server->pool) {
            wbt_thread_t *worker = wbt_thread_get(server->pool);
            if (wbt_thread_send(worker, conn_sock) != 0) {
                wbt_close_socket(conn_sock);
                continue;
            }
        } else {
            wbt_event_t *p_ev, tmp_ev;
            tmp_ev.timer.on_timeout = std_socket_on_timeout;
            tmp_ev.timer.timeout    = wbt_cur_mtime + 15 * 1000;
            tmp_ev.on_recv = std_socket_on_recv;
            tmp_ev.on_send = std_socket_on_send;
            tmp_ev.events  = WBT_EV_READ | WBT_EV_ET;
            tmp_ev.fd      = conn_sock;

            if((p_ev = wbt_event_add(server->vm->events, &tmp_ev)) == NULL) {
                wbt_close_socket(conn_sock);
                continue;
            }

            p_ev->ctx = server;
        }
        
        //p_ev->ctx= server;

        /*
        wbt_connection_count ++;

        if( ev->fd == wbt_secure_fd && wbt_ssl_on_conn(p_ev) != WBT_OK ) {
            wbt_on_close(p_ev);
            continue;
        }*/
    }

    if (conn_sock == -1) {
        wbt_err_t err = wbt_socket_errno;

        if (err == WBT_EAGAIN) {
            return WBT_OK;
        } else if (err == WBT_ECONNABORTED) {
            //wbt_log_add("accept failed\n");

            return WBT_ERROR;
        } else if (err == WBT_EMFILE || err == WBT_ENFILE) {
            //wbt_log_add("accept failed\n");

            return WBT_ERROR;
        }
    }

    return WBT_OK;
}

extern wbt_atomic_t wbt_wating_to_exit;

void* worker(void *args) {
    wbt_thread_t *thread = args;
    lgx_server_t *master = thread->ctx;

    lgx_vm_t vm;
    lgx_vm_init(&vm, master->vm->bc);

    lgx_co_suspend(&vm);

    lgx_server_t server;
    server.vm = &vm;
    server.on_request = master->on_request;
    server.pool = NULL;

    time_t timeout = 0;

    while (!wbt_wating_to_exit) {
        lgx_vm_execute(&vm);

        if (!lgx_co_has_task(&vm)) {
            // 全部协程均执行完毕
            break;
        }

        if (lgx_co_has_ready_task(&vm)) {
            lgx_co_schedule(&vm);
        } else {
            timeout = wbt_timer_process(&vm.events->timer);
            wbt_event_wait(vm.events, timeout);
            wbt_time_update();
            timeout = wbt_timer_process(&vm.events->timer);

            wbt_event_t *p_ev, tmp_ev;
            tmp_ev.on_recv = std_socket_on_recv;
            tmp_ev.on_send = std_socket_on_send;
            tmp_ev.events  = WBT_EV_READ | WBT_EV_ET;
            tmp_ev.timer.on_timeout = std_socket_on_timeout;
            tmp_ev.timer.timeout    = wbt_cur_mtime + 15 * 1000;
            while (wbt_thread_recv(thread, &tmp_ev.fd) == 0) {
                if((p_ev = wbt_event_add(vm.events, &tmp_ev)) == NULL) {
                    wbt_close_socket(tmp_ev.fd);
                    continue;
                }

                p_ev->ctx = &server;
            }
        }
    }

    lgx_vm_cleanup(&vm);

    return NULL;
}

int std_socket_server_create(void *p) {
    lgx_vm_t *vm = p;

    unsigned base = vm->regs[0].v.fun->stack_size;
    long long port = vm->regs[base+4].v.l;
    lgx_fun_t *fun = vm->regs[base+5].v.fun;

    // 创建监听端口并加入事件循环
    wbt_socket_t fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd <= 0) {
        //wbt_log_add("create socket failed\n");
        return lgx_ext_return_false(vm);
    }

    /* 把监听socket设置为非阻塞方式 */
    if( wbt_nonblocking(fd) == -1 ) {
        //wbt_log_add("set nonblocking failed\n");
        wbt_close_socket(fd);
        return lgx_ext_return_false(vm);
    }

    /* 在重启程序以及进行热更新时，避免 TIME_WAIT 和 CLOSE_WAIT 状态的连接导致 bind 失败 */
    int on = 1; 
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) != 0) {  
        //wbt_log_add("set SO_REUSEADDR failed\n");
        wbt_close_socket(fd);
        return lgx_ext_return_false(vm);
    }

    /* bind & listen */    
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    if(bind(fd, (const struct sockaddr*)&sin, sizeof(sin)) != 0) {
        //wbt_log_add("bind failed\n");
        wbt_close_socket(fd);
        return lgx_ext_return_false(vm);
    }

    if(listen(fd, WBT_CONN_BACKLOG) != 0) {
        //wbt_log_add("listen failed\n");
        wbt_close_socket(fd);
        return lgx_ext_return_false(vm);
    }

    wbt_event_t ev, *p_ev;
    memset(&ev, 0, sizeof(wbt_event_t));

    ev.on_recv = std_socket_on_accept;
    ev.on_send = NULL;
    ev.events = WBT_EV_READ | WBT_EV_ET;
    ev.fd = fd;

    if((p_ev = wbt_event_add(vm->events, &ev)) == NULL) {
        wbt_close_socket(fd);
        return lgx_ext_return_false(vm);
    }

    lgx_server_t *server = xmalloc(sizeof(lgx_server_t));
    if (!server) {
        wbt_close_socket(fd);
        return lgx_ext_return_false(vm);
    }

    server->vm = vm;
    server->on_request = fun;
    server->pool = wbt_thread_create_pool(4, worker, server); // TODO 读取参数决定线程数量
    //server->pool = NULL;

    p_ev->ctx = server;

    lgx_ext_return_true(vm);

    lgx_co_suspend(vm);

    return 0;
}

int std_socket_server_send(void *p) {
    lgx_vm_t *vm = p;

    return 0;
}

int std_socket_server_close(void *p) {
    lgx_vm_t *vm = p;

    return 0;
}

int std_socket_load_symbols(lgx_hash_t *hash) {
    lgx_val_t symbol;

    symbol.type = T_FUNCTION;
    symbol.v.fun = lgx_fun_new(2);
    symbol.v.fun->buildin = std_socket_server_create;

    symbol.v.fun->args[0].type = T_LONG;
    symbol.v.fun->args[1].type = T_FUNCTION;

    if (lgx_ext_add_symbol(hash, "server_create", &symbol)) {
        return 1;
    }

    return 0;
}

lgx_buildin_ext_t ext_std_socket_ctx = {
    "std.socket",
    std_socket_load_symbols
};