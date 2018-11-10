#include "../common/hash.h"
#include "../common/str.h"
#include "ext.h"

#include "std_time.h"
#include "std_math.h"
#include "std_coroutine.h"
#include "std_net_tcp_server.h"
#include "std_net_tcp_client.h"

lgx_buildin_ext_t *buildin_exts[] = {
    &ext_std_time_ctx,
    &ext_std_math_ctx,
    &ext_std_coroutine_ctx,
    &ext_std_net_tcp_server_ctx,
    &ext_std_net_tcp_client_ctx
};

int lgx_ext_init(lgx_vm_t *vm) {
//    lgx_extensions = lgx_hash_new(32);

    return 0;
}

int lgx_ext_add_symbol(lgx_hash_t *hash, char *symbol, lgx_val_t *v) {
    lgx_hash_node_t n;
    n.k.type = T_STRING;
    n.k.v.str = lgx_str_new_ref(symbol, strlen(symbol));
    n.v = *v;
    n.v.u.c.used = 1;

    if (lgx_hash_get(hash, &n.k)) {
        // 符号已存在
        return 1;
    }

    lgx_hash_set(hash, &n);

    return 0;
}

int lgx_ext_load_symbols(lgx_hash_t *hash) {
    // 从内建扩展中加载符号
    int i;
    for (i = 0; i < sizeof(buildin_exts)/sizeof(lgx_buildin_ext_t*); i++) {
        if (buildin_exts[i]->load_symbols) {
            if (buildin_exts[i]->load_symbols(hash)) {
                return 1;
            }
        }
    }

    return 0;
}

int lgx_ext_return(lgx_co_t *co, lgx_val_t *v) {
    // 参数起始地址
    int base = co->stack.buf[0].v.fun->stack_size;

    // 返回值地址
    int ret = co->stack.buf[base + 1].v.l;

    // 写入返回值
    lgx_gc_ref_del(&co->stack.buf[ret]);
    co->stack.buf[ret] = *v;
    lgx_gc_ref_add(&co->stack.buf[ret]);

    return 0;
}

int lgx_ext_return_long(lgx_co_t *co, long long v) {
    lgx_val_t ret;
    ret.type = T_LONG;
    ret.v.l = v;

    return lgx_ext_return(co, &ret);
}

int lgx_ext_return_double(lgx_co_t *co, double v) {
    lgx_val_t ret;
    ret.type = T_DOUBLE;
    ret.v.d = v;

    return lgx_ext_return(co, &ret);
}

int lgx_ext_return_true(lgx_co_t *co) {
    lgx_val_t ret;
    ret.type = T_BOOL;
    ret.v.d = 1;

    return lgx_ext_return(co, &ret);
}

int lgx_ext_return_false(lgx_co_t *co) {
    lgx_val_t ret;
    ret.type = T_BOOL;
    ret.v.d = 0;

    return lgx_ext_return(co, &ret);
}

int lgx_ext_return_string(lgx_co_t *co, lgx_str_t *str) {
    lgx_val_t ret;
    ret.type = T_STRING;
    ret.v.str = str;

    return lgx_ext_return(co, &ret);
}