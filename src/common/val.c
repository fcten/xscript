#include "common.h"
#include "val.h"
#include "str.h"
#include "fun.h"
#include "obj.h"
#include "res.h"
#include "hash.h"

struct type {
    int type;
    char* s;
};

static const struct type lgx_val_types[] = {
    {T_UNDEFINED, "undefined"},
    {T_LONG, "integer"},
    {T_DOUBLE, "float"},
    {T_BOOL, "boolean"},
    {T_REDERENCE, "reference"},
    {T_STRING, "string"},
    {T_ARRAY, "array"},
    {T_OBJECT, "object"},
    {T_FUNCTION, "function"},
    {T_RESOURCE, "resource"}};

const char *lgx_val_typeof(lgx_val_t *v) {
    if (v->type >= T_UNDEFINED && v->type <= T_RESOURCE) {
        if (v->type == T_OBJECT && v->v.obj && v->v.obj->is_interface) {
            return "interface";
        } else {
            return lgx_val_types[v->type].s;
        }
    } else {
        return lgx_val_types[T_UNDEFINED].s;
    }
}


void lgx_val_print(lgx_val_t *v, int deep) {
    switch (v->type) {
        case T_UNDEFINED:
            printf("<%s>", lgx_val_typeof(v));
            break;
        case T_LONG:
            //printf("<%s> %lld", lgx_val_typeof(v), v->v.l);
            printf("%lld", v->v.l);
            break;
        case T_DOUBLE:
            //printf("<%s> %lg", lgx_val_typeof(v), v->v.d);
            printf("%lg", v->v.d);
            break;
        case T_BOOL:
            if (v->v.l) {
                //printf("<%s> true\n", lgx_val_typeof(v));
                printf("true");
            } else {
                //printf("<%s> false\n", lgx_val_typeof(v));
                printf("false");
            }
            break;
        case T_STRING:
            //printf("<%s(%u)> \"%.*s\"\n", lgx_val_typeof(v), v->v.str->length, v->v.str->length, v->v.str->buffer);
            lgx_str_print(v->v.str);
            break;
        case T_ARRAY:
            //printf("<%s>\n", lgx_val_typeof(v));
            lgx_hash_print(v->v.arr, deep + 1);
            break;
        case T_OBJECT:
            lgx_obj_print(v->v.obj, deep + 1);
            break;
        case T_RESOURCE:
            printf("<%s>", lgx_val_typeof(v));
            break;
        case T_REDERENCE:
            printf("<%s>", lgx_val_typeof(v));
            break;
        case T_FUNCTION:
            printf("<%s addr:%u>", lgx_val_typeof(v), v->v.fun->addr);
            break;
        default:
            printf("<error-type>\n");
            break;
    }
}


// 相等返回 1，不相等返回 0
int lgx_val_cmp(lgx_val_t *src, lgx_val_t *dst) {
    if (src->type == dst->type) {
        switch (src->type) {
            case T_UNDEFINED:
                return 1;
            case T_LONG:
                return src->v.l == dst->v.l;
            case T_DOUBLE:
                return src->v.d == dst->v.d;
            case T_BOOL:
                if ( (src->v.l && dst->v.l) || (!src->v.l && !dst->v.l) ) {
                    return 1;
                }
                break;
            case T_STRING:
                if (lgx_str_cmp(src->v.str, dst->v.str) == 0) {
                    return 1;
                }
                break;
            case T_FUNCTION:
                if (src->v.fun == dst->v.fun) {
                    return 1;
                }
                break;
            case T_ARRAY:
                // 比较大数组可能会很慢，需要关注
                if (lgx_hash_cmp(src->v.arr, dst->v.arr) == 0) {
                    return 1;
                }
                break;
            case T_OBJECT:
                if (src->v.obj == dst->v.obj) {
                    return 1;
                }
                break;
            default:
                return 0;
        }
    }

    return 0;
}

void lgx_val_free(lgx_val_t *src) {
    assert(src->v.gc->ref_cnt == 0);

    switch (src->type) {
        case T_STRING:   lgx_str_delete(src->v.str); break;
        case T_ARRAY:    lgx_hash_delete(src->v.arr); break;
        case T_FUNCTION: lgx_fun_delete(src->v.fun); break;
        case T_OBJECT:   lgx_obj_delete(src->v.obj); break;
        case T_RESOURCE: lgx_res_delete(src->v.res); break;
        default: break;
    }
}