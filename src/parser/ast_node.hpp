#ifndef _XSCRIPT_PARSER_AST_NODE_HPP_
#define _XSCRIPT_PARSER_AST_NODE_HPP_

#include <memory>
#include <vector>
#include "../tokenizer/token.hpp"

namespace xscript::parser {

typedef enum {
    ROOT = 1,
    BLOCK,
    // Declaration
    PACKAGE_DECLARATION,
    IMPORT_DECLARATION,
    EXPORT_DECLARATION,
    VARIABLE_DECLARATION,
    CONSTANT_DECLARATION,
    FUNCTION_DECLARATION,
    TYPE_DECLARATION,
    // Statement
    IF_STATEMENT,
    FOR_STATEMENT,
    WHILE_STATEMENT,
    DO_STATEMENT,
    CONTINUE_STATEMENT,
    BREAK_STATEMENT,
    SWITCH_STATEMENT,
    RETURN_STATEMENT,
    EXPRESSION_STATEMENT,
    TRY_STATEMENT,
    THROW_STATEMENT,
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
    TOKEN,
    // Other
    PACKAGE_NAME,
    PACKAGE_RENAME,
    TYPE_DECLARATOR,
    VARIABLE_INITIALIZER,
    FOR_CONDITION,
    FUNCTION_RECEIVER
} type_t;

class ast_node {
private:
    std::vector<std::unique_ptr<ast_node>> children;

    type_t type;
    tokenizer::token token;

    // 当前节点对应的代码位置
    size_t offset;
    size_t length;
    size_t line;
    size_t row;

public:
    ast_node();
    ast_node(type_t t);
    
    type_t get_type();
    bool is_empty();
    std::unique_ptr<ast_node>& add_child(type_t t);
    std::unique_ptr<ast_node>& replace_child(type_t t);
    void set_token(tokenizer::token t);

    void print(int indent);
};

}

#endif // _XSCRIPT_PARSER_AST_NODE_HPP_