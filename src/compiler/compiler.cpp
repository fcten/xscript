#include <cassert>
#include "compiler.hpp"

namespace xscript::compiler {

compiler::compiler(std::string s) :
    ast(s)
{

}

bool compiler::compile() {
    if (!ast.parse()) {
        return false;
    }

    assert(ast.get_root() != nullptr);

    return compile_root(ast.get_root());
}

bool compiler::compile_function_declaration(std::unique_ptr<parser::ast_node>& node) {

}

bool compiler::compile_global_variable_declaration(std::unique_ptr<parser::ast_node>& node) {
    assert(node->get_type() == parser::VARIABLE_DECLARATION);
    assert(node->get_children().size() >= 2);
    assert(node->get_children().at(0)->get_type() == parser::TOKEN);
    //assert(node->get_children().at(1)->get_type() == parser::TYPE_EXPRESSION);


}

bool compiler::compile_local_variable_declaration(std::unique_ptr<parser::ast_node>& node) {
    assert(node->get_type() == parser::VARIABLE_DECLARATION);
    assert(node->get_children().size() >= 2);
    assert(node->get_children().at(0)->get_type() == parser::TOKEN);
    //assert(node->get_children().at(1)->get_type() == parser::TYPE_EXPRESSION);


}

bool compiler::compile_constant_declaration(std::unique_ptr<parser::ast_node>& node) {

}

bool compiler::compile_root(std::unique_ptr<parser::ast_node>& node) {
    assert(node->get_type() == parser::ROOT);

    bool ret = true;

    auto& children = node->get_children();
    for (auto & n : children) {
        switch (n->get_type()) {
            case parser::FUNCTION_DECLARATION:
                if (!compile_function_declaration(n)) {
                    ret = false;
                }
                break;
            case parser::VARIABLE_DECLARATION:
                if (!compile_global_variable_declaration(n)) {
                    ret = false;
                }
                break;
            case parser::CONSTANT_DECLARATION:
                if (!compile_constant_declaration(n)) {
                    ret = false;
                }
                break;
            case parser::PACKAGE_DECLARATION:
            case parser::IMPORT_DECLARATION:
            case parser::EXPORT_DECLARATION:
                // TODO
                break;
            default:
                assert(false);
                ret = false;
                break;
        }
    }

    return ret;
}

}
