#ifndef LGX_AST_H
#define LGX_AST_H

#include "../common/list.h"
#include "../common/ht.h"
#include "../tokenizer/lex.h"
#include "type.h"
#include "symbol.h"

typedef enum {
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
    EXPRESSION_STATEMENT,
    TRY_STATEMENT,
    CATCH_STATEMENT,
    THROW_STATEMENT,
    // Declaration
    FUNCTION_DECLARATION,
    VARIABLE_DECLARATION,
    CONSTANT_DECLARATION,
    STRUCT_DECLARATION,
    INTERFACE_DECLARATION,
    // Parameter
    FUNCTION_CALL_PARAMETER,
    FUNCTION_DECL_PARAMETER,
    // Expression
    BINARY_EXPRESSION,
    UNARY_EXPRESSION,
    ARRAY_EXPRESSION,
    FUNCTION_EXPRESSION,
    STRUCT_EXPRESSION,
    // Other
    IDENTIFIER_TOKEN,
    LONG_TOKEN,
    DOUBLE_TOKEN,
    CHAR_TOKEN,
    STRING_TOKEN,
    TRUE_TOKEN,
    FALSE_TOKEN,
} lgx_ast_type_t;

typedef struct lgx_ast_node_list_s {
    lgx_list_t head;
    struct lgx_ast_node_s *node;
} lgx_ast_node_list_t;

typedef struct lgx_ast_node_s {
    lgx_ast_type_t type:8;
    struct lgx_ast_node_s* parent;

    // 当前节点对应的代码位置
    char* file;
    unsigned line;
    unsigned offset;

    char* start;
    unsigned length;

    union {
        // 当节点类型为 BLOCK 时，用于保存符号表
        lgx_ht_t *symbols;

        // 当节点类型为 EXPRESSION 时，用于保存 EXPRESSION 的类型
        unsigned short op;

        // 当节点类型为 FOR_STATEMENT、WHILE_STATEMENT、DO_WHILE_STATEMENT 时，用于保存 break 与 continue 语句出现的位置
        // 当节点类型为 SWITCH_CASE_STATEMENT 时，保存 break 语句出现的位置
        lgx_ast_node_list_t *jmps;

        // 当节点类型为 BREAK_STATEMENT、CONTINUE_STATEMENT、RETURN_STATEMENT 时，用于保存语句在字节码中对于的位置
        unsigned pos;

        // 当节点类型为 VARIABLE_DECLARATION 时，保存变量类型
        // 当节点类型为 FUNCTION_DECLARATION 时，保存返回值类型
        lgx_value_t *type;
    } u;

    int children;          // 子节点数量
    int size;              // 已分配空间的长度
    struct lgx_ast_node_s* child[];
} lgx_ast_node_t;

typedef struct {
    lgx_list_t head;

    lgx_lex_t lex;

    int cur_token;
    char* cur_start;
    int cur_length;
    int cur_line;
    int cur_line_offset;

    int finished;
} lgx_package_t;

typedef struct lgx_ast {
    // package
    lgx_package_t imported;

    // ast
    lgx_ast_node_t* root;

    // 错误信息
    int err_no;
    char *err_info;
    int err_len;
} lgx_ast_t;

int lgx_ast_init(lgx_ast_t* ast);
int lgx_ast_cleanup(lgx_ast_t* ast);

int lgx_ast_parser(lgx_ast_t* ast, char *file);
int lgx_ast_import(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent, char *file);

int lgx_ast_optimizer(lgx_ast_t* ast);

void lgx_ast_print(lgx_ast_node_t* node, int indent);

#endif // LGX_AST_H