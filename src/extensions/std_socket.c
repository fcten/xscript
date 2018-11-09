
#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "../common/str.h"
#include "../common/fun.h"
#include "../common/obj.h"
#include "../interpreter/coroutine.h"
#include "std_coroutine.h"

#define WBT_MAX_PROTO_BUF_LEN (64 * 1024)

extern time_t wbt_cur_mtime;

typedef struct {
    // 主线程
    lgx_vm_t *vm;
    // 工作线程
    wbt_thread_pool_t *pool;
    // Server 对象
    lgx_obj_t *obj;
    // on_request
    lgx_fun_t *on_request;
} lgx_server_t;

typedef struct {
    // 主线程
    lgx_vm_t *vm;
    // 对应协程
    lgx_co_t *co;
    // 是否已写入返回值
    unsigned is_return;
} lgx_client_t;

wbt_status std_socket_on_close(wbt_event_t *ev);

wbt_status std_socket_client_on_recv(wbt_event_t *ev);
wbt_status std_socket_client_on_send(wbt_event_t *ev);
wbt_status std_socket_client_on_close(wbt_event_t *ev);
wbt_status std_socket_client_on_timeout(wbt_timer_t *timer);

wbt_socket_t std_socket_create_fd() {
    wbt_socket_t fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        //wbt_log_add("create socket failed\n");
        return fd;
    }

    /* 把监听socket设置为非阻塞方式 */
    if( wbt_nonblocking(fd) == -1 ) {
        //wbt_log_add("set nonblocking failed\n");
        wbt_close_socket(fd);
        return fd;
    }

    int on = 1;
    /* TODO 发送大文件时，使用 TCP_CORK 关闭 Nagle 算法保证网络利用率 */
    //setsockopt(fd, SOL_TCP, TCP_CORK, &on, sizeof ( on ));
    /* 测试 keep-alive 性能时，使用 TCP_NODELAY 立即发送小数据，否则会阻塞 40 毫秒 */
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on));

    return fd;
}

wbt_status std_socket_listen(wbt_socket_t fd, int port) {
    /* 在重启程序以及进行热更新时，避免 TIME_WAIT 和 CLOSE_WAIT 状态的连接导致 bind 失败 */
    int on = 1; 
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) != 0) {  
        //wbt_log_add("set SO_REUSEADDR failed\n");
        return WBT_ERROR;
    }

    /* bind & listen */    
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    if(bind(fd, (const struct sockaddr*)&sin, sizeof(sin)) != 0) {
        //wbt_log_add("bind failed\n");
        return WBT_ERROR;
    }

    if(listen(fd, WBT_CONN_BACKLOG) != 0) {
        //wbt_log_add("listen failed\n");
        return WBT_ERROR;
    }

    return WBT_OK;
}

wbt_status std_socket_connect(wbt_socket_t fd, char *ip, int port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
#ifdef WIN32
    addr.sin_addr.S_un.S_addr = inet_addr(ip);
#else
    addr.sin_addr.s_addr = inet_addr(ip);
#endif
    memset(addr.sin_zero, 0x00, 8);

    int ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        wbt_err_t err = wbt_socket_errno;

        if (err != WBT_EINPROGRESS
#ifdef WIN32
            /* Winsock returns WSAEWOULDBLOCK (WBT_EAGAIN) */
            && err != WBT_EAGAIN
#endif
            )
        {
            if (err == WBT_ECONNREFUSED
#ifndef WIN32 // TODO 判断 LINUX 平台
                /*
                 * Linux returns EAGAIN instead of ECONNREFUSED
                 * for unix sockets if listen queue is full
                 */
                || err == WBT_EAGAIN
#endif
                || err == WBT_ECONNRESET
                || err == WBT_ENETDOWN
                || err == WBT_ENETUNREACH
                || err == WBT_EHOSTDOWN
                || err == WBT_EHOSTUNREACH)
            {
                // 普通错误
                return WBT_ERROR;
            } else {
                // 未知错误
                return WBT_ERROR;
            }
        } else {
            return WBT_EAGAIN;
        }
    }

    return WBT_OK;
}

