#include "../common/fun.h"
#include "../common/str.h"
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

LGX_FUNCTION(to_string) {
    LGX_FUNCTION_ARGS_INIT();
    LGX_FUNCTION_ARGS_GET(from, 0, T_UNDEFINED);

    lgx_str_t *ret;
    // 50 字节应当足够保存任意 64 位数字类型
    char buff[50];
    
    switch (from->type) {
    case T_UNDEFINED:
        ret = lgx_str_new("undefined", sizeof("undefined") - 1);
        break;
    case T_LONG:
        sprintf(buff, "%lld", from->v.l);
        ret = lgx_str_new(buff, strlen(buff));
        break;
    case T_DOUBLE:
        sprintf(buff, "%f", from->v.d);
        ret = lgx_str_new(buff, strlen(buff));
        break;
    case T_BOOL:
        if (from->v.l) {
            ret = lgx_str_new("true", sizeof("true") - 1);
        } else {
            ret = lgx_str_new("false", sizeof("false") - 1);
        }
        break;
    default:
        lgx_vm_throw_s(vm, "invalid param type for to_string");
        return 1;
    }

    LGX_RETURN_STRING(ret);
    
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

    LGX_FUNCTION_BEGIN(to_string, 1)
        LGX_FUNCTION_RET(T_STRING)
        LGX_FUNCTION_ARG(0, T_UNDEFINED)
    LGX_FUNCTION_END

    return 0;
}

lgx_buildin_ext_t ext_std_string_ctx = {
    wbt_string("std.string"),
    std_string_load_symbols
};