#include "../common/obj.h"
#include "../common/str.h"
#include "../common/fun.h"
#include "std_exception.h"

LGX_METHOD(Exception, constructor) {
    LGX_METHOD_ARGS_INIT();
    //LGX_METHOD_ARGS_THIS(obj);
    LGX_METHOD_ARGS_GET(message, 0, T_STRING);

    // TODO 记录当前执行代码的位置

    // TODO 如果存在调试信息，寻找对应的源代码位置

    LGX_RETURN_TRUE();
    return 0;
}

LGX_CLASS(Exception) {
    LGX_METHOD_BEGIN(Exception, constructor, 1)
        LGX_METHOD_RET(T_BOOL)
        LGX_METHOD_ARG(0, T_STRING)
        LGX_METHOD_ACCESS(P_PUBLIC)
    LGX_METHOD_END

    return 0;
}

int std_exception_load_symbols(lgx_hash_t *hash) {
    LGX_CLASS_INIT(Exception);

    return 0;
}

lgx_buildin_ext_t ext_std_exception_ctx = {
    wbt_string("std.exception"),
    std_exception_load_symbols
};