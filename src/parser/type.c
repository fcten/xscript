#include "type.h"

char* lgx_type_to_string(lgx_type_t* type) {
    switch (type->type) {
        case T_UNKNOWN:
            return "unknown";
        case T_LONG:
            return "integer";
        case T_DOUBLE:
            return "float";
        case T_BOOL:
            return "boolen";
        case T_STRING:
            return "string";
        case T_ARRAY:
            return "array";
        case T_MAP:
            return "map";
        case T_STRUCT:
            return "struct";
        case T_INTERFACE:
            return "interface";
        case T_FUNCTION:
            return "function";
        default:
            return "invalid";
    }
}

int lgx_type_cmp(lgx_type_t* t1, lgx_type_t* t2) {
    if (t1->type != t2->type) {
        return 1;
    }

    switch (t1->type) {
        case T_ARRAY:
            // TODO
            break;
        case T_MAP:
            // TODO
            break;
        case T_STRUCT:
            // TODO
            break;
        case T_INTERFACE:
            // TODO
            break;
        case T_FUNCTION:
            // TODO
            break;
        default:
            break;
    }

    return 0;
}

int lgx_type_dup(lgx_type_t* src, lgx_type_t* dst) {
    dst->type = src->type;

    // TODO

    return 0;
}

int lgx_type_init(lgx_type_t* type, lgx_val_type_t t) {
    memset(type, 0, sizeof(lgx_type_t));

    type->type = t;

    switch (type->type) {
        case T_CUSTOM:
        case T_LONG:
        case T_DOUBLE:
        case T_BOOL:
        case T_STRING:
        case T_UNKNOWN:
            // 简单类型
            break;
        case T_ARRAY:
            type->u.arr = xcalloc(1, sizeof(lgx_type_array_t));
            if (!type->u.arr) {
                return 1;
            }
            break;
        case T_MAP:
            type->u.map = xcalloc(1, sizeof(lgx_type_map_t));
            if (!type->u.map) {
                return 1;
            }
            break;
        case T_STRUCT:
            type->u.sru = xcalloc(1, sizeof(lgx_type_struct_t));
            if (!type->u.sru) {
                return 1;
            }
            break;
        case T_INTERFACE:
            type->u.itf = xcalloc(1, sizeof(lgx_type_interface_t));
            if (!type->u.itf) {
                return 1;
            }
            break;
        case T_FUNCTION:
            type->u.fun = xcalloc(1, sizeof(lgx_type_function_t));
            if (!type->u.fun) {
                return 1;
            }
            break;
        default:
            return 1;
    }

    return 0;
}

void lgx_type_array_cleanup(lgx_type_array_t* arr) {

}

void lgx_type_struct_cleanup(lgx_type_struct_t* sru) {

}

void lgx_type_interface_cleanup(lgx_type_interface_t* itf) {

}

void lgx_type_function_cleanup(lgx_type_function_t* fun) {

}

void lgx_type_cleanup(lgx_type_t* type) {
    switch (type->type) {
        case T_ARRAY:
            lgx_type_array_cleanup(type->u.arr);
            break;
        case T_STRUCT:
            lgx_type_struct_cleanup(type->u.sru);
            break;
        case T_INTERFACE:
            lgx_type_interface_cleanup(type->u.itf);
            break;
        case T_FUNCTION:
            lgx_type_function_cleanup(type->u.fun);
            break;
        case T_UNKNOWN:
        case T_LONG:
        case T_DOUBLE:
        case T_BOOL:
        case T_STRING:
        default:
            break;
    }

    memset(type, 0, sizeof(lgx_type_t));
}