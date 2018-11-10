
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
    // 对应协程
    lgx_co_t *co;
    // 是否已写入返回值
    unsigned is_return;
} lgx_client_t;

static wbt_status on_connect(wbt_event_t *ev);
static wbt_status on_recv(wbt_event_t *ev);
static wbt_status on_send(wbt_event_t *ev);
static wbt_status on_close(wbt_event_t *ev);
static wbt_status on_timeout(wbt_timer_t *timer);

static wbt_socket_t create_fd() {
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

static wbt_status try_connect(wbt_socket_t fd, char *ip, int port) {
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
            return WBT_AGAIN;
        }
    }

    return WBT_OK;
}

static wbt_status on_connect(wbt_event_t *ev) {
    wbt_debug("client:on_connect %d", ev->fd);

    lgx_client_t *client = (lgx_client_t *)ev->ctx;

	// 检查 socket 是否连接成功 
	int result;
	socklen_t result_len = sizeof(result);
	if (getsockopt(ev->fd, SOL_SOCKET, SO_ERROR, &result, &result_len) < 0) {
        on_close(ev);
        return WBT_OK;
	}

	if (result != 0) {
        on_close(ev);
        return WBT_OK;
	}
	
    // 修改事件
    ev->on_recv = on_recv;
    ev->on_send = on_send;
    ev->events = WBT_EV_WRITE | WBT_EV_ET;
    ev->timer.timeout = wbt_cur_mtime + 15 * 1000;

    if(wbt_event_mod(client->vm->events, ev) != WBT_OK) {
        on_close(ev);
        return WBT_OK;
    }

    return WBT_OK;
}

