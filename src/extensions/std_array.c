#include "../common/fun.h"
#include "std_array.h"

LGX_FUNCTION(array_length) {
    LGX_FUNCTION_ARGS_INIT();
    LGX_FUNCTION_ARGS_GET(arrval, 0, T_ARRAY);

    LGX_RETURN_LONG(arrval->v.arr->length);
    return 0;
}

int std_array_load_symbols(lgx_hash_t *hash) {
    LGX_FUNCTION_BEGIN(array_length, 1)
        LGX_FUNCTION_RET(T_LONG)
        LGX_FUNCTION_ARG(0, T_ARRAY)
    LGX_FUNCTION_END

    return 0;
}

lgx_buildin_ext_t ext_std_array_ctx = {
    "std.array",
    std_array_load_symbols
};