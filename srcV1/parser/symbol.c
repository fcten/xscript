#include "symbol.h"

int symbol_add(lgx_ht_t *symbols, lgx_symbol_type_t type, lgx_str_t s_name, lgx_value_t s_type) {
    return 0;
}

int symbol_parse(lgx_ast_t* ast, lgx_ast_node_t* node) {
    switch (node->type) {
        case PACKAGE_DECLARATION:
        case IMPORT_DECLARATION:
        case EXPORT_DECLARATION:
        case VARIABLE_DECLARATION: {
            if (node->parent->type == BLOCK_STATEMENT) {
                //symbol_add(node->parent->u.symbols, S_VARIABLE,
                //    ast->lex.source.content + node->offset, node->length);
                lgx_ast_node_t* id = node->child[0];
                printf("[variable] %.*s\n", id->length, ast->lex.source.content + id->offset);
            }
        }
        case CONSTANT_DECLARATION:
        case FUNCTION_DECLARATION:
        case TYPE_DECLARATION:
        default: break;
    }

    int i, ret = 0;
    for (i = 0; i < node->children; ++i) {
        if (symbol_parse(ast, node->child[i])) {
            ret = 1;
        }
    }
    
    return ret;
}

// 遍历语法树生成符号信息
int lgx_symbol_init(lgx_ast_t* ast) {
    return symbol_parse(ast, ast->root);
}