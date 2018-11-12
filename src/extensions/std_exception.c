
#include "../common/obj.h"
#include "std_exception.h"

int std_exception_load_symbols(lgx_hash_t *hash) {
    lgx_str_t name;
    name.buffer = "Exception";
    name.length = sizeof("Exception")-1;

    lgx_val_t symbol;
    symbol.type = T_OBJECT;
    symbol.v.obj = lgx_obj_create(&name);

    if (lgx_ext_add_symbol(hash, symbol.v.obj->name->buffer, &symbol)) {
        return 1;
    }

    return 0;
}

lgx_buildin_ext_t ext_std_exception_ctx = {
    "std.exception",
    std_exception_load_symbols
};