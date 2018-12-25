#include <sys/time.h>

#include "../common/str.h"
#include "../common/fun.h"
#include "std_time.h"

LGX_FUNCTION(time) {
    LGX_FUNCTION_ARGS_INIT();
    LGX_FUNCTION_ARGS_GET(type, 0, T_LONG);

    struct timeval tv;
    gettimeofday(&tv, NULL);

    if (type->v.l == 0) {
        LGX_RETURN_LONG(tv.tv_sec);
    } else {
        LGX_RETURN_LONG(tv.tv_sec*1000 + tv.tv_usec/1000);
    }
    
    return 0;
}

int std_time_load_symbols(lgx_hash_t *hash) {
    LGX_FUNCTION_BEGIN(time, 1)
        LGX_FUNCTION_RET(T_LONG)
        LGX_FUNCTION_ARG_OPTIONAL(0, T_LONG, 0)
    LGX_FUNCTION_END

    return 0;
}

lgx_buildin_ext_t ext_std_time_ctx = {
    wbt_string("std.time"),
    std_time_load_symbols
};