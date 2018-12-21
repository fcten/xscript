#include "../common/str.h"
#include "../common/fun.h"
#include "../common/obj.h"
#include "net_http.h"

LGX_METHOD(Http, constructor) {
    LGX_METHOD_ARGS_INIT();
    //LGX_METHOD_ARGS_THIS(obj);
    LGX_METHOD_ARGS_GET(conn, 0, T_OBJECT);

    LGX_RETURN_TRUE();
    return 0;
}

LGX_CLASS(Http) {
    LGX_METHOD_BEGIN(Http, constructor, 1)
        LGX_METHOD_RET(T_BOOL)
        LGX_METHOD_ARG_OBJECT(0, Socket)
        LGX_METHOD_ACCESS(P_PUBLIC)
    LGX_METHOD_END

    return 0;
}

int net_http_load_symbols(lgx_hash_t *hash) {
    LGX_CLASS_INIT(Http);

    return 0;
}

lgx_buildin_ext_t ext_net_http_ctx = {
    "net.http",
    net_http_load_symbols
};