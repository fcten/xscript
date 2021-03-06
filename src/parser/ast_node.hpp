#ifndef _XSCRIPT_PARSER_AST_NODE_HPP_
#define _XSCRIPT_PARSER_AST_NODE_HPP_

#include <memory>
#include <vector>
#include "../tokenizer/token.hpp"
#include "register_allocator.hpp"
#include "symbol_table.hpp"

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
    // Expression
    BINARY_EXPRESSION,
    UNARY_EXPRESSION,
    LITERAL_ARRAY_EXPRESSION,
    LITERAL_OBJECT_EXPRESSION,
    // Token
    TOKEN,
    // Other
    PACKAGE_NAME,
    PACKAGE_RENAME,
    TYPE_DECLARATOR,
    VARIABLE_INITIALIZER,
    KEY_VALUE_PAIR,
    SWITCH_LABEL,
    CATCH_BLOCK
} type_t;

class ast_node {
private:
    ast_node* parent;
    std::vector<std::unique_ptr<ast_node>> children;

    type_t type;
    tokenizer::token token;

    // 当前节点对应的代码位置
    size_t offset;
    size_t length;
    size_t line;
    size_t row;

    // 指向当前节点对应的符号表
    std::shared_ptr<symbol_table> symbols;

    // 指向当前节点对应的寄存器分配器
    std::shared_ptr<register_allocator> regs;

    // 当节点类型为 TYPE_EXPRESSION 时，用于保存类型
    //lgx_val_type_t type;

    // 当节点类型为 FOR_STATEMENT、WHILE_STATEMENT、DO_STATEMENT 时，用于保存 break 与 continue 语句出现的位置
    // 当节点类型为 SWITCH_STATEMENT 时，保存 break 语句出现的位置
    //lgx_list_t *jmps;

    // 当节点类型为 BREAK_STATEMENT、CONTINUE_STATEMENT、RETURN_STATEMENT 时，用于保存语句在字节码中对于的位置
    //unsigned pos;

public:
    ast_node();
    ast_node(type_t t, ast_node* p);
    
    type_t get_type();
    std::vector<std::unique_ptr<ast_node>>& get_children();
    std::shared_ptr<symbol_table>& get_symbols();
    std::shared_ptr<register_allocator>& get_regs();

    bool is_empty();
    std::unique_ptr<ast_node>& add_child(type_t t);
    std::unique_ptr<ast_node>& replace_child(type_t t);
    void set_token(tokenizer::token t);

    void print(int indent);
};

}

#endif // _XSCRIPT_PARSER_AST_NODE_HPP_