wbt_status std_socket_client_on_connect(wbt_event_t *ev) {
    lgx_client_t *client = (lgx_client_t *)ev->ctx;

	// 检查 socket 是否连接成功 
	int result;
	socklen_t result_len = sizeof(result);
	if (getsockopt(ev->fd, SOL_SOCKET, SO_ERROR, &result, &result_len) < 0) {
        std_socket_client_on_close(ev);
        return WBT_OK;
	}

	if (result != 0) {
        std_socket_client_on_close(ev);
        return WBT_OK;
	}
	
    // 修改事件
    ev->on_recv = std_socket_client_on_recv;
    ev->on_send = std_socket_client_on_send;
    ev->events = WBT_EV_WRITE | WBT_EV_ET;
    ev->timer.timeout = wbt_cur_mtime + 15 * 1000;

    if(wbt_event_mod(client->vm->events, ev) != WBT_OK) {
        std_socket_client_on_close(ev);
        return WBT_OK;
    }

    return WBT_OK;
}

wbt_status std_socket_on_close(wbt_event_t *ev) {
    //wbt_log_debug("connection %d close.",ev->fd);
    
    lgx_server_t *server = (lgx_server_t *)ev->ctx;

    wbt_event_del(server->vm->events, ev);

    return WBT_OK;
}

wbt_status std_socket_client_on_close(wbt_event_t *ev) {
    //wbt_log_debug("connection %d close.",ev->fd);
    
    lgx_client_t *client = (lgx_client_t *)ev->ctx;

    wbt_event_del(client->vm->events, ev);

    // 写入返回值
    if (!client->is_return) {
        lgx_ext_return_false(client->co);
    }

    // 恢复协程执行
    lgx_co_resume(client->vm, client->co);

    xfree(client);

    return WBT_OK;
}

int std_socket_on_yield(lgx_vm_t *vm) {
    lgx_co_t *co = vm->co_running;
    wbt_event_t *ev = (wbt_event_t *)co->ctx;

    if (co->status != CO_DIED) {
        return 0;
    }

    // 读取返回值
    if (vm->regs[1].type != T_STRING) {
        std_socket_on_close(ev);
        return 1;
    }
    lgx_str_t *str = vm->regs[1].v.str;

    // 生成响应
    ev->buff = xmalloc(str->length);
    if (!ev->buff) {
        std_socket_on_close(ev);
        return 1;
    }
    memcpy(ev->buff, str->buffer, str->length);
    ev->buff_len = str->length;
    ev->buff_offset = 0;

    // 修改事件
    ev->events = WBT_EV_WRITE | WBT_EV_ET;
    ev->timer.timeout = wbt_cur_mtime + 15 * 1000;

    if(wbt_event_mod(vm->events, ev) != WBT_OK) {
        std_socket_on_close(ev);
        return 1;
    }

    return 0;
}

wbt_status std_socket_on_recv(wbt_event_t *ev) {
    //printf("[%lu] on_recv %d \n", pthread_self(), ev->fd);

    lgx_server_t *server = (lgx_server_t *)ev->ctx;

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
    if (!co) {
        std_socket_on_close(ev);
        return WBT_OK;
    }

    co->on_yield = std_socket_on_yield;
    co->ctx = ev;

    // 写入参数
    co->stack.buf[4].type = T_OBJECT;
    co->stack.buf[4].v.obj = server->obj;
    lgx_gc_ref_add(&co->stack.buf[4]);

    return WBT_OK;
}

wbt_status std_socket_client_on_recv(wbt_event_t *ev) {
    //printf("[%lu] on_recv %d \n", pthread_self(), ev->fd);

    lgx_client_t *client = (lgx_client_t *)ev->ctx;

    if (ev->buff == NULL) {
        ev->buff = xmalloc(WBT_MAX_PROTO_BUF_LEN);
        ev->buff_len = 0;
        
        if( ev->buff == NULL ) {
            /* 内存不足 */
            std_socket_client_on_close(ev);
            return WBT_OK;
        }
    }
    
    if (ev->buff_len >= WBT_MAX_PROTO_BUF_LEN) {
        /* 请求长度超过限制
         */
        //wbt_log_add("connection close: request length exceeds limitation\n");
        std_socket_client_on_close(ev);
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
            std_socket_client_on_close(ev);
            return WBT_OK;
        } else {
            // 其他不可弥补的错误
            //wbt_log_add("connection close: %d\n", err);
            std_socket_client_on_close(ev);
            return WBT_OK;
        }
    } else if (nread == 0) {
        //wbt_log_add("connection close: connection closed by peer\n");
        //std_socket_client_on_close(ev);
        //return WBT_OK;
    }
    
    ev->buff_len += nread;

    //printf("%.*s", (int)ev->buff_len, (char *)ev->buff);

    // TODO 如果数据读完
    if (nread > 0) {
        return WBT_OK;
    }

    // 写入返回值
    lgx_ext_return_string(client->co, lgx_str_new_ref(ev->buff, ev->buff_len));
    client->is_return = 1;

    ev->buff = NULL;

    // 恢复协程执行
    lgx_co_resume(client->vm, client->co);

    // 关闭连接
    std_socket_client_on_close(ev);

    return WBT_OK;
}

