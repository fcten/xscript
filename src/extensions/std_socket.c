#include "../common/str.h"
#include "../common/fun.h"
#include "std_coroutine.h"

int std_socket_server_create(void *p) {
    lgx_vm_t *vm = p;

    unsigned base = vm->regs[0].v.fun->stack_size;
    long long port = vm->regs[base+4].v.l;

    // 创建监听端口并加入事件循环

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