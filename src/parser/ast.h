#ifndef LGX_AST_H
#define LGX_AST_H

#include "../tokenizer/lex.h"
#include "../common/hash.h"

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
    SWITCH_STATEMENT,
    CASE_STATEMENT,
    DEFAULT_STATEMENT,
    RETURN_STATEMENT,
    ECHO_STATEMENT,
    EXPRESSION_STATEMENT,
    TRY_STATEMENT,
    CATCH_STATEMENT,
    FINALLY_STATEMENT,
    THROW_STATEMENT,
    // Declaration
    FUNCTION_DECLARATION,
    VARIABLE_DECLARATION,
    CLASS_DECLARATION,
    INTERFACE_DECLARATION,
    PROPERTY_DECLARATION,
    METHOD_DECLARATION,
    // Parameter
    FUNCTION_CALL_PARAMETER,
    FUNCTION_DECL_PARAMETER,
    // Expression
    CONDITIONAL_EXPRESSION,
    BINARY_EXPRESSION,
    UNARY_EXPRESSION,
    // Other
    IDENTIFIER_TOKEN,
    UNDEFINED_TOKEN,
    LONG_TOKEN,
    DOUBLE_TOKEN,
    STRING_TOKEN,
    FUNCTION_TOKEN, // 匿名函数
    ARRAY_TOKEN,
    OBJECT_TOKEN,
    TRUE_TOKEN,
    FALSE_TOKEN
};

typedef struct lgx_ast_node_list_s {
    lgx_list_t head;
    struct lgx_ast_node_s *node;
} lgx_ast_node_list_t;

typedef struct lgx_ast_node_s {
    unsigned short type;
    struct lgx_ast_node_s* parent;

    unsigned line; // 当前节点对应的代码位置

    union {
        // 当节点类型为 BLOCK 时，用于保存符号表
        lgx_hash_t *symbols;

        // 当节点类型为 EXPRESSION 时，用于保存 EXPRESSION 的类型
        unsigned short op;

        // 当节点类型为 FOR_STATEMENT、WHILE_STATEMENT、DO_WHILE_STATEMENT 时，用于保存 break 与 continue 语句出现的位置
        // 当节点类型为 SWITCH_CASE_STATEMENT 时，保存 break 语句出现的位置
        // 当节点类型为 FUNCTION_DECLARATION 时，保存 return 语句出现的位置
        lgx_ast_node_list_t *jmps;

        // 当节点类型为 BREAK_STATEMENT、CONTINUE_STATEMENT、RETURN_STATEMENT 时，用于保存语句在字节码中对于的位置
        unsigned pos;

        // 当节点类型为 VARIABLE_DECLARATION 时，保存变量类型
        // 当节点类型为 FUNCTION_DECLARATION 时，保存返回值类型
        unsigned type;

        // 当节点类型为 PROPERTY_DECLARATION、METHOD_DECLARATION 时，保存 modifier
        struct {
            char is_static;
            char is_const;
            char access;
        } modifier;
    } u;

    int children;          // 子节点数量
    int size;              // 已分配空间的长度
    struct lgx_ast_node_s* child[];
} lgx_ast_node_t;

typedef struct lgx_ast_node_token_s {
    unsigned short type;
    struct lgx_ast_node_s* parent;

    unsigned line; // 当前节点对应的代码位置

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

    int err_no;
    char *err_info;
    int err_len;
} lgx_ast_t;

int lgx_ast_init(lgx_ast_t* ast);
int lgx_ast_parser(lgx_ast_t* ast);
int lgx_ast_optimizer(lgx_ast_t* ast);
int lgx_ast_cleanup(lgx_ast_t* ast);
void lgx_ast_print(lgx_ast_node_t* node, int indent);

#endif // LGX_AST_H