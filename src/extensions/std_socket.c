#include "../common/str.h"
#include "../common/fun.h"
#include "../interpreter/coroutine.h"
#include "std_coroutine.h"

wbt_status std_socket_on_accept(wbt_event_t *ev) {
    printf("test\n");
    return WBT_OK;
}

int std_socket_server_create(void *p) {
    lgx_vm_t *vm = p;

    unsigned base = vm->regs[0].v.fun->stack_size;
    long long port = vm->regs[base+4].v.l;

    // 创建监听端口并加入事件循环
    wbt_socket_t fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd <= 0) {
        //wbt_log_add("create socket failed\n");
        return lgx_ext_return_false(vm);
    }
    /* 把监听socket设置为非阻塞方式 */
    if( wbt_nonblocking(fd) == -1 ) {
        //wbt_log_add("set nonblocking failed\n");
        return lgx_ext_return_false(vm);
    }

    /* 在重启程序以及进行热更新时，避免 TIME_WAIT 和 CLOSE_WAIT 状态的连接导致 bind 失败 */
    int on = 1; 
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) != 0) {  
        //wbt_log_add("set SO_REUSEADDR failed\n");  
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
        return lgx_ext_return_false(vm);
    }

    if(listen(fd, WBT_CONN_BACKLOG) != 0) {
        //wbt_log_add("listen failed\n");
        return lgx_ext_return_false(vm);
    }

    wbt_event_t ev;
    memset(&ev, 0, sizeof(wbt_event_t));

    ev.on_recv = std_socket_on_accept;
    ev.on_send = NULL;
    ev.events = WBT_EV_READ | WBT_EV_ET;
    ev.fd = fd;

    if(wbt_event_add(vm->events, &ev) == NULL) {
        wbt_close_socket(fd);
        return lgx_ext_return_false(vm);
    }

    return lgx_ext_return_true(vm);
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
    symbol.v.fun = lgx_fun_new(1);
    symbol.v.fun->buildin = std_socket_server_create;

    symbol.v.fun->args[0].type = T_LONG;

    if (lgx_ext_add_symbol(hash, "server_create", &symbol)) {
        return 1;
    }

    return 0;
}

lgx_buildin_ext_t ext_std_socket_ctx = {
    "std.socket",
    std_socket_load_symbols
};