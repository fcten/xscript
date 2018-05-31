#ifndef LGX_AST_H
#define LGX_AST_H

#include "../tokenizer/lex.h"

enum {
    // Statement
    BLOCK_STATEMENT = 1,
    IF_STATEMENT,
    IF_ELSE_STATEMENT,
    FOR_STATEMENT,
    WHILE_STATEMENT,
    DO_WHILE_STATEMENT,
    CONTINUE_STATEMENT,
    BREAK_STATEMENT,
    SWITCH_CASE_STATEMENT,
    RETURN_STATEMENT,
    ASSIGNMENT_STATEMENT,
    // Declaration
    FUNCTION_DECLARATION,
    VARIABLE_DECLARATION,
    // Parameter
    FUNCTION_CALL_PARAMETER,
    FUNCTION_DECL_PARAMETER,
    // Expression
    CALL_EXPRESSION,
    ARRAY_EXPRESSION,
    CONDITIONAL_EXPRESSION,
    BINARY_EXPRESSION,
    UNARY_EXPRESSION,
    // Other
    IDENTIFIER_TOKEN,
    NUMBER_TOKEN,
    STRING_TOKEN
};

typedef struct lgx_ast_node {
    unsigned short type;
    //int line;            // 在源代码中对应的位置

    unsigned short op;     // 用于保存 EXPRESSION 的类型

    int children;          // 子节点数量
    int size;              // 已分配空间的长度
    struct lgx_ast_node* child[];
} lgx_ast_node_t;

typedef struct lgx_ast_node_token {
    unsigned short type;
    //int line;            // 在源代码中对应的位置

    char* tk_start;
    int tk_length;
} lgx_ast_node_token_t;

typedef struct lgx_ast {
    lgx_lex_t lex;

    int cur_token;
    char* cur_start;
    int cur_length;
    int cur_line;
    
    lgx_ast_node_t* root;

    int errno;
    char *err_info;
    int err_len;
} lgx_ast_t;

int lgx_ast_init(lgx_ast_t* ast);
int lgx_ast_parser(lgx_ast_t* ast);
int lgx_ast_optimizer(lgx_ast_t* ast);
void lgx_ast_print(lgx_ast_node_t* node, int indent);

#endif // LGX_AST_H