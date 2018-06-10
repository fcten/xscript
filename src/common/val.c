#include <stdio.h>

#include "val.h"

void lgx_val_print(lgx_val_t *v) {
    switch (v->type) {
        case T_UNDEFINED:
            printf("<UNDEFINED>\n");
            break;
        case T_UNINITIALIZED:
            printf("<UNINITIALIZED>\n");
            break;
        case T_LONG:
            printf("<INT> %lld\n", v->v.l);
            break;
        case T_DOUBLE:
            printf("<FLOAT> %f\n", v->v.d);
            break;
        case T_BOOL:
            if (v->v.l) {
                printf("<BOOL> true\n");
            } else {
                printf("<BOOL> false\n");
            }
            break;
        case T_STRING:
            printf("<STRING>\n");
            break;
        case T_ARRAY:
            printf("<ARRAY>\n");
            break;
        case T_OBJECT:
            printf("<OBJECT>\n");
            break;
        case T_RESOURCE:
            printf("<RESOURCE>\n");
            break;
        case T_REDERENCE:
            printf("<REDERENCE>\n");
            break;
        case T_FUNCTION:
            printf("<FUNCTION>\n");
            break;
        default:
            printf("<ERROR>\n");
            break;
    }
}