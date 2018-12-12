#include "../common/fun.h"
#include "std_string.h"

/* @name string_length
 * 
 * @description 返回字符串长度
 * 
 * @param arrval T_STRING
 * 
 * @return T_LONG
 */
LGX_FUNCTION(string_length) {
    LGX_FUNCTION_ARGS_INIT();
    LGX_FUNCTION_ARGS_GET(strval, 0);

    LGX_RETURN_LONG(strval->v.str->length);
    return 0;
}

int std_string_load_symbols(lgx_hash_t *hash) {
    LGX_FUNCTION_BEGIN(string_length, 1)
        LGX_FUNCTION_RET(T_LONG)
        LGX_FUNCTION_ARG(0, T_STRING)
    LGX_FUNCTION_END

    return 0;
}

lgx_buildin_ext_t ext_std_string_ctx = {
    "std.string",
    std_string_load_symbols
};