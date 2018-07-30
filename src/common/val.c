#include <stdio.h>

#include "val.h"
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
        return lgx_val_types[v->type].s;
    } else {
        return lgx_val_types[T_UNDEFINED].s;
    }
}


void lgx_val_print(lgx_val_t *v) {
    switch (v->type) {
        case T_UNDEFINED:
            printf("<%s>", lgx_val_typeof(v));
            break;
        case T_LONG:
            //printf("<%s> %lld\n", lgx_val_typeof(v), v->v.l);
            printf("%lld", v->v.l);
            break;
        case T_DOUBLE:
            //printf("<%s> %f\n", lgx_val_typeof(v), v->v.d);
            printf("%f", v->v.d);
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
            printf("\"%.*s\"", v->v.str->length, v->v.str->buffer);
            break;
        case T_ARRAY:
            //printf("<%s>\n", lgx_val_typeof(v));
            lgx_hash_print(v->v.arr);
            break;
        case T_OBJECT:
            printf("<%s>", lgx_val_typeof(v));
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