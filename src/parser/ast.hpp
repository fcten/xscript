#ifndef _XSCRIPT_PARSER_AST_HPP_
#define _XSCRIPT_PARSER_AST_HPP_

#include <vector>
#include <string>
#include <set>
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
    bool failover(std::initializer_list<std::string_view> args, std::set<tokenizer::token_t> tokens);
    bool failover(std::initializer_list<std::string_view> args);
    bool expect();

    bool parse_string_token(ast_node& parent);

    bool parse_package_declaration(ast_node& parent);
    bool parse_import_declaration(ast_node& parent);
    bool parse_export_declaration(ast_node& parent);
    bool parse_variable_declaration(ast_node& parent);
    bool parse_constant_declaration(ast_node& parent);
    bool parse_function_declaration(ast_node& parent);
    bool parse_type_declaration(ast_node& parent);

    bool parse_if_statement(ast_node& parent);
    bool parse_for_statement(ast_node& parent);
    bool parse_while_statement(ast_node& parent);
    bool parse_do_statement(ast_node& parent);
    bool parse_break_statement(ast_node& parent);
    bool parse_continue_statement(ast_node& parent);
    bool parse_switch_statement(ast_node& parent);
    bool parse_return_statement(ast_node& parent);
    bool parse_try_statement(ast_node& parent);
    bool parse_throw_statement(ast_node& parent);
    bool parse_echo_statement(ast_node& parent);
    bool parse_co_statement(ast_node& parent);
    bool parse_expression_statement(ast_node& parent);

    bool parse_declaration(ast_node& parent);
    bool parse_statement(ast_node& parent);

    bool parse_package_name(ast_node& parent);
    bool parse_package_rename(ast_node& parent);
    bool parse_import_declarations(ast_node& parent);
    bool parse_global_declarations(ast_node& parent);
    
public:
    ast(std::string s);

    bool parse();

    void print();
};

}

#endif // _XSCRIPT_PARSER_AST_HPP_