#include "../common/fun.h"
#include "std_string.h"

LGX_FUNCTION(string_length) {
    LGX_FUNCTION_ARGS_INIT();
    LGX_FUNCTION_ARGS_GET(strval, 0, T_STRING);

    LGX_RETURN_LONG(strval->v.str->length);
    return 0;
}

LGX_FUNCTION(string_char_at) {
    LGX_FUNCTION_ARGS_INIT();
    LGX_FUNCTION_ARGS_GET(strval, 0, T_STRING);
    LGX_FUNCTION_ARGS_GET(posval, 1, T_LONG);

    if (posval->v.l < 0 || posval->v.l >= strval->v.str->length) {
        LGX_RETURN_LONG(-1);
    } else {
        LGX_RETURN_LONG((unsigned)strval->v.str->buffer[posval->v.l]);
    }
    return 0;
}

int std_string_load_symbols(lgx_hash_t *hash) {
    LGX_FUNCTION_BEGIN(string_length, 1)
        LGX_FUNCTION_RET(T_LONG)
        LGX_FUNCTION_ARG(0, T_STRING)
    LGX_FUNCTION_END

    LGX_FUNCTION_BEGIN(string_char_at, 2)
        LGX_FUNCTION_RET(T_LONG)
        LGX_FUNCTION_ARG(0, T_STRING)
        LGX_FUNCTION_ARG(1, T_LONG)
    LGX_FUNCTION_END

    return 0;
}

lgx_buildin_ext_t ext_std_string_ctx = {
    wbt_string("std.string"),
    std_string_load_symbols
};