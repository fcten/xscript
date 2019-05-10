#ifndef LGX_SYMBOL_H
#define LGX_SYMBOL_H

#include "ast.h"

// 符号类型
typedef enum {
    S_VARIABLE = 0,
    S_CONSTANT,
    S_TYPE,
} lgx_symbol_type_t;

int lgx_symbol_init(lgx_ast_t* ast);

#endif // LGX_SYMBOL_H