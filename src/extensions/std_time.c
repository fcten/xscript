#include <time.h>

#include "../common/str.h"
#include "../common/fun.h"
#include "std_time.h"

LGX_FUNCTION(time) {
    LGX_RETURN_LONG(time(NULL));
    return 0;
}

int std_time_load_symbols(lgx_hash_t *hash) {
    LGX_FUNCTION_BEGIN(time, 0)
        LGX_FUNCTION_RET(T_LONG, 0)
    LGX_FUNCTION_END

    return 0;
}

lgx_buildin_ext_t ext_std_time_ctx = {
    "std.time",
    std_time_load_symbols
};