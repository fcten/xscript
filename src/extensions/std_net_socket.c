#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "../common/obj.h"
#include "../common/str.h"
#include "../common/fun.h"
#include "../common/res.h"
#include "std_net_socket.h"

extern time_t wbt_cur_mtime;

typedef struct {
    // socket
    wbt_socket_t fd;
    // 当前阻塞的协程，没有则为 NULL
    lgx_co_t *co;
    // 所属的对象
    lgx_obj_t *obj;
    // send/recv数据缓存
    char *buffer;
    unsigned length;
    unsigned offset;
} lgx_socket_t;

LGX_METHOD(Socket, constructor) {
    LGX_METHOD_ARGS_INIT();
    LGX_METHOD_ARGS_THIS(obj);

    lgx_socket_t *sock = NULL;
    lgx_res_t *res = NULL;

    sock = xcalloc(1, sizeof(lgx_socket_t));
    if (!sock) {
        lgx_vm_throw_s(vm, "xcalloc() failed");
        goto err;
    }

    sock->fd = -1;
    sock->obj = obj->v.obj;

    res = lgx_res_new(0, sock);
    if (!res) {
        lgx_vm_throw_s(vm, "lgx_res_new() failed");
        goto err;
    }

    lgx_val_t k, v;
    k.type = T_STRING;
    k.v.str = lgx_str_new_ref("ctx", sizeof("ctx") - 1);
    v.type = T_RESOURCE;
    v.v.res = res;
    if (lgx_obj_set(obj->v.obj, &k, &v)) {
        lgx_res_delete(res);
        lgx_vm_throw_s(vm, "lgx_obj_set() failed");
        goto err;
    }

    sock->fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock->fd < 0) {
        lgx_vm_throw_s(vm, "socket() failed");
        goto err;
    }

    /* 把监听socket设置为非阻塞方式 */
    if ( wbt_nonblocking(sock->fd) == -1 ) {
        wbt_close_socket(sock->fd);
        sock->fd = -1;
        lgx_vm_throw_s(vm, "wbt_nonblocking() failed");
        goto err;
    }

    int on = 1;
    if (setsockopt(sock->fd, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on)) != 0) {
        wbt_close_socket(sock->fd);
        sock->fd = -1;
        lgx_vm_throw_s(vm, "setsockopt(TCP_NODELAY) failed");
        goto err;
    }

    LGX_RETURN_TRUE();
    return 0;

err:
    if (res) {
        lgx_res_delete(res);
    } else if (sock) {
        xfree(sock);
    }

    LGX_RETURN_FALSE();
    return 0;
}

LGX_METHOD(Socket, bind) {
    LGX_METHOD_ARGS_INIT();
    LGX_METHOD_ARGS_THIS(obj);
    LGX_METHOD_ARGS_GET(ip, 0, T_STRING);
    LGX_METHOD_ARGS_GET(port, 1, T_LONG);

    if (ip->v.str->length > 32) {
        lgx_vm_throw_s(vm, "invalid param `ip`");
        return 1;
    }

    if (port->v.l <= 0 || port->v.l >= 65535) {
        lgx_vm_throw_s(vm, "invalid param `port`");
        return 1;
    }

    lgx_val_t *res = lgx_obj_get_s(obj->v.obj, "ctx");
    if (!res || res->type != T_RESOURCE) {
        lgx_vm_throw_s(vm, "invalid property `ctx`");
        return 1;
    }

    lgx_socket_t *sock = (lgx_socket_t *)res->v.res->buf;
    if (sock->fd < 0) {
        lgx_vm_throw_s(vm, "socket alreay closed");
        return 1;
    }

    /* 在重启程序以及进行热更新时，避免 TIME_WAIT 和 CLOSE_WAIT 状态的连接导致 bind 失败 */
    int on = 1; 
    if(setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) != 0) {  
        lgx_vm_throw_s(vm, "setsockopt(SO_REUSEADDR) failed");
        return 1;
    }

    char ip_str[64];
    memcpy(ip_str, ip->v.str->buffer, ip->v.str->length);
    ip_str[ip->v.str->length] = '\0';

    /* bind & listen */    
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(ip_str);
    sin.sin_port = htons(port->v.l);

    if(bind(sock->fd, (const struct sockaddr*)&sin, sizeof(sin)) != 0) {
        //wbt_log_add("bind failed\n");
        lgx_vm_throw_s(vm, "bind() failed");
        return 1;
    }

    LGX_RETURN_TRUE();
    return 0;
}

