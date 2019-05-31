#include "value.h"

int lgx_value_init(lgx_value_t* v) {
    memset(v, 0, sizeof(lgx_value_t));
    return 0;
}

// TODO
void lgx_value_cleanup(lgx_value_t* v) {
    switch (v->type) {
        case T_FUNCTION:
            lgx_str_cleanup(&v->v.fun->name);
            xfree(v->v.fun);
            break;
        default:
            break;
    }
}

lgx_string_t* lgx_string_new() {
    lgx_string_t* s = xcalloc(1, sizeof(lgx_string_t));
    return s;
}

int lgx_value_dup(lgx_value_t* src, lgx_value_t* dst) {
    switch (src->type) {
        case T_LONG:
        case T_BOOL:
            dst->type = src->type;
            dst->v.l = src->v.l;
            break;
        case T_DOUBLE:
            dst->type = src->type;
            dst->v.d = src->v.d;
            break;
        case T_STRING:
            dst->type = src->type;
            dst->v.str = lgx_string_new();
            if (!dst->v.str) {
                return 1;
            }
            lgx_str_init(&dst->v.str->string, src->v.str->string.length);
            lgx_str_dup(&src->v.str->string, &dst->v.str->string);
            break;
        default:
            return 1;
    }
    return 0;
}

void lgx_value_print(lgx_value_t* v) {
   switch (v->type) {
        case T_LONG:
            printf("%lld", v->v.l);
            break;
        case T_DOUBLE:
            printf("%lg", v->v.d);
            break;
        case T_BOOL:
            if (v->v.l) {
                printf("true");
            } else {
                printf("false");
            }
            break;
        case T_STRING:
            lgx_str_print(&v->v.str->string);
            break;
        default:
            break;
    }
}

char* lgx_value_typeof(lgx_value_t* v) {
    switch (v->type) {
        case T_UNKNOWN:
            return "unknown";
        case T_LONG:
            return "integer";
        case T_DOUBLE:
            return "float";
        case T_BOOL:
            return "boolean";
        default:
            return lgx_type_to_string(&v->v.gc->type);
    }
}

int lgx_value_cmp(lgx_value_t* v1, lgx_value_t* v2) {
    return 0;
}