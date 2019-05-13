#ifndef LGX_SYMBOL_H
#define LGX_SYMBOL_H

#include "ast.h"

// 符号类型
typedef enum {
    S_VARIABLE = 0,
    S_CONSTANT,
    S_TYPE,
} lgx_symbol_type_t;

typedef struct {
    // 符号类型
    lgx_symbol_type_t type;

    // 值类型
    lgx_value_t value;

    // 符号声明对应的 AST 节点
    lgx_ast_node_t* node;
} lgx_symbol_t;

int lgx_symbol_init(lgx_ast_t* ast);
int lgx_symbol_cleanup(lgx_ast_t* ast);

lgx_symbol_t* lgx_symbol_get(lgx_ast_node_t* node, lgx_str_t* name, unsigned offset);

#endif // LGX_SYMBOL_H