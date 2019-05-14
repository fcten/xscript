#include "type.h"

char* lgx_type_to_string(lgx_val_type_t type) {
    switch (type) {
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