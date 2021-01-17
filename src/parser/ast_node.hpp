#ifndef _XSCRIPT_PARSER_AST_NODE_HPP_
#define _XSCRIPT_PARSER_AST_NODE_HPP_

#include <memory>
#include <vector>

namespace xscript::parser {

typedef enum {
    AST_ROOT = 1,
    // Declaration
    PACKAGE_DECLARATION,
    IMPORT_DECLARATION,
    EXPORT_DECLARATION,
    VARIABLE_DECLARATION,
    CONSTANT_DECLARATION,
    FUNCTION_DECLARATION,
    TYPE_DECLARATION,
    // Statement
    BLOCK_STATEMENT,
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
    ECHO_STATEMENT,
    CO_STATEMENT,
    // Parameter
    FUNCTION_CALL_PARAMETER,
    FUNCTION_DECL_PARAMETER,
    EXCEPTION_DECL_PARAMETER,
    FUNCTION_TYPE_DECL_PARAMETER,
    // Expression
    BINARY_EXPRESSION,
    UNARY_EXPRESSION,
    ARRAY_EXPRESSION,
    STRUCT_EXPRESSION,
    TYPE_EXPRESSION,
    // Token
    IDENTIFIER_TOKEN,
    LONG_TOKEN,
    DOUBLE_TOKEN,
    CHAR_TOKEN,
    STRING_TOKEN,
    TRUE_TOKEN,
    FALSE_TOKEN,
    NULL_TOKEN,
    // Other
    PACKAGE_NAME,
    PACKAGE_RENAME,
    FOR_CONDITION,
    FUNCTION_RECEIVER
} type_t;

class ast_node {
private:
    ast_node* parent;
    std::vector<ast_node> children;

    type_t type;

    // 当前节点对应的代码位置
    size_t offset;
    size_t length;
    size_t line;
    size_t row;

public:
    ast_node();
    ast_node(ast_node& p, type_t t);
    
    ast_node* get_parent();
    type_t get_type();
    ast_node& add_child(type_t t);
};

}

#endif // _XSCRIPT_PARSER_AST_NODE_HPP_