#ifndef _XSCRIPT_PARSER_AST_HPP_
#define _XSCRIPT_PARSER_AST_HPP_

#include <vector>
#include <string>
#include <initializer_list>
#include "../tokenizer/scanner.hpp"
#include "ast_node.hpp"

namespace xscript::parser {

class ast {
private:
    std::string source;
    tokenizer::scanner scanner;

    bool parsed;

    ast_node root;

    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    // 词法分析相关数据
    tokenizer::token prev_token;
    tokenizer::token cur_token;
    size_t line;

    void syntax_error(std::initializer_list<std::string_view> args);
    void next();

    bool parse_string_token(std::shared_ptr<ast_node> parent);

    bool parse_package_declaration(std::shared_ptr<ast_node> parent);
    bool parse_import_declaration(std::shared_ptr<ast_node> parent);
    bool parse_export_declaration(std::shared_ptr<ast_node> parent);
    bool parse_variable_declaration(std::shared_ptr<ast_node> parent);
    bool parse_constant_declaration(std::shared_ptr<ast_node> parent);
    bool parse_function_declaration(std::shared_ptr<ast_node> parent);
    bool parse_type_declaration(std::shared_ptr<ast_node> parent);

    bool parse_if_statement(std::shared_ptr<ast_node> parent);
    bool parse_for_statement(std::shared_ptr<ast_node> parent);
    bool parse_while_statement(std::shared_ptr<ast_node> parent);
    bool parse_do_statement(std::shared_ptr<ast_node> parent);
    bool parse_break_statement(std::shared_ptr<ast_node> parent);
    bool parse_continue_statement(std::shared_ptr<ast_node> parent);
    bool parse_switch_statement(std::shared_ptr<ast_node> parent);
    bool parse_return_statement(std::shared_ptr<ast_node> parent);
    bool parse_try_statement(std::shared_ptr<ast_node> parent);
    bool parse_throw_statement(std::shared_ptr<ast_node> parent);
    bool parse_echo_statement(std::shared_ptr<ast_node> parent);
    bool parse_co_statement(std::shared_ptr<ast_node> parent);
    bool parse_expression_statement(std::shared_ptr<ast_node> parent);

    bool parse_declaration(std::shared_ptr<ast_node> parent);
    bool parse_statement(std::shared_ptr<ast_node> parent);

public:
    ast(std::string s);

    bool parse();
};

}

#endif // _XSCRIPT_PARSER_AST_HPP_