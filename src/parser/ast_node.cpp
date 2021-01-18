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

}
