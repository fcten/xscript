#ifndef LGX_AST_H
#define LGX_AST_H

#include "../common/list.h"
#include "../common/ht.h"
#include "../tokenizer/lex.h"
#include "../compiler/register.h"
#include "type.h"

typedef enum {
    // Statement
    BLOCK_STATEMENT = 1,
    IF_STATEMENT,
    IF_ELSE_STATEMENT,
    FOR_STATEMENT,
    WHILE_STATEMENT,
    DO_STATEMENT,
    CONTINUE_STATEMENT,
    BREAK_STATEMENT,
    SWITCH_STATEMENT,
    CASE_STATEMENT,
    DEFAULT_STATEMENT,
    RETURN_STATEMENT,
    EXPRESSION_STATEMENT,
    TRY_STATEMENT,
    CATCH_STATEMENT,
    THROW_STATEMENT,
    // Declaration
    PACKAGE_DECLARATION,
    IMPORT_DECLARATION,
    EXPORT_DECLARATION,
    VARIABLE_DECLARATION,
    CONSTANT_DECLARATION,
    FUNCTION_DECLARATION,
    TYPE_DECLARATION,
    // Parameter
    FUNCTION_CALL_PARAMETER,
    FUNCTION_DECL_PARAMETER,
    // Expression
    BINARY_EXPRESSION,
    UNARY_EXPRESSION,
    ARRAY_EXPRESSION,
    STRUCT_EXPRESSION,
    TYPE_EXPRESSION,
    // Other
    IDENTIFIER_TOKEN,
    LONG_TOKEN,
    DOUBLE_TOKEN,
    CHAR_TOKEN,
    STRING_TOKEN,
    TRUE_TOKEN,
    FALSE_TOKEN,
    NULL_TOKEN,
    FOR_CONDITION
} lgx_ast_type_t;

typedef struct lgx_ast_node_list_s {
    lgx_list_t head;
    struct lgx_ast_node_s *node;
} lgx_ast_node_list_t;

typedef struct lgx_ast_node_s {
    // 节点类型
    lgx_ast_type_t type:8;

    // 父节点
    struct lgx_ast_node_s* parent;

    // 子节点
    unsigned size;
    unsigned children;
    struct lgx_ast_node_s** child;

    // 当前节点对应的代码位置
    unsigned offset;
    unsigned length;
    unsigned line;
    unsigned row;

    union {
        // 当节点类型为 BLOCK_STATEMENT 时，用于保存符号表
        lgx_ht_t *symbols;

        // 当节点类型为 FUNCTION_DECLARATION 时，用于保存寄存器分配器
        lgx_reg_t *regs;

        // 当节点类型为 BINARY_EXPRESSION、UNARY_EXPRESSION 时，用于保存 EXPRESSION 的操作符
        lgx_token_t op;

        // 当节点类型为 FOR_STATEMENT、WHILE_STATEMENT、DO_WHILE_STATEMENT 时，用于保存 break 与 continue 语句出现的位置
        // 当节点类型为 SWITCH_CASE_STATEMENT 时，保存 break 语句出现的位置
        lgx_ast_node_list_t *jmps;

        // 当节点类型为 BREAK_STATEMENT、CONTINUE_STATEMENT、RETURN_STATEMENT 时，用于保存语句在字节码中对于的位置
        unsigned pos;
    } u;
} lgx_ast_node_t;

typedef struct lgx_ast_error_list_s {
    lgx_list_t head;

    // 错误信息
    int err_no;
    lgx_str_t err_msg;
} lgx_ast_error_list_t;

typedef struct lgx_ast {
    // 源文件
    lgx_lex_t lex;

    // 词法分析相关数据
    lgx_token_t prev_token;
    lgx_token_t cur_token;
    char* cur_start;
    int cur_length;

    // 抽象语法树
    lgx_ast_node_t *root;

    // 错误信息
    lgx_list_t errors; // lgx_ast_error_list_t
} lgx_ast_t;

int lgx_ast_init(lgx_ast_t* ast, char* file);
int lgx_ast_cleanup(lgx_ast_t* ast);

void lgx_ast_print(lgx_ast_t* ast);
void lgx_ast_print_error(lgx_ast_t* ast);

#endif // LGX_AST_H