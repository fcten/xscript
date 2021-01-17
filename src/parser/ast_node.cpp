#include "ast_node.hpp"

namespace xscript::parser {

ast_node::ast_node() :
    parent(nullptr),
    type(PACKAGE_DECLARATION)
{

}

ast_node::ast_node(ast_node& p, type_t t) :
    parent(&p),
    type(t)
{

}

void ast_node::add_child(ast_node& child) {
    children.push_back(child);
}

ast_node* ast_node::get_parent() {
    return parent;
}

}
