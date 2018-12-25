#include <stdlib.h>
#include <time.h>

#include "../common/str.h"
#include "../common/fun.h"
#include "std_math.h"

LGX_FUNCTION(rand) {
    LGX_RETURN_LONG(rand());
    return 0;
}

int std_math_load_symbols(lgx_hash_t *hash) {
    // 初始化随机数种子
    srand((unsigned)time(NULL));

    LGX_FUNCTION_BEGIN(rand, 0)
        LGX_FUNCTION_RET(T_LONG)
    LGX_FUNCTION_END

    return 0;
}

lgx_buildin_ext_t ext_std_math_ctx = {
    wbt_string("std.math"),
    std_math_load_symbols
};