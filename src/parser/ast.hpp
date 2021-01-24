#ifndef _XSCRIPT_PARSER_AST_HPP_
#define _XSCRIPT_PARSER_AST_HPP_

#include <vector>
#include <string>
#include <set>
#include <functional>
#include <initializer_list>
#include "../tokenizer/scanner.hpp"
#include "ast_node.hpp"

namespace xscript::parser {

class ast {
private:
    std::string source;
    tokenizer::scanner scanner;

    bool parsed;

    std::unique_ptr<ast_node> root;

    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    // 词法分析相关数据
    tokenizer::token prev_token;
    tokenizer::token cur_token;
    // 当前处理的行号
    size_t line;
    // 当前处理行的起始偏移量
    size_t line_offset;

    void next();
    void next_line(); 

    bool append_syntax_error(std::string f, int l, std::initializer_list<std::string_view> args);
    
    bool process_failover(std::string f, int l, std::initializer_list<std::string_view> args, std::set<tokenizer::token_t> tokens, bool step_over);
    bool process_failover(std::string f, int l, std::initializer_list<std::string_view> args, bool step_over);
    bool process_failover(std::string f, int l, std::set<tokenizer::token_t> tokens, bool step_over);

    bool parse_package_declaration(std::unique_ptr<ast_node>& parent);
    bool parse_import_declaration(std::unique_ptr<ast_node>& parent);
    bool parse_export_declaration(std::unique_ptr<ast_node>& parent);
    bool parse_variable_declaration(std::unique_ptr<ast_node>& parent);
    bool parse_constant_declaration(std::unique_ptr<ast_node>& parent);
    bool parse_function_declaration(std::unique_ptr<ast_node>& parent);
    bool parse_type_declaration(std::unique_ptr<ast_node>& parent);

    bool parse_block_statement(std::unique_ptr<ast_node>& parent);
    bool parse_if_statement(std::unique_ptr<ast_node>& parent);
    bool parse_for_statement(std::unique_ptr<ast_node>& parent);
    bool parse_while_statement(std::unique_ptr<ast_node>& parent);
    bool parse_do_statement(std::unique_ptr<ast_node>& parent);
    bool parse_break_statement(std::unique_ptr<ast_node>& parent);
    bool parse_continue_statement(std::unique_ptr<ast_node>& parent);
    bool parse_switch_statement(std::unique_ptr<ast_node>& parent);
    bool parse_return_statement(std::unique_ptr<ast_node>& parent);
    bool parse_try_statement(std::unique_ptr<ast_node>& parent);
    bool parse_throw_statement(std::unique_ptr<ast_node>& parent);
    bool parse_echo_statement(std::unique_ptr<ast_node>& parent);
    bool parse_expression_statement(std::unique_ptr<ast_node>& parent);

    bool parse_declaration(std::unique_ptr<ast_node>& parent);
    bool parse_statement(std::unique_ptr<ast_node>& parent);
    bool parse_expression(std::unique_ptr<ast_node>& parent);

    bool parse_expression(std::unique_ptr<ast_node>& parent, int precedence);
    bool parse_basic_expression(std::unique_ptr<ast_node>& parent);

    bool parse_token(std::unique_ptr<ast_node>& parent);
    bool parse_array_literal(std::unique_ptr<ast_node>& parent);
    bool parse_object_literal(std::unique_ptr<ast_node>& parent);

    bool parse_package_name(std::unique_ptr<ast_node>& parent);
    bool parse_package_rename(std::unique_ptr<ast_node>& parent);
    bool parse_import_declarations(std::unique_ptr<ast_node>& parent);
    bool parse_global_declarations(std::unique_ptr<ast_node>& parent);
    bool parse_type_declarator(std::unique_ptr<ast_node>& parent);
    bool parse_variable_declarator(std::unique_ptr<ast_node>& parent);
    bool parse_variable_initializer(std::unique_ptr<ast_node>& parent);
    bool parse_function_decl_parameter(std::unique_ptr<ast_node>& parent);
    bool parse_function_call_parameter(std::unique_ptr<ast_node>& parent);
    bool parse_block(std::unique_ptr<ast_node>& parent);
    bool parse_block_statements(std::unique_ptr<ast_node>& parent);
    bool parse_key_value_pair(std::unique_ptr<ast_node>& parent);
    bool parse_switch_block(std::unique_ptr<ast_node>& parent);
    bool parse_switch_label(std::unique_ptr<ast_node>& parent);

    bool predict_type_declarator();
    bool predict_block_statements();
    bool predict_block_statement();
    bool predict_expression_statement();

    int operator_precedence(tokenizer::token_t t);
    
public:
    ast(std::string s);

    bool parse();

    void print();
    void print(std::unique_ptr<ast_node>& node);
    void print(std::unique_ptr<ast_node>& node, int indent);
};

}

#endif // _XSCRIPT_PARSER_AST_HPP_