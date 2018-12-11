
#include "../common/obj.h"
#include "std_exception.h"

LGX_CLASS(Exception) {
    return 0;
}

int std_exception_load_symbols(lgx_hash_t *hash) {
    LGX_CLASS_INIT(Exception);

    return 0;
}

lgx_buildin_ext_t ext_std_exception_ctx = {
    "std.exception",
    std_exception_load_symbols
};