static wbt_status on_close(wbt_event_t *ev) {
    wbt_debug("client:on_close %d", ev->fd);
    
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

static wbt_status on_recv(wbt_event_t *ev) {
    wbt_debug("client:on_recv %d", ev->fd);

    lgx_client_t *client = (lgx_client_t *)ev->ctx;

    if (ev->recv.buf == NULL) {
        ev->recv.buf = xmalloc(WBT_MAX_PROTO_BUF_LEN);
        ev->recv.len = 0;
        
        if( ev->recv.buf == NULL ) {
            /* 内存不足 */
            on_close(ev);
            return WBT_OK;
        }
    }
    
    if (ev->recv.len >= WBT_MAX_PROTO_BUF_LEN) {
        /* 请求长度超过限制
         */
        wbt_debug("client: request length exceeds limitation %d", ev->fd);
        on_close(ev);
        return WBT_OK;
    }

    int nread = recv(ev->fd, (unsigned char *)ev->recv.buf + ev->recv.len,
        WBT_MAX_PROTO_BUF_LEN - ev->recv.len, 0);

    if (nread < 0) {
        wbt_err_t err = wbt_socket_errno;
        if (err == WBT_EAGAIN) {
            // 当前缓冲区已无数据可读
        } else if (err == WBT_ECONNRESET) {
            // 对方发送了RST
            wbt_debug("client: connection reset by peer %d", ev->fd);
            on_close(ev);
            return WBT_OK;
        } else {
            // 其他不可弥补的错误
            wbt_debug("client: errno %d %d", err, ev->fd);
            on_close(ev);
            return WBT_OK;
        }
    } else if (nread == 0) {
        wbt_debug("client: connection closed by peer %d", ev->fd);
        on_close(ev);
        return WBT_OK;
    } else {
        ev->recv.len += nread;
    }

    //printf("%.*s", (int)ev->buff_len, (char *)ev->buff);

    // TODO 如果数据读完

    // 写入返回值
    lgx_ext_return_string(client->co, lgx_str_new_ref((char *)ev->recv.buf, ev->recv.len));
    client->is_return = 1;

    ev->recv.buf = NULL;

    // 恢复协程执行
    lgx_co_resume(client->vm, client->co);

    // 关闭连接
    on_close(ev);

    return WBT_OK;
}

static wbt_status on_send(wbt_event_t *ev) {
    wbt_debug("client:on_send %d", ev->fd);

    lgx_client_t *client = (lgx_client_t *)ev->ctx;

    if (ev->send.len > ev->send.offset) {
        int nwrite = send(ev->fd, ev->send.buf + ev->send.offset,
            ev->send.len - ev->send.offset, 0);

        if (nwrite <= 0) {
            if( wbt_socket_errno == WBT_EAGAIN ) {
                return WBT_OK;
            } else {
                on_close(ev);
                return WBT_OK;
            }
        }

        ev->send.offset += nwrite;

        if (ev->send.len > ev->send.offset) {
            /* 尚未发送完，缓冲区满 */
            return WBT_OK;
        }
    }

    // 发送完毕
    //on_close(ev);
    xfree(ev->send.buf);
    ev->send.buf = NULL;
    ev->send.len = 0;

    // 修改事件
    ev->events = WBT_EV_READ | WBT_EV_ET;
    ev->timer.timeout = wbt_cur_mtime + 15 * 1000;

    if (wbt_event_mod(client->vm->events, ev) != WBT_OK) {
        on_close(ev);
        return WBT_OK;
    }

    return WBT_OK;
}

static wbt_status on_timeout(wbt_timer_t *timer) {
    wbt_event_t *ev = wbt_timer_entry(timer, wbt_event_t, timer);

    wbt_debug("[%lu] client:on_timeout %d \n", pthread_self(), ev->fd);

    return on_close(ev);
}

static int client_get(void *p) {
    lgx_vm_t *vm = (lgx_vm_t *)p;
    lgx_co_t *co = vm->co_running;

    unsigned base = vm->regs[0].v.fun->stack_size;

    //lgx_val_t *obj = &vm->regs[base+4];
    lgx_val_t *ip = &vm->regs[base+5];
    lgx_val_t *port = &vm->regs[base+6];

    if (ip->type != T_STRING || ip->v.str->length > 15) {
        // TODO throw exception?
        return lgx_ext_return_false(vm->co_running);
    }

    if (port->type != T_LONG || port->v.l <= 0 || port->v.l >= 65535) {
        // TODO throw exception?
        return lgx_ext_return_false(vm->co_running);
    }

    wbt_socket_t fd = create_fd();
    if (fd < 0) {
        return lgx_ext_return_false(vm->co_running);
    }

    char ip_str[16];
    memcpy(ip_str, ip->v.str->buffer, ip->v.str->length);
    ip_str[ip->v.str->length] = '\0';

    wbt_status ret = try_connect(fd, ip_str, port->v.l);

    if (ret == WBT_ERROR) {
        wbt_close_socket(fd);
        return lgx_ext_return_false(vm->co_running);
    } else if (ret == WBT_OK) {
        lgx_client_t *client = (lgx_client_t *)xcalloc(1, sizeof(lgx_client_t));
        client->vm = vm;
        client->co = co;

        // 添加事件
        wbt_event_t *p_ev, tmp_ev;
        tmp_ev.timer.on_timeout = on_timeout;
        tmp_ev.timer.timeout    = wbt_cur_mtime + 15 * 1000;
        tmp_ev.on_recv = on_recv;
        tmp_ev.on_send = on_send;
        tmp_ev.events  = WBT_EV_WRITE | WBT_EV_ET;
        tmp_ev.fd      = fd;

        if((p_ev = wbt_event_add(vm->events, &tmp_ev)) == NULL) {
            xfree(client);
            wbt_close_socket(fd);
            return lgx_ext_return_false(vm->co_running);
        }

        p_ev->ctx = client;

        p_ev->send.len = sizeof("GET / HTTP/1.1\r\nHost: www.fhack.com\r\n\r\n") - 1;
        p_ev->send.offset = 0;
        p_ev->send.buf = strdup("GET / HTTP/1.1\r\nHost: www.fhack.com\r\n\r\n\r\n\r\n");

        lgx_co_suspend(vm);
    } else { // ret == WBT_AGAIN
        lgx_client_t *client = (lgx_client_t *)xcalloc(1, sizeof(lgx_client_t));
        client->vm = vm;
        client->co = co;

        // 添加事件
        wbt_event_t *p_ev, tmp_ev;
        tmp_ev.timer.on_timeout = on_timeout;
        tmp_ev.timer.timeout    = wbt_cur_mtime + 15 * 1000;
        tmp_ev.on_recv = NULL;
        tmp_ev.on_send = on_connect;
        tmp_ev.events  = WBT_EV_WRITE | WBT_EV_ET;
        tmp_ev.fd      = fd;

        if((p_ev = wbt_event_add(vm->events, &tmp_ev)) == NULL) {
            xfree(client);
            wbt_close_socket(fd);
            return lgx_ext_return_false(vm->co_running);
        }

        p_ev->ctx = client;

        p_ev->send.len = sizeof("GET / HTTP/1.1\r\nHost: www.fhack.com\r\n\r\n\r\n\r\n") - 1;
        p_ev->send.offset = 0;
        p_ev->send.buf = strdup("GET / HTTP/1.1\r\nHost: www.fhack.com\r\n\r\n\r\n\r\n");

        lgx_co_suspend(vm);
    }

    return 0;
}

static int std_net_tcp_client_load_symbols(lgx_hash_t *hash) {
    lgx_str_t name;
    name.buffer = "Client";
    name.length = sizeof("Client")-1;

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

    lgx_hash_node_t symbol_get;
    symbol_get.k.type = T_STRING;
    symbol_get.k.v.str = lgx_str_new_ref("get", sizeof("get")-1);
    symbol_get.v.type = T_FUNCTION;
    symbol_get.v.v.fun = lgx_fun_new(2);
    symbol_get.v.v.fun->buildin = client_get;
    symbol_get.v.v.fun->args[0].type = T_STRING;
    symbol_get.v.v.fun->args[1].type = T_LONG;

    if (lgx_obj_add_method(symbol.v.obj, &symbol_get)) {
        return 1;
    }

    if (lgx_ext_add_symbol(hash, symbol.v.obj->name->buffer, &symbol)) {
        return 1;
    }

    return 0;
}

lgx_buildin_ext_t ext_std_net_tcp_client_ctx = {
    "std.net.tcp",
    std_net_tcp_client_load_symbols
};