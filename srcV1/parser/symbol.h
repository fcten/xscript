#ifndef LGX_SYMBOL_H
#define LGX_SYMBOL_H

#include "ast.h"

// 符号类型
typedef enum {
    S_VARIABLE = 0,
    S_CONSTANT,
    S_TYPE,
} lgx_symbol_type_t;

int lgx_symbol_add(lgx_ht_t *symbols, lgx_symbol_type_t type, lgx_str_t s_name, lgx_value_t s_type);

#endif // LGX_SYMBOL_H