LGX_METHOD(Socket, listen) {
    LGX_METHOD_ARGS_INIT();
    LGX_METHOD_ARGS_THIS(obj);

    lgx_val_t *res = lgx_obj_get_s(obj->v.obj, "ctx");
    if (!res || res->type != T_RESOURCE) {
        lgx_vm_throw_s(vm, "invalid property `ctx`");
        return 1;
    }

    lgx_socket_t *sock = (lgx_socket_t *)res->v.res->buf;
    if (sock->fd < 0) {
        lgx_vm_throw_s(vm, "socket alreay closed");
        return 1;
    }

    if(listen(sock->fd, WBT_CONN_BACKLOG) != 0) {
        lgx_vm_throw_s(vm, "listen() failed");
        return 1;
    }

    LGX_RETURN_TRUE();
    return 0;
}

static wbt_status on_accept(wbt_event_t *ev);

static void do_accept(lgx_socket_t *sock) {
    lgx_vm_t *vm = sock->co->vm;
    lgx_co_t *co = sock->co;
    lgx_obj_t *obj = sock->obj;

    struct sockaddr_in remote;
    int addrlen = sizeof(remote);
    wbt_socket_t conn_sock;
#ifdef WBT_USE_ACCEPT4
    conn_sock = accept4(sock->fd,(struct sockaddr *) &remote, (socklen_t *)&addrlen, SOCK_NONBLOCK);
#else
    conn_sock = accept(sock->fd,(struct sockaddr *) &remote, (int *)&addrlen));
    if (conn_sock >= 0) {
        if ( wbt_nonblocking(conn_sock) == -1 ) {
            wbt_close_socket(conn_sock);
            sock->co = NULL;
            lgx_co_throw_s(co, "wbt_nonblocking() failed");
            return;
        }
    }
#endif

    if (conn_sock >= 0) {
        int on = 1;
        if (setsockopt(conn_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on)) != 0) {
            wbt_close_socket(conn_sock);
            sock->co = NULL;
            lgx_co_throw_s(co, "setsockopt(TCP_NODELAY) failed");
            return;
        }

        lgx_socket_t *newsock = xcalloc(1, sizeof(lgx_socket_t));
        if (!newsock) {
            wbt_close_socket(conn_sock);
            sock->co = NULL;
            lgx_co_throw_s(co, "xcalloc() failed");
            return;
        }

        lgx_res_t *res = lgx_res_new(0, newsock);
        if (!res) {
            wbt_close_socket(conn_sock);
            xfree(newsock);
            sock->co = NULL;
            lgx_co_throw_s(co, "lgx_res_new() failed");
            return;
        }

        // 新建 Socket 对象
        lgx_obj_t *newobj = lgx_obj_new(obj->parent);

        lgx_val_t k, v;
        k.type = T_STRING;
        k.v.str = lgx_str_new_ref("ctx", sizeof("ctx") - 1);
        v.type = T_RESOURCE;
        v.v.res = res;
        if (lgx_obj_set(newobj, &k, &v)) {
            wbt_close_socket(conn_sock);
            lgx_res_delete(res);
            sock->co = NULL;
            lgx_co_throw_s(co, "lgx_obj_set() failed");
            return;
        }

        newsock->obj = newobj;
        newsock->fd = conn_sock;

        sock->co = NULL;
        lgx_co_return_object(co, newobj);
    } else {
        wbt_err_t err = wbt_socket_errno;

        if (err == WBT_EAGAIN) {
            if (!(co && co->status == CO_RUNNING)) {
                sock->co = NULL;
                lgx_co_throw_s(co, "unexpected EAGAIN");
                return;
            }

            // 阻塞等待
            wbt_event_t ev, *p_ev;
            memset(&ev, 0, sizeof(wbt_event_t));

            ev.on_recv = on_accept;
            ev.on_send = NULL;
            ev.events = WBT_EV_READ | WBT_EV_ET;
            ev.fd = sock->fd;

            if((p_ev = wbt_event_add(vm->events, &ev)) == NULL) {
                sock->co = NULL;
                lgx_co_throw_s(co, "wbt_event_add() failed");
                return;
            }

            p_ev->ctx = sock;

            lgx_co_suspend(vm);
        } else {
            sock->co = NULL;
            lgx_co_throw_s(co, strerror(err));
        }
    }
}

