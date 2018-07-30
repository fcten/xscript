#include "cast.h"

int lgx_cast_long(lgx_val_t *ret, lgx_val_t *src) {
    ret->type = T_LONG;

    switch (src->type) {
        case T_LONG:
            ret->v.l = src->v.l;
            break;
        default:
            return 1;
    }

    return 0;
}

int lgx_cast_double(lgx_val_t *ret, lgx_val_t *src) {
    ret->type = T_DOUBLE;

    switch (src->type) {
        case T_DOUBLE:
            ret->v.d = src->v.d;
            break;
        case T_LONG:
            ret->v.d = src->v.l;
            break;
        default:
            return 1;
    }

    return 0;
}

int lgx_cast_string(lgx_val_t *ret, lgx_val_t *src) {
    ret->type = T_STRING;

    switch (src->type) {
        case T_STRING:
            ret->v.str = src->v.str;
            break;
        default:
            return 1;
    }

    return 0;
}

int lgx_cast_array(lgx_val_t *ret, lgx_val_t *src) {
    ret->type = T_ARRAY;

    switch (src->type) {
        case T_ARRAY:
            ret->v.arr = src->v.arr;
            break;
        default:
            return 1;
    }

    return 0;
}

int lgx_cast_bool(lgx_val_t *ret, lgx_val_t *src) {
    ret->type = T_BOOL;

    switch (src->type) {
        case T_BOOL:
            ret->v.l = src->v.l;
            break;
        default:
            return 1;
    }

    return 0;
}