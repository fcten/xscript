#include "../common/str.h"
#include "../common/fun.h"
#include "../common/obj.h"
#include "std_net_http_server.h"

static int std_net_http_server_load_symbols(lgx_hash_t *hash) {
    lgx_val_t *parent = lgx_ext_get_symbol(hash, "Server");
    if (!parent || parent->type != T_OBJECT) {
        return 1;
    }

    lgx_str_t name;
    name.buffer = "HttpServer";
    name.length = sizeof("HttpServer")-1;

    lgx_val_t symbol;
    symbol.type = T_OBJECT;
    symbol.v.obj = lgx_obj_create(&name);
    symbol.v.obj->parent = parent->v.obj;

    if (lgx_ext_add_symbol(hash, symbol.v.obj->name->buffer, &symbol)) {
        return 1;
    }

    return 0;
}

lgx_buildin_ext_t ext_std_net_http_server_ctx = {
    "std.net.http",
    std_net_http_server_load_symbols
};