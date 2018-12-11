
#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "../common/str.h"
#include "../common/fun.h"
#include "../common/obj.h"
#include "../interpreter/coroutine.h"
#include "../webit/http/wbt_http.h"
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
    wbt_list_t head;
    lgx_server_t *server;
    wbt_event_t *ev;
} lgx_request_t;

typedef struct {
    lgx_server_t *server;
    wbt_http_request_t http;
    lgx_request_t req;
} lgx_conn_t;

static wbt_status on_accept(wbt_event_t *ev);
static wbt_status on_recv(wbt_event_t *ev);
static wbt_status on_send(wbt_event_t *ev);
static wbt_status on_close(wbt_event_t *ev);
static wbt_status on_timeout(wbt_timer_t *timer);

static int on_yield(lgx_vm_t *vm);

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

static wbt_status try_listen(wbt_socket_t fd, int port) {
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

static wbt_status on_close(wbt_event_t *ev) {
    wbt_debug("server:on_close %d", ev->fd);

    //wbt_debug("close time: %lld", wbt_cur_mtime);
    
    lgx_conn_t *conn = (lgx_conn_t *)ev->ctx;
    lgx_server_t *server = conn->server;

    wbt_event_del(server->vm->events, ev);

    if (wbt_list_empty(&conn->req.head)) {
        xfree(conn);
    } else {
        lgx_request_t *req;
        wbt_list_for_each_entry(req, lgx_request_t, &conn->req.head, head) {
            req->ev = NULL;
        }
        xfree(conn);
    }

    return WBT_OK;
}

static int on_yield(lgx_vm_t *vm) {
    lgx_co_t *co = vm->co_running;
    lgx_request_t *req = (lgx_request_t *)co->ctx;
    wbt_event_t *ev = req->ev;

    if (co->status != CO_DIED) {
        return 0;
    }

    if (!ev) {
        // 在协程返回前连接已经被关闭
        wbt_debug("server:on_yeild conn closed");
        xfree(req);
        return 0;
    } else {
        // 释放 request
        wbt_list_del(&req->head);
        xfree(req);
    }

    // 读取返回值
    if (vm->regs[1].type != T_STRING) {
        on_close(ev);
        return 1;
    }

    wbt_http_response_t resp;
    wbt_memset(&resp, 0, sizeof(wbt_http_response_t));
    resp.status = STATUS_200;
    wbt_http_generate_response_status(&resp);

    lgx_conn_t *conn = (lgx_conn_t *)ev->ctx;

    if (conn->http.keep_alive) {
        wbt_str_t keep_alive = wbt_string("keep-alive");
        wbt_http_generate_response_header(&resp, HEADER_CONNECTION, &keep_alive);
    } else {
        wbt_str_t close = wbt_string("close");
        wbt_http_generate_response_header(&resp, HEADER_CONNECTION, &close);
    }

    wbt_str_t body;
    body.str = vm->regs[1].v.str->buffer;
    body.len = vm->regs[1].v.str->length;
    wbt_http_generate_response_body(&resp, &body);

    //wbt_debug("%.*s", resp.send.offset, resp.send.buf);

    // 生成响应
    // 如果存在未发送完毕的响应，则追加到缓冲区末尾
    if (ev->send.buf) {
        while (ev->send.size < ev->send.len + resp.send.offset) {
            void *buf = xrealloc(ev->send.buf, ev->send.size * 2);
            if (!buf) {
                on_close(ev);
                return 1;
            }
            ev->send.buf = buf;
            ev->send.size *= 2;
        }
    } else {
        ev->send.buf = xmalloc(resp.send.offset);
        if (!ev->send.buf) {
            on_close(ev);
            return 1;
        }
        ev->send.size = resp.send.offset;
        ev->send.len = 0;
        ev->send.offset = 0;
    }
    memcpy(ev->send.buf + ev->send.len, resp.send.buf, resp.send.offset);
    ev->send.len += resp.send.offset;

    // 修改事件，每产生一个响应都会重置超时事件
    ev->events = WBT_EV_WRITE | WBT_EV_READ | WBT_EV_ET;
    ev->timer.timeout = wbt_cur_mtime + 30 * 1000;

    if(wbt_event_mod(vm->events, ev) != WBT_OK) {
        on_close(ev);
        return 1;
    }

    return 0;
}

static wbt_status on_recv(wbt_event_t *ev) {
    wbt_debug("server:on_recv %d", ev->fd);

    lgx_conn_t *conn = (lgx_conn_t *)ev->ctx;
    lgx_server_t *server = conn->server;

    if (ev->recv.buf == NULL) {
        ev->recv.buf = xmalloc(WBT_MAX_PROTO_BUF_LEN);
        ev->recv.size = WBT_MAX_PROTO_BUF_LEN;
        ev->recv.len = 0;
        ev->recv.offset = 0;
        
        if( ev->recv.buf == NULL ) {
            /* 内存不足 */
            on_close(ev);
            return WBT_OK;
        }
    }
    
    if (ev->recv.len >= WBT_MAX_PROTO_BUF_LEN) {
        /* 请求长度超过限制
         */
        wbt_debug("server: request length exceeds limitation", ev->fd);
        on_close(ev);
        return WBT_OK;
    }

    int nread = recv(ev->fd, (unsigned char *)ev->recv.buf + ev->recv.len,
        ev->recv.size - ev->recv.len, 0);

    if (nread < 0) {
        wbt_err_t err = wbt_socket_errno;
        if (err == WBT_EAGAIN) {
            // 当前缓冲区已无数据可读
        } else if (err == WBT_ECONNRESET) {
            // 对方发送了RST
            wbt_debug("server: connection reset by peer", ev->fd);
            on_close(ev);
            return WBT_OK;
        } else {
            // 其他不可弥补的错误
            wbt_debug("server: %d %d", err, ev->fd);
            on_close(ev);
            return WBT_OK;
        }
    } else if (nread == 0) {
        wbt_debug("server: connection closed by peer", ev->fd);
        on_close(ev);
        return WBT_OK;
    } else {
        ev->recv.len += nread;
    }

    // HTTP 协议解析
    conn->http.recv.buf = (char *)ev->recv.buf + ev->recv.offset;
    conn->http.recv.length = ev->recv.len - ev->recv.offset;

    wbt_status ret = wbt_http_parse_request(&conn->http);
    if (ret == WBT_AGAIN) {
        // 数据不完整，继续等待
        return WBT_OK;
    } else if (ret == WBT_ERROR) {
        // 数据不合法，关闭连接
        on_close(ev);
        return WBT_OK;
    }

    wbt_debug(
        "%s %.*s %.*s %u %d",
        wbt_http_method(conn->http.method),
        conn->http.uri.len, conn->http.recv.buf + conn->http.uri.start,
        conn->http.params.len, conn->http.recv.buf + conn->http.params.start,
        conn->http.keep_alive, conn->http.content_length
    );
    wbt_debug(
        "%.*s",
        conn->http.body.len, conn->http.recv.buf + conn->http.body.start
    );

    // 释放掉旧数据
    ev->recv.offset += conn->http.recv.offset;
    if (ev->recv.offset > ev->recv.size / 2) {
        wbt_memcpy(ev->recv.buf, ev->recv.buf + ev->recv.offset, ev->recv.len - ev->recv.offset);
        ev->recv.len -= ev->recv.offset;
        ev->recv.offset = 0;
    }

    // 收到完整的数据包则重置超时事件
    ev->timer.timeout = wbt_cur_mtime + 30 * 1000;

    if(wbt_event_mod(server->vm->events, ev) != WBT_OK) {
        on_close(ev);
        return WBT_OK;
    }

    lgx_request_t *req = (lgx_request_t *)xmalloc(sizeof(lgx_request_t));
    if (!req) {
        on_close(ev);
        return WBT_OK;
    }
    req->ev = ev;
    req->server = server;
    wbt_list_add_tail(&req->head, &conn->req.head);

    // 调用 xscript
    lgx_co_t *co = lgx_co_create(server->vm, server->on_request);
    if (!co) {
        xfree(req);
        on_close(ev);
        return WBT_OK;
    }

    co->on_yield = on_yield;
    co->ctx = req;

    // 写入参数
    lgx_co_set_object(co, 4, server->obj);
    lgx_co_set_string(co, 5, lgx_str_new_ref(conn->http.recv.buf, conn->http.recv.offset));

    return WBT_OK;
}

static wbt_status on_send(wbt_event_t *ev) {
    wbt_debug("server:on_send %d", ev->fd);

    lgx_conn_t *conn = (lgx_conn_t *)ev->ctx;
    lgx_server_t *server = conn->server;

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
            // TODO 清理已发送的缓冲区
            return WBT_OK;
        }
    }

    // 发送完毕
    ev->send.len = 0;
    ev->send.offset = 0;

    if (conn->http.keep_alive) {
        // 修改事件
        ev->events = WBT_EV_READ | WBT_EV_ET;
        ev->timer.timeout = wbt_cur_mtime + 30 * 1000;

        if (wbt_event_mod(server->vm->events, ev) != WBT_OK) {
            on_close(ev);
            return WBT_OK;
        }

        wbt_memset(&conn->http, 0, sizeof(wbt_http_request_t));
    } else {
        on_close(ev);
    }

    return WBT_OK;
}

