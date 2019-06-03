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

lgx_function_t* lgx_fucntion_new() {
    lgx_function_t* f = xcalloc(1, sizeof(lgx_function_t));
    return f;
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
            printf("\"");
            lgx_str_print(&v->v.str->string);
            printf("\"");
            break;
        case T_FUNCTION:
            lgx_str_print(&v->v.fun->name);
            printf("()");
            break;
        default:
            printf("unknwon");
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