static wbt_status on_accept(wbt_event_t *ev) {
    lgx_socket_t *sock = (lgx_socket_t *)ev->ctx;

    wbt_debug("socket:on_accept %d", ev->fd);

    wbt_event_del(sock->co->vm->events, ev);

    lgx_co_resume(sock->co->vm, sock->co);

    do_accept(sock);

    return WBT_OK;
}

LGX_METHOD(Socket, accept) {
    LGX_METHOD_ARGS_INIT();
    LGX_METHOD_ARGS_THIS(obj);

    lgx_val_t *res = lgx_obj_get_s(obj->v.obj, "ctx");
    if (!res || res->type != T_RESOURCE) {
        lgx_vm_throw_s(vm, "invalid property `ctx`");
        return 1;
    }

    lgx_socket_t *sock = (lgx_socket_t *)res->v.res->buf;
    if (sock->fd < 0) {
        lgx_vm_throw_s(vm, "socket alreay closed");
        return 1;
    }

    // 不能在多个协程中并发执行
    if (sock->co) {
        lgx_vm_throw_s(vm, "concurrent accept");
        return 1;
    }
    sock->co = vm->co_running;

    do_accept(sock);
    return 0;
}

static wbt_status on_timeout(wbt_timer_t *timer) {
    wbt_event_t *ev = wbt_timer_entry(timer, wbt_event_t, timer);

    wbt_debug("socket:on_timeout %d", ev->fd);

    lgx_socket_t *sock = ev->ctx;
    lgx_co_resume(sock->co->vm, sock->co);
    lgx_co_throw_s(sock->co, "timeout");

    wbt_event_del(sock->co->vm->events, ev);

    return WBT_OK;
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
    lgx_socket_t *sock = (lgx_socket_t *)ev->ctx;

    wbt_debug("socket:on_connect %d", ev->fd);

    wbt_event_del(sock->co->vm->events, ev);

    lgx_co_resume(sock->co->vm, sock->co);

    sock->co = NULL;

    return WBT_OK;
}