static wbt_status on_timeout(wbt_timer_t *timer) {
    wbt_event_t *ev = wbt_timer_entry(timer, wbt_event_t, timer);

    wbt_debug("server:on_timeout %d", ev->fd);

    return on_close(ev);
}

static wbt_status on_accept(wbt_event_t *ev) {
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
        wbt_debug("server:on_accept %d", conn_sock);

        if (server->pool) {
            wbt_thread_t *worker = wbt_thread_get(server->pool);
            if (wbt_thread_send(worker, conn_sock) != 0) {
                wbt_close_socket(conn_sock);
                continue;
            }
        } else {
            lgx_conn_t *conn = (lgx_conn_t *)xmalloc(sizeof(lgx_conn_t));
            if (!conn) {
                wbt_close_socket(conn_sock);
                continue;
            }
            conn->server = server;
            wbt_memset(&conn->http, 0, sizeof(wbt_http_request_t));
            wbt_list_init(&conn->req.head);

            wbt_event_t *p_ev, tmp_ev;
            tmp_ev.timer.on_timeout = on_timeout;
            tmp_ev.timer.timeout    = wbt_cur_mtime + 30 * 1000;
            tmp_ev.on_recv = on_recv;
            tmp_ev.on_send = on_send;
            tmp_ev.events  = WBT_EV_READ | WBT_EV_ET;
            tmp_ev.fd      = conn_sock;

            //wbt_debug("accept time: %lld", wbt_cur_mtime);

            if((p_ev = wbt_event_add(server->vm->events, &tmp_ev)) == NULL) {
                xfree(conn);
                wbt_close_socket(conn_sock);
                continue;
            }

            p_ev->ctx = conn;
        }
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

static void* worker(void *args) {
    wbt_thread_t *thread = (wbt_thread_t *)args;
    lgx_server_t *master = (lgx_server_t *)thread->ctx;

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    if (pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0) {
        int i;
        for (i = 0; i < master->pool->size; i++) {
            if (CPU_ISSET(i, &cpuset)) {
                wbt_debug("CPU %d", i);
            }
        }
    }

    lgx_vm_t vm;
    // TODO 常量表、异常表、字节码缓存是共享的，这可能会导致问题
    lgx_vm_init(&vm, master->vm->bc);

    lgx_co_suspend(&vm);

    lgx_server_t server;
    server.vm = &vm;
    server.on_request = master->on_request;
    server.pool = NULL;
    // TODO 这里需要完整复制对象，否则多线程访问会出错
    server.obj = lgx_obj_new(master->obj->parent);

    time_t timeout = 0;

    while (!wbt_wating_to_exit) {
        lgx_vm_execute(&vm);

        if (!lgx_co_has_task(&vm)) {
            // 全部协程均执行完毕
            break;
        }

        if (vm.co_running) {
            continue;
        } else if (lgx_co_has_ready_task(&vm)) {
            lgx_co_schedule(&vm);
        } else {
            timeout = wbt_timer_process(&vm.events->timer);
            wbt_event_wait(vm.events, timeout);
            timeout = wbt_timer_process(&vm.events->timer);

            wbt_event_t *p_ev, tmp_ev;
            tmp_ev.on_recv = on_recv;
            tmp_ev.on_send = on_send;
            tmp_ev.events  = WBT_EV_READ | WBT_EV_ET;
            tmp_ev.timer.on_timeout = on_timeout;
            tmp_ev.timer.timeout    = wbt_cur_mtime + 30 * 1000;
            while (wbt_thread_recv(thread, &tmp_ev.fd) == 0) {
                if((p_ev = wbt_event_add(vm.events, &tmp_ev)) == NULL) {
                    wbt_close_socket(tmp_ev.fd);
                    continue;
                }

                lgx_conn_t *conn = (lgx_conn_t *)xmalloc(sizeof(lgx_conn_t));
                if (!conn) {
                    wbt_close_socket(tmp_ev.fd);
                    continue;
                }
                conn->server = &server;
                wbt_memset(&conn->http, 0, sizeof(wbt_http_request_t));
                wbt_list_init(&conn->req.head);

                p_ev->ctx = conn;
            }
        }
    }

    lgx_obj_delete(server.obj);

    lgx_vm_cleanup(&vm);

    return NULL;
}

static int server_listen(void *p) {
    lgx_vm_t *vm = (lgx_vm_t *)p;

    unsigned base = vm->regs[0].v.fun->stack_size;

    lgx_val_t *obj = &vm->regs[base+4];
    lgx_val_t *port = &vm->regs[base+5];

    if (port->type != T_LONG || port->v.l <= 0 || port->v.l >= 65535) {
        lgx_vm_throw_s(vm, "invalid param `port`");
        return 1;
    }

    // 创建监听端口
    wbt_socket_t fd = create_fd();
    if (fd < 0) {
        lgx_vm_throw_s(vm, "create socket failed");
        return 1;
    }

    if (try_listen(fd, port->v.l) != WBT_OK) {
        wbt_close_socket(fd);
        lgx_vm_throw_s(vm, "listen failed");
        return 1;
    }

    // 把 fd 记录到对象的 fd 属性中
    lgx_val_t k, v;
    k.type = T_STRING;
    k.v.str = lgx_str_new_ref("fd", sizeof("fd")-1); // TODO memory leak
    v.type = T_LONG;
    v.v.l = fd;
    if (lgx_obj_set(obj->v.obj, &k, &v) != 0) {
        wbt_close_socket(fd);
        lgx_vm_throw_s(vm, "set property `fd` failed");
        return 1;
    }

    lgx_co_return_undefined(vm->co_running);

    return 0;
}

static int server_on_request(void *p) {
    lgx_vm_t *vm = (lgx_vm_t *)p;

    return lgx_co_return_undefined(vm->co_running);
}

static int server_start(void *p) {
    lgx_vm_t *vm = (lgx_vm_t *)p;

    unsigned base = vm->regs[0].v.fun->stack_size;

    lgx_val_t *obj = &vm->regs[base+4];
    lgx_val_t *worker_num = &vm->regs[base+5];

    if (worker_num->type != T_LONG || worker_num->v.l < 0 || worker_num->v.l > 128) {
        lgx_vm_throw_s(vm, "invalid param `ip`");
        return 1;
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
        lgx_vm_throw_s(vm, "invalid property `fd`");
        return 1;
    }

    name.buffer = "onRequest";
    name.length = sizeof("onRequest")-1;
    lgx_val_t *on_req = lgx_obj_get(obj->v.obj, &k);
    if (!on_req || on_req->type != T_FUNCTION || on_req->v.fun == NULL || on_req->v.fun->buildin) {
        lgx_vm_throw_s(vm, "invalid method `onRequest`");
        return 1;
    }

    // 把监听的 socket 加入事件循环
    wbt_event_t ev, *p_ev;
    memset(&ev, 0, sizeof(wbt_event_t));

    ev.on_recv = on_accept;
    ev.on_send = NULL;
    ev.events = WBT_EV_READ | WBT_EV_ET;
    ev.fd = fd->v.l;

    if((p_ev = wbt_event_add(vm->events, &ev)) == NULL) {
        wbt_close_socket(fd->v.l);
        lgx_vm_throw_s(vm, "add event failed");
        return 1;
    }

    lgx_server_t *server = (lgx_server_t *)xmalloc(sizeof(lgx_server_t));
    if (!server) {
        wbt_close_socket(fd->v.l);
        lgx_vm_throw_s(vm, "out of memory");
        return 1;
    }

    server->vm = vm;
    server->obj = obj->v.obj;
    server->on_request = on_req->v.fun;
    if (worker_num->v.l) {
        server->pool = wbt_thread_create_pool(worker_num->v.l, worker, server);
    } else {
        server->pool = NULL;
    }

    p_ev->ctx = server;

    lgx_co_return_undefined(vm->co_running);

    lgx_co_suspend(vm);

    return 0;
}

static int std_net_tcp_server_load_symbols(lgx_hash_t *hash) {
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
    symbol_fd.v.u.c.modifier.access = P_PROTECTED;

    if (lgx_obj_add_property(symbol.v.obj, &symbol_fd)) {
        return 1;
    }

    lgx_hash_node_t symbol_listen;
    symbol_listen.k.type = T_STRING;
    symbol_listen.k.v.str = lgx_str_new_ref("listen", sizeof("listen")-1);
    symbol_listen.v.type = T_FUNCTION;
    symbol_listen.v.v.fun = lgx_fun_new(1);
    symbol_listen.v.v.fun->buildin = server_listen;
    symbol_listen.v.v.fun->args[0].type = T_LONG;

    if (lgx_obj_add_method(symbol.v.obj, &symbol_listen)) {
        return 1;
    }

    lgx_hash_node_t symbol_on_request;
    symbol_on_request.k.type = T_STRING;
    symbol_on_request.k.v.str = lgx_str_new_ref("onRequest", sizeof("onRequest")-1);
    symbol_on_request.v.type = T_FUNCTION;
    symbol_on_request.v.v.fun = lgx_fun_new(2);
    symbol_on_request.v.v.fun->buildin = server_on_request;
    symbol_on_request.v.v.fun->args[0].type = T_OBJECT; // TODO 编译时需要能够检查函数原型
    symbol_on_request.v.v.fun->args[1].type = T_STRING;

    if (lgx_obj_add_method(symbol.v.obj, &symbol_on_request)) {
        return 1;
    }

    lgx_hash_node_t symbol_create;
    symbol_create.k.type = T_STRING;
    symbol_create.k.v.str = lgx_str_new_ref("start", sizeof("start")-1);
    symbol_create.v.type = T_FUNCTION;
    symbol_create.v.v.fun = lgx_fun_new(1);
    symbol_create.v.v.fun->buildin = server_start;
    symbol_create.v.v.fun->args[0].type = T_LONG;

    if (lgx_obj_add_method(symbol.v.obj, &symbol_create)) {
        return 1;
    }

    if (lgx_ext_add_symbol(hash, symbol.v.obj->name->buffer, &symbol)) {
        return 1;
    }

    return 0;
}

lgx_buildin_ext_t ext_std_net_tcp_server_ctx = {
    "std.net.tcp",
    std_net_tcp_server_load_symbols
};