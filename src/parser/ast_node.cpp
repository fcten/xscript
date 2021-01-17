#include "ast_node.hpp"

namespace xscript::parser {

ast_node::ast_node() :
    parent(nullptr),
    type(AST_ROOT)
{

}

ast_node::ast_node(ast_node& p, type_t t) :
    parent(&p),
    type(t)
{

}

ast_node& ast_node::add_child(type_t t) {
    children.push_back(ast_node(*this, t));
    return children.back();
}

type_t ast_node::get_type() {
    return type;
}

ast_node* ast_node::get_parent() {
    return parent;
}

}