LGX_METHOD(Socket, connect) {
    LGX_METHOD_ARGS_INIT();
    LGX_METHOD_ARGS_THIS(obj);
    LGX_METHOD_ARGS_GET(ip, 0, T_STRING);
    LGX_METHOD_ARGS_GET(port, 1, T_LONG);

    if (ip->v.str->length > 32) {
        lgx_vm_throw_s(vm, "invalid param `ip`");
        return 1;
    }

    if (port->v.l <= 0 || port->v.l >= 65535) {
        lgx_vm_throw_s(vm, "invalid param `port`");
        return 1;
    }

    lgx_val_t *res = lgx_obj_get_s(obj->v.obj, "ctx");
    if (!res || res->type != T_RESOURCE) {
        lgx_vm_throw_s(vm, "invalid property `ctx`");
        return 1;
    }

    lgx_socket_t *sock = (lgx_socket_t *)res->v.res->buf;
    if (sock->fd < 0) {
        lgx_vm_throw_s(vm, "socket alreay closed");
        return 1;
    }

    // 不能在多个协程中并发执行
    if (sock->co) {
        lgx_vm_throw_s(vm, "concurrent connect");
        return 1;
    }
    sock->co = vm->co_running;

    char ip_str[32];
    memcpy(ip_str, ip->v.str->buffer, ip->v.str->length);
    ip_str[ip->v.str->length] = '\0';

    wbt_status ret = try_connect(sock->fd, ip_str, port->v.l);

    if (ret == WBT_ERROR) {
        sock->co = NULL;
        lgx_vm_throw_s(vm, "connect faild");
        return 1;
    } else if (ret == WBT_OK) {
        sock->co = NULL;
        LGX_RETURN_TRUE();
        return 0;
    } else { // ret == WBT_AGAIN
        // 添加事件
        wbt_event_t *p_ev, tmp_ev;
        tmp_ev.timer.on_timeout = on_timeout;
        tmp_ev.timer.timeout    = wbt_cur_mtime + 15 * 1000;
        tmp_ev.on_recv = NULL;
        tmp_ev.on_send = on_connect;
        tmp_ev.events  = WBT_EV_WRITE | WBT_EV_ET;
        tmp_ev.fd      = sock->fd;

        if((p_ev = wbt_event_add(vm->events, &tmp_ev)) == NULL) {
            lgx_vm_throw_s(vm, "add event faild");
            return 1;
        }

        p_ev->ctx = sock;
        lgx_co_suspend(vm);
        return 0;
    }
}

static wbt_status on_send(wbt_event_t *ev);

static void do_send(lgx_socket_t *sock) {
    lgx_vm_t *vm = sock->co->vm;
    lgx_co_t *co = sock->co;

    if (sock->length > sock->offset) {
        int nwrite = send(sock->fd, sock->buffer + sock->offset,
            sock->length - sock->offset, 0);

        if (nwrite <= 0) {
            wbt_err_t err = wbt_socket_errno;
            if (err == WBT_EAGAIN) {
                goto wait_send;
            } else {
                sock->co = NULL;
                lgx_co_throw_s(co, strerror(err));
                return;
            }
        }

        wbt_debug("socket:send %.*s", nwrite, sock->buffer + sock->offset);

        sock->offset += nwrite;

        if (sock->length > sock->offset) {
            goto wait_send;
        }
    }

    // 发送完毕
    sock->co = NULL;
    lgx_co_return_true(co);
    return;

wait_send:;
    // 阻塞等待
    wbt_event_t ev, *p_ev;
    memset(&ev, 0, sizeof(wbt_event_t));
    ev.timer.on_timeout = on_timeout;
    ev.timer.timeout    = wbt_cur_mtime + 30 * 1000;
    ev.on_recv = NULL;
    ev.on_send = on_send;
    ev.events = WBT_EV_WRITE | WBT_EV_ET;
    ev.fd = sock->fd;

    if((p_ev = wbt_event_add(vm->events, &ev)) == NULL) {
        sock->co = NULL;
        lgx_co_throw_s(co, "wbt_event_add() failed");
        return;
    }

    p_ev->ctx = sock;

    lgx_co_suspend(vm);
}

static wbt_status on_send(wbt_event_t *ev) {
    lgx_socket_t *sock = (lgx_socket_t *)ev->ctx;

    wbt_debug("socket:on_send %d", ev->fd);

    wbt_event_del(sock->co->vm->events, ev);

    lgx_co_resume(sock->co->vm, sock->co);

    do_send(sock);

    return WBT_OK;
}