wbt_status std_socket_on_send(wbt_event_t *ev) {
    //printf("[%lu] on_send %d\n", pthread_self(), ev->fd);
    lgx_server_t *server = (lgx_server_t *)ev->ctx;

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

wbt_status std_socket_client_on_send(wbt_event_t *ev) {
    //printf("[%lu] on_send %d\n", pthread_self(), ev->fd);
    lgx_client_t *client = (lgx_client_t *)ev->ctx;

    if (ev->buff_len > ev->buff_offset) {
        int nwrite = send(ev->fd, ev->buff + ev->buff_offset,
            ev->buff_len - ev->buff_offset, 0);

        if (nwrite <= 0) {
            if( wbt_socket_errno == WBT_EAGAIN ) {
                return WBT_OK;
            } else {
                std_socket_client_on_close(ev);
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

    if (wbt_event_mod(client->vm->events, ev) != WBT_OK) {
        std_socket_client_on_close(ev);
        return WBT_OK;
    }

    return WBT_OK;
}

wbt_status std_socket_on_timeout(wbt_timer_t *timer) {
    wbt_event_t *ev = wbt_timer_entry(timer, wbt_event_t, timer);
    //printf("[%lu] on_timeout %d\n", pthread_self(), ev->fd);

    return std_socket_on_close(ev);
}

wbt_status std_socket_client_on_timeout(wbt_timer_t *timer) {
    wbt_event_t *ev = wbt_timer_entry(timer, wbt_event_t, timer);
    //printf("[%lu] on_timeout %d\n", pthread_self(), ev->fd);

    return std_socket_client_on_close(ev);
}

wbt_status std_socket_on_accept(wbt_event_t *ev) {
    lgx_server_t *server = (lgx_server_t *)ev->ctx;

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
    wbt_thread_t *thread = (wbt_thread_t *)args;
    lgx_server_t *master = (lgx_server_t *)thread->ctx;

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

int std_socket_server_listen(void *p) {
    lgx_vm_t *vm = (lgx_vm_t *)p;

    unsigned base = vm->regs[0].v.fun->stack_size;

    lgx_val_t *obj = &vm->regs[base+4];
    lgx_val_t *port = &vm->regs[base+5];

    if (port->type != T_LONG || port->v.l <= 0 || port->v.l >= 65535) {
        // TODO throw exception?
        return lgx_ext_return_false(vm->co_running);
    }

    // 创建监听端口
    wbt_socket_t fd = std_socket_create_fd();
    if (fd < 0) {
        return lgx_ext_return_false(vm->co_running);
    }

    if (std_socket_listen(fd, port->v.l) != WBT_OK) {
        wbt_close_socket(fd);
        return lgx_ext_return_false(vm->co_running);
    }

    // 把 fd 记录到对象的 fd 属性中
    lgx_val_t k, v;
    k.type = T_STRING;
    k.v.str = lgx_str_new_ref("fd", sizeof("fd")-1); // TODO memory leak
    v.type = T_LONG;
    v.v.l = fd;
    if (lgx_obj_set(obj->v.obj, &k, &v) != 0) {
        wbt_close_socket(fd);
        return lgx_ext_return_false(vm->co_running);
    }

    return lgx_ext_return_true(vm->co_running);
}

int std_socket_server_on_request(void *p) {
    lgx_vm_t *vm = (lgx_vm_t *)p;

    return lgx_ext_return_true(vm->co_running);
}

int std_socket_get(void *p) {
    lgx_vm_t *vm = (lgx_vm_t *)p;
    lgx_co_t *co = vm->co_running;

    wbt_socket_t fd = std_socket_create_fd();
    if (fd < 0) {
        return lgx_ext_return_false(vm->co_running);
    }

    wbt_status ret = std_socket_connect(fd, "127.0.0.1", 1039);

    if (ret == WBT_ERROR) {
        wbt_close_socket(fd);
        return lgx_ext_return_false(vm->co_running);
    } else if (ret == WBT_OK) {
        lgx_client_t *client = (lgx_client_t *)xcalloc(1, sizeof(lgx_client_t));
        client->vm = vm;
        client->co = co;

        // 添加事件
        wbt_event_t *p_ev, tmp_ev;
        tmp_ev.timer.on_timeout = std_socket_client_on_timeout;
        tmp_ev.timer.timeout    = wbt_cur_mtime + 15 * 1000;
        tmp_ev.on_recv = std_socket_client_on_recv;
        tmp_ev.on_send = std_socket_client_on_send;
        tmp_ev.events  = WBT_EV_WRITE | WBT_EV_ET;
        tmp_ev.fd      = fd;

        if((p_ev = wbt_event_add(vm->events, &tmp_ev)) == NULL) {
            xfree(client);
            wbt_close_socket(fd);
            return lgx_ext_return_false(vm->co_running);
        }

        p_ev->ctx = client;

        p_ev->buff_len = sizeof("GET / HTTP/1.1\r\nHost: www.fhack.com\r\n\r\n") - 1;
        p_ev->buff_offset = 0;
        p_ev->buff = strdup("GET / HTTP/1.1\r\nHost: www.fhack.com\r\n\r\n\r\n\r\n");

        lgx_co_suspend(vm);
    } else { // ret == WBT_EAGAIN
        lgx_client_t *client = (lgx_client_t *)xcalloc(1, sizeof(lgx_client_t));
        client->vm = vm;
        client->co = co;

        // 添加事件
        wbt_event_t *p_ev, tmp_ev;
        tmp_ev.timer.on_timeout = std_socket_client_on_timeout;
        tmp_ev.timer.timeout    = wbt_cur_mtime + 15 * 1000;
        tmp_ev.on_recv = NULL;
        tmp_ev.on_send = std_socket_client_on_connect;
        tmp_ev.events  = WBT_EV_WRITE | WBT_EV_ET;
        tmp_ev.fd      = fd;

        if((p_ev = wbt_event_add(vm->events, &tmp_ev)) == NULL) {
            xfree(client);
            wbt_close_socket(fd);
            return lgx_ext_return_false(vm->co_running);
        }

        p_ev->ctx = client;

        p_ev->buff_len = sizeof("GET / HTTP/1.1\r\nHost: www.fhack.com\r\n\r\n\r\n\r\n") - 1;
        p_ev->buff_offset = 0;
        p_ev->buff = strdup("GET / HTTP/1.1\r\nHost: www.fhack.com\r\n\r\n\r\n\r\n");

        lgx_co_suspend(vm);
    }

    return 0;
}

int std_socket_server_start(void *p) {
    lgx_vm_t *vm = (lgx_vm_t *)p;

    unsigned base = vm->regs[0].v.fun->stack_size;

    lgx_val_t *obj = &vm->regs[base+4];
    lgx_val_t *worker_num = &vm->regs[base+5];

    if (worker_num->type != T_LONG || worker_num->v.l < 0 || worker_num->v.l > 128) {
        // TODO throw exception?
        return lgx_ext_return_false(vm->co_running);
    }

    //lgx_obj_print(obj->v.obj);

    lgx_str_t name;
    name.buffer = "fd";
    name.length = sizeof("fd")-1;
    lgx_val_t k;
    k.type = T_STRING;
    k.v.str = &name;
    lgx_val_t *fd = lgx_obj_get(obj->v.obj, &k);
    if (!fd || fd->type != T_LONG || fd->v.l == 0) {
        return lgx_ext_return_false(vm->co_running);
    }

    name.buffer = "onRequest";
    name.length = sizeof("onRequest")-1;
    lgx_val_t *on_req = lgx_obj_get(obj->v.obj, &k);
    if (!on_req || on_req->type != T_FUNCTION || on_req->v.fun == NULL) {
        return lgx_ext_return_false(vm->co_running);
    }

    // 把监听的 socket 加入事件循环
    wbt_event_t ev, *p_ev;
    memset(&ev, 0, sizeof(wbt_event_t));

    ev.on_recv = std_socket_on_accept;
    ev.on_send = NULL;
    ev.events = WBT_EV_READ | WBT_EV_ET;
    ev.fd = fd->v.l;

    if((p_ev = wbt_event_add(vm->events, &ev)) == NULL) {
        wbt_close_socket(fd->v.l);
        return lgx_ext_return_false(vm->co_running);
    }

    lgx_server_t *server = (lgx_server_t *)xmalloc(sizeof(lgx_server_t));
    if (!server) {
        wbt_close_socket(fd->v.l);
        return lgx_ext_return_false(vm->co_running);
    }

    server->vm = vm;
    server->obj = obj->v.obj;
    server->on_request = on_req->v.fun;
    if (worker_num->v.l) {
        server->pool = wbt_thread_create_pool(worker_num->v.l, worker, server); // TODO 读取参数决定线程数量
    } else {
        server->pool = NULL;
    }

    p_ev->ctx = server;

    lgx_ext_return_true(vm->co_running);

    lgx_co_suspend(vm);

    return 0;
}

int std_socket_load_symbols(lgx_hash_t *hash) {
    lgx_str_t name;
    name.buffer = "Server";
    name.length = sizeof("Server")-1;

    lgx_val_t symbol;
    symbol.type = T_OBJECT;
    symbol.v.obj = lgx_obj_create(&name);

    lgx_hash_node_t symbol_fd;
    symbol_fd.k.type = T_STRING;
    symbol_fd.k.v.str = lgx_str_new_ref("fd", sizeof("fd")-1);
    symbol_fd.v.type = T_LONG;
    symbol_fd.v.v.l = 0;
    symbol_fd.v.u.c.access = P_PRIVATE;

    if (lgx_obj_add_property(symbol.v.obj, &symbol_fd)) {
        return 1;
    }

    lgx_hash_node_t symbol_listen;
    symbol_listen.k.type = T_STRING;
    symbol_listen.k.v.str = lgx_str_new_ref("listen", sizeof("listen")-1);
    symbol_listen.v.type = T_FUNCTION;
    symbol_listen.v.v.fun = lgx_fun_new(1);
    symbol_listen.v.v.fun->buildin = std_socket_server_listen;
    symbol_listen.v.v.fun->args[0].type = T_LONG;

    if (lgx_obj_add_method(symbol.v.obj, &symbol_listen)) {
        return 1;
    }

    lgx_hash_node_t symbol_on_request;
    symbol_on_request.k.type = T_STRING;
    symbol_on_request.k.v.str = lgx_str_new_ref("onRequest", sizeof("onRequest")-1);
    symbol_on_request.v.type = T_FUNCTION;
    symbol_on_request.v.v.fun = lgx_fun_new(1);
    symbol_on_request.v.v.fun->buildin = std_socket_server_on_request;
    symbol_on_request.v.v.fun->args[0].type = T_FUNCTION; // TODO 编译时需要能够检查函数原型

    if (lgx_obj_add_method(symbol.v.obj, &symbol_on_request)) {
        return 1;
    }

    lgx_hash_node_t symbol_get;
    symbol_get.k.type = T_STRING;
    symbol_get.k.v.str = lgx_str_new_ref("get", sizeof("get")-1);
    symbol_get.v.type = T_FUNCTION;
    symbol_get.v.v.fun = lgx_fun_new(0);
    symbol_get.v.v.fun->buildin = std_socket_get;

    if (lgx_obj_add_method(symbol.v.obj, &symbol_get)) {
        return 1;
    }

    lgx_hash_node_t symbol_create;
    symbol_create.k.type = T_STRING;
    symbol_create.k.v.str = lgx_str_new_ref("start", sizeof("start")-1);
    symbol_create.v.type = T_FUNCTION;
    symbol_create.v.v.fun = lgx_fun_new(1);
    symbol_create.v.v.fun->buildin = std_socket_server_start;
    symbol_create.v.v.fun->args[0].type = T_LONG;

    if (lgx_obj_add_method(symbol.v.obj, &symbol_create)) {
        return 1;
    }

    if (lgx_ext_add_symbol(hash, symbol.v.obj->name->buffer, &symbol)) {
        return 1;
    }

    return 0;
}

lgx_buildin_ext_t ext_std_socket_ctx = {
    "std.socket",
    std_socket_load_symbols
};