#include "../common/hash.h"
#include "../common/str.h"
#include "ext.h"

#include "std_time.h"
#include "std_math.h"
#include "std_string.h"
#include "std_array.h"
#include "std_coroutine.h"
#include "std_exception.h"
#include "net_socket.h"
#include "net_http.h"
#include "net_tcp_server.h"
#include "net_tcp_client.h"

#include "redis.h"

lgx_buildin_ext_t *buildin_exts[] = {
    &ext_std_time_ctx,
    &ext_std_math_ctx,
    &ext_std_string_ctx,
    &ext_std_array_ctx,
    &ext_std_coroutine_ctx,
    &ext_std_exception_ctx,
    &ext_net_socket_ctx,
    &ext_net_http_ctx,
    &ext_net_tcp_server_ctx,
    &ext_net_tcp_client_ctx,
    &ext_redis_ctx
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

    // 扩展只应该导入 类/函数/常量，与 import 的规则一致
    n.v.u.c.modifier.is_const = 1;

    if (lgx_hash_get(hash, &n.k)) {
        // 符号已存在
        return 1;
    }

    lgx_hash_set(hash, &n);

    return 0;
}

lgx_val_t* lgx_ext_get_symbol(lgx_hash_t *hash, char *symbol) {
    lgx_str_t s;
    s.buffer = symbol;
    s.length = strlen(symbol);

    lgx_val_t k;
    k.type = T_STRING;
    k.v.str = &s;

    lgx_hash_node_t *n = lgx_hash_get(hash, &k);
    if (!n) {
        return NULL;
    } else {
        return &n->v;
    }
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