LGX_METHOD(Socket, send) {
    LGX_METHOD_ARGS_INIT();
    LGX_METHOD_ARGS_THIS(obj);
    LGX_METHOD_ARGS_GET(data, 0, T_STRING);

    lgx_val_t *res = lgx_obj_get_s(obj->v.obj, "ctx");
    if (!res || res->type != T_RESOURCE) {
        lgx_vm_throw_s(vm, "invalid property `ctx`");
        return 1;
    }

    lgx_socket_t *sock = (lgx_socket_t *)res->v.res->buf;
    if (sock->fd < 0) {
        lgx_vm_throw_s(vm, "socket alreay closed");
        return 1;
    }

    // 不能在多个协程中并发执行
    if (sock->co) {
        lgx_vm_throw_s(vm, "concurrent send");
        return 1;
    }
    sock->co = vm->co_running;

    sock->buffer = data->v.str->buffer;
    sock->length = data->v.str->length;
    sock->offset = 0;

    do_send(sock);
    return 0;
}

static wbt_status on_recv(wbt_event_t *ev);

static void do_recv(lgx_socket_t *sock) {
    lgx_vm_t *vm = sock->co->vm;
    lgx_co_t *co = sock->co;

    if (sock->length > sock->offset) {
        int nread = recv(sock->fd, sock->buffer + sock->offset,
            sock->length - sock->offset, 0);

        if (nread < 0) {
            wbt_err_t err = wbt_socket_errno;
            if (err == WBT_EAGAIN) {
                goto wait_recv;
            } else {
                sock->co = NULL;
                xfree(sock->buffer);
                sock->buffer= NULL;
                sock->length = sock->offset = 0;
                lgx_co_throw_s(co, strerror(err));
                return;
            }
        } else if (nread == 0) {
            sock->co = NULL;
            xfree(sock->buffer);
            sock->buffer= NULL;
            sock->length = sock->offset = 0;
            lgx_co_throw_s(co, "connection closed by peer");
            return;
        }

        wbt_debug("socket:recv %.*s", nread, sock->buffer + sock->offset);

        sock->offset += nread;
    }

    // 接收完毕
    lgx_str_t *str = lgx_str_new_ref(sock->buffer, sock->offset);
    str->size = sock->length;
    str->is_ref = 0;

    sock->co = NULL;
    lgx_co_return_string(co, str);
    return;

wait_recv:;
    // 阻塞等待
    wbt_event_t ev, *p_ev;
    memset(&ev, 0, sizeof(wbt_event_t));
    ev.timer.on_timeout = on_timeout;
    ev.timer.timeout    = wbt_cur_mtime + 30 * 1000;
    ev.on_recv = on_recv;
    ev.on_send = NULL;
    ev.events = WBT_EV_READ | WBT_EV_ET;
    ev.fd = sock->fd;

    if((p_ev = wbt_event_add(vm->events, &ev)) == NULL) {
        sock->co = NULL;
        lgx_co_throw_s(co, "wbt_event_add() failed");
        return;
    }

    p_ev->ctx = sock;

    lgx_co_suspend(vm);
}

static wbt_status on_recv(wbt_event_t *ev) {
    lgx_socket_t *sock = (lgx_socket_t *)ev->ctx;

    wbt_debug("socket:on_recv %d", ev->fd);

    wbt_event_del(sock->co->vm->events, ev);

    lgx_co_resume(sock->co->vm, sock->co);

    do_recv(sock);
    return WBT_OK;
}

