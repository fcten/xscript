#include "ast_node.hpp"

namespace xscript::parser {

ast_node::ast_node(type_t node_type) :
    type(node_type)
{

}

void ast_node::add_child(std::shared_ptr<ast_node> child) {
    children.push_back(child);
}

std::weak_ptr<ast_node> ast_node::get_parent() {
    return parent;
}

}
