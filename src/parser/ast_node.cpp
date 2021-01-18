#include <iostream>
#include "ast_node.hpp"

namespace xscript::parser {

ast_node::ast_node() :
    type(ROOT)
{

}

ast_node::ast_node(type_t t) :
    type(t)
{

}

std::unique_ptr<ast_node>& ast_node::add_child(type_t t) {
    children.push_back(std::make_unique<ast_node>(t));
    return children.back();
}

std::unique_ptr<ast_node>& ast_node::replace_child(type_t t) {
    if (children.size() == 0) {
        return add_child(t);
    } else {
        std::unique_ptr<ast_node> t1 = std::move(children.back());
        children.pop_back();
        std::unique_ptr<ast_node>& t2 = add_child(t);
        std::unique_ptr<ast_node>& t3 = t2->add_child(t1->get_type());
        t3 = std::move(t1);
        return t2;
    }
}

type_t ast_node::get_type() {
    return type;
}

void ast_node::set_token(tokenizer::token t) {
    token = t;
}

bool ast_node::is_empty() {
    return children.size() == 0;
}

std::unordered_map<type_t, std::string_view> type_names = {
    {ROOT, "ROOT"},
    {BLOCK, "BLOCK"},
    // Declaration
    {PACKAGE_DECLARATION, "PACKAGE_DECLARATION"},
    {IMPORT_DECLARATION, "IMPORT_DECLARATION"},
    {EXPORT_DECLARATION, "EXPORT_DECLARATION"},
    {VARIABLE_DECLARATION, "VARIABLE_DECLARATION"},
    {CONSTANT_DECLARATION, "CONSTANT_DECLARATION"},
    {FUNCTION_DECLARATION, "FUNCTION_DECLARATION"},
    {TYPE_DECLARATION, "TYPE_DECLARATION"},
    // Statement
    {IF_STATEMENT, "IF_STATEMENT"},
    {FOR_STATEMENT, "FOR_STATEMENT"},
    {WHILE_STATEMENT, "WHILE_STATEMENT"},
    {DO_STATEMENT, "DO_STATEMENT"},
    {CONTINUE_STATEMENT, "CONTINUE_STATEMENT"},
    {BREAK_STATEMENT, "BREAK_STATEMENT"},
    {SWITCH_STATEMENT, "SWITCH_STATEMENT"},
    {RETURN_STATEMENT, "RETURN_STATEMENT"},
    {EXPRESSION_STATEMENT, "EXPRESSION_STATEMENT"},
    {TRY_STATEMENT, "TRY_STATEMENT"},
    {THROW_STATEMENT, "THROW_STATEMENT"},
    {CO_STATEMENT, "CO_STATEMENT"},
    // Parameter
    {FUNCTION_CALL_PARAMETER, "FUNCTION_CALL_PARAMETER"},
    {FUNCTION_DECL_PARAMETER, "FUNCTION_DECL_PARAMETER"},
    {EXCEPTION_DECL_PARAMETER, "EXCEPTION_DECL_PARAMETER"},
    {FUNCTION_TYPE_DECL_PARAMETER, "FUNCTION_TYPE_DECL_PARAMETER"},
    // Expression
    {BINARY_EXPRESSION, "BINARY_EXPRESSION"},
    {UNARY_EXPRESSION, "UNARY_EXPRESSION"},
    {ARRAY_EXPRESSION, "ARRAY_EXPRESSION"},
    {STRUCT_EXPRESSION, "STRUCT_EXPRESSION"},
    {TYPE_EXPRESSION, "TYPE_EXPRESSION"},
    // Token
    {TOKEN, "TOKEN"},
    // Other
    {PACKAGE_NAME, "PACKAGE_NAME"},
    {PACKAGE_RENAME, "PACKAGE_RENAME"},
    {TYPE_DECLARATOR, "TYPE_DECLARATOR"},
    {VARIABLE_INITIALIZER, "VARIABLE_INITIALIZER"},
    {FOR_CONDITION, "FOR_CONDITION"},
    {FUNCTION_RECEIVER, "FUNCTION_RECEIVER"}
};

void ast_node::print(int indent) {
    for (int i = 0 ; i < indent ; i++) {
        std::cout << " ";
    }
    auto it = type_names.find(type);
    if (it != type_names.end()) {
        std::cout << it->second << std::endl;
    } else {
        std::cout << "UNKNOWN_AST_NODE" << std::endl;
    }
    for (auto it = children.begin() ; it != children.end() ; it ++) {
        (*it)->print(indent + 2);
    }
}

}
