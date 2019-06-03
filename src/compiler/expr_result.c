#include "expr_result.h"

int lgx_expr_to_value(lgx_expr_result_t* e, lgx_value_t* v) {
    v->type = e->v_type.type;

    switch (v->type) {
    case T_LONG:
        v->v.l = e->v.l;
        break;
    case T_DOUBLE:
        v->v.d = e->v.d;
        break;
    case T_STRING:
        v->v.str = lgx_string_new();
        if (!v->v.str) {
            return 1;
        }
        if (lgx_str_init(&v->v.str->string, e->v.str.length)) {
            xfree(v->v.str);
            v->v.str = NULL;
            return 1;
        }
        lgx_str_dup(&e->v.str, &v->v.str->string);
        break;
    case T_BOOL:
        v->v.l = e->v.l;
        break;
    // case T_ARRAY:
    // TODO 其他类型
    default:
        break;
    }

    return 0;
}