LGX_METHOD(Socket, recv) {
    LGX_METHOD_ARGS_INIT();
    LGX_METHOD_ARGS_THIS(obj);
    LGX_METHOD_ARGS_GET(length, 0, T_LONG);

    lgx_val_t *res = lgx_obj_get_s(obj->v.obj, "ctx");
    if (!res || res->type != T_RESOURCE) {
        lgx_vm_throw_s(vm, "invalid property `ctx`");
        return 1;
    }

    lgx_socket_t *sock = (lgx_socket_t *)res->v.res->buf;
    if (sock->fd < 0) {
        lgx_vm_throw_s(vm, "socket alreay closed");
        return 1;
    }

    // 不能在多个协程中并发执行
    if (sock->co) {
        lgx_vm_throw_s(vm, "concurrent recv");
        return 1;
    }
    sock->co = vm->co_running;

    sock->buffer = xmalloc(length->v.l);
    sock->length = length->v.l;
    sock->offset = 0;
    if (!sock->buffer) {
        sock->co = NULL;
        lgx_vm_throw_s(vm, "xmalloc() failed");
        return 1;
    }

    do_recv(sock);
    return 0;
}

LGX_METHOD(Socket, close) {
    LGX_METHOD_ARGS_INIT();
    LGX_METHOD_ARGS_THIS(obj);

    lgx_val_t *res = lgx_obj_get_s(obj->v.obj, "ctx");
    if (!res || res->type != T_RESOURCE) {
        lgx_vm_throw_s(vm, "invalid property `ctx`");
        return 1;
    }

    lgx_socket_t *sock = (lgx_socket_t *)res->v.res->buf;
    if (sock->fd) {
        wbt_close_socket(sock->fd);
        sock->fd = -1;
    }

    LGX_RETURN_TRUE();
    return 0;
}

LGX_CLASS(Socket) {
    LGX_PROPERTY_BEGIN(Socket, ctx, T_RESOURCE, lgx_res_new(0, NULL))
        LGX_PROPERTY_ACCESS(P_PROTECTED)
    LGX_PROPERTY_END

    LGX_METHOD_BEGIN(Socket, constructor, 0)
        LGX_METHOD_RET(T_BOOL)
        LGX_METHOD_ACCESS(P_PUBLIC)
    LGX_METHOD_END

    LGX_METHOD_BEGIN(Socket, bind, 2)
        LGX_METHOD_RET(T_BOOL)
        LGX_METHOD_ARG(0, T_STRING)
        LGX_METHOD_ARG(1, T_LONG)
        LGX_METHOD_ACCESS(P_PUBLIC)
    LGX_METHOD_END

    LGX_METHOD_BEGIN(Socket, listen, 0)
        LGX_METHOD_RET(T_BOOL)
        LGX_METHOD_ACCESS(P_PUBLIC)
    LGX_METHOD_END

    LGX_METHOD_BEGIN(Socket, accept, 0)
        LGX_METHOD_RET_OBJECT(Socket)
        LGX_METHOD_ACCESS(P_PUBLIC)
    LGX_METHOD_END

    LGX_METHOD_BEGIN(Socket, connect, 2)
        LGX_METHOD_RET(T_BOOL)
        LGX_METHOD_ARG(0, T_STRING)
        LGX_METHOD_ARG(1, T_LONG)
        LGX_METHOD_ACCESS(P_PUBLIC)
    LGX_METHOD_END

    LGX_METHOD_BEGIN(Socket, send, 1)
        LGX_METHOD_RET(T_BOOL)
        LGX_METHOD_ARG(0, T_STRING)
        LGX_METHOD_ACCESS(P_PUBLIC)
    LGX_METHOD_END

    LGX_METHOD_BEGIN(Socket, recv, 1)
        LGX_METHOD_RET(T_STRING)
        LGX_METHOD_ARG(0, T_LONG)
        LGX_METHOD_ACCESS(P_PUBLIC)
    LGX_METHOD_END

    LGX_METHOD_BEGIN(Socket, close, 0)
        LGX_METHOD_RET(T_BOOL)
        LGX_METHOD_ACCESS(P_PUBLIC)
    LGX_METHOD_END

    return 0;
}

int std_net_socket_load_symbols(lgx_hash_t *hash) {
    LGX_CLASS_INIT(Socket);

    return 0;
}

lgx_buildin_ext_t ext_std_net_socket_ctx = {
    "std.net",
    std_net_socket_load_symbols
};