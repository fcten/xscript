#ifndef LGX_SYMBOL_H
#define LGX_SYMBOL_H

#include "ast.h"
#include "type.h"

// 符号类型
typedef enum {
    S_UNKNOWN = 0,
    S_VARIABLE,
    S_CONSTANT,
    S_TYPE,
} lgx_symbol_type_t;

typedef struct {
    // 符号类型
    lgx_symbol_type_t s_type;

    // 值类型
    lgx_type_t type;

    union {
        // 如果符号类型为 S_CONSTANT，保存符号的值
        lgx_value_t v;

        // 如果符号类型为 S_VARIABLE，保存符号的编号
        unsigned r;
    } u;

    // 是否为全局符号
    unsigned char is_global:1;

    // 是否被赋值
    unsigned char is_initialized:1;

    // 是否被使用
    unsigned char is_used:1;

    // 符号声明对应的 AST 节点
    lgx_ast_node_t* node;
} lgx_symbol_t;

int lgx_symbol_init(lgx_ast_t* ast);
void lgx_symbol_cleanup(lgx_ast_t* ast);

lgx_symbol_t* lgx_symbol_get(lgx_ast_node_t* node, lgx_str_t* name, unsigned offset);

#endif // LGX_SYMBOL_H