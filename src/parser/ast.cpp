#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <exception>
#include "ast.hpp"
#include "../util/log.hpp"

namespace xscript::parser {

ast::ast(std::string s) :
    source(s),
    scanner(source),
    parsed(false),
    root(std::make_unique<ast_node>()),
    line(1),
    line_offset(0)
{
    // TODO remove debug code
    //scanner.print();
    //scanner.reset();
}

void ast::next_line() {
    line++;
    line_offset = scanner.get_offset();
}

// 解析下一个 token
void ast::next() {
    while (true) {
        tokenizer::token token = scanner.next();
        switch (token.type) {
            case tokenizer::TK_SPACE:
            case tokenizer::TK_TAB:
                // 忽略空格与制表符
                break;
            case tokenizer::TK_COMMENT_LINE:
                // 跳过单行注释
                while (token != tokenizer::TK_EOF && token != tokenizer::TK_EOL) {
                    token = scanner.next();
                    if (token == tokenizer::TK_EOL) {
                        next_line();
                    }
                }
                break;
            case tokenizer::TK_COMMENT_BEGIN:
                // 跳过多行注释
                while (token != tokenizer::TK_EOF && token != tokenizer::TK_COMMENT_END) {
                    token = scanner.next();
                    if (token == tokenizer::TK_EOL) {
                        next_line();
                    }
                }
                break;
            case tokenizer::TK_EOL:
                // 忽略换行
                next_line();
                break;
            case tokenizer::TK_EOF:
                // 扫描结束
                if (cur_token == tokenizer::TK_EOF) {
                    return;
                }
            default:
                prev_token = cur_token;
                cur_token = token;
                return;
        }
    }
}

class syntax_error_exception : public std::exception
{
public:
	syntax_error_exception(const std::string &s) throw(): m_s(s) {}
	~syntax_error_exception() throw() = default;

	virtual const char * what() const throw()
	{
		return m_s.c_str();
	}
protected:
	std::string m_s;
};

#define syntax_error(args) append_syntax_error(__FILE__, __LINE__, args)

// 追加一条错误信息
bool ast::append_syntax_error(std::string f, int l, std::initializer_list<std::string_view> args) {
    std::stringstream ss;
    
    ss <<  COLOR_GRAY << f << ":" << l << COLOR_RESET "\n";
    ss << "[" COLOR_RED "syntax error" COLOR_RESET "] [main.x:" << line << ":" << scanner.get_milestone() - line_offset + 1 << "] ";

    for (auto it = args.begin(); it != args.end(); ++it) {
        ss << COLOR_WHITE << *it << COLOR_RESET;
    }
    
    errors.push_back(ss.str());

    if (errors.size() > 9) {
        throw syntax_error_exception("to many errors");
    }

    return false;
}

#define syntax_error(args) append_syntax_error(__FILE__, __LINE__, args)
#define failover(...) process_failover(__FILE__, __LINE__, __VA_ARGS__, true)
#define failbefore(...) process_failover(__FILE__, __LINE__, __VA_ARGS__, false)

bool ast::process_failover(std::string f, int l, std::initializer_list<std::string_view> args, std::set<tokenizer::token_t> tokens, bool step_over) {
    if (args.size() > 0) {
        append_syntax_error(f, l, args);
    }
    while (cur_token != tokenizer::TK_EOF && tokens.find(cur_token.type) == tokens.end()) {
        next();
    }
    if (step_over && cur_token != tokenizer::TK_EOF) {
        next();
    }
    return false;
}

bool ast::process_failover(std::string f, int l, std::initializer_list<std::string_view> args, bool step_over) {
    return process_failover(f, l, args, {tokenizer::TK_SEMICOLON}, step_over);
}

bool ast::process_failover(std::string f, int l, std::set<tokenizer::token_t> tokens, bool step_over) {
    return process_failover(f, l, {}, tokens, step_over);
}

void ast::print() {
    print(root);

    for (auto it = errors.begin() ; it != errors.end() ; it++) {
        std::cout << *it << std::endl;
    }
}

void ast::print(std::unique_ptr<ast_node>& node) {
    print(node, 0);
}

void ast::print(std::unique_ptr<ast_node>& node, int indent) {
    node->print(indent);
}

// root <- package_declaration import_declarations? global_declarations?
bool ast::parse() {
    if (parsed) {
        return false;
    }

    try {
        next();

        parse_package_declaration(root);

        if (cur_token.type == tokenizer::TK_IMPORT) {
            parse_import_declarations(root);
        }

        if (cur_token.type == tokenizer::TK_VAR ||
            cur_token.type == tokenizer::TK_FUNCTION ||
            cur_token.type == tokenizer::TK_CONST ||
            cur_token.type == tokenizer::TK_TYPE) {
            parse_global_declarations(root);
        }

        if (errors.size() == 0 && cur_token.type != tokenizer::TK_EOF) {
            syntax_error({"<global declarations> expected"});
        }
    } catch (syntax_error_exception& e) {
        errors.push_back(e.what());
    }

    parsed = true;
    return errors.size() == 0;
}

// package_declaration <- TK_PACKAGE package_name TK_SEMICOLON
bool ast::parse_package_declaration(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(PACKAGE_DECLARATION);

    if (cur_token != tokenizer::TK_PACKAGE) {
        return failover({"<package declaration> expected"});
    }
    next();

    if (!parse_package_name(node)) {
        return false;
    }

    if (cur_token != tokenizer::TK_SEMICOLON) {
        return syntax_error({"expected ';' after package declaration"});
    }
    next();

    return true;
}

// package_name <- TK_IDENTIFIER [ TK_DOT TK_IDENTIFIER ]*
bool ast::parse_package_name(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(PACKAGE_NAME);

    if (cur_token != tokenizer::TK_IDENTIFIER) {
        return failover({"<package name> expected"});
    } else {
        parse_token(node);
    }

    while (cur_token == tokenizer::TK_DOT) {
        next();

        if (cur_token != tokenizer::TK_IDENTIFIER) {
            return failover({"illegal <package name>"});
        } else {
            parse_token(node);
        }
    }

    return true;
}

// import_declarations <- import_declaration*
bool ast::parse_import_declarations(std::unique_ptr<ast_node>& parent) {
    while (cur_token == tokenizer::TK_IMPORT) {
        parse_import_declaration(parent);
    }

    return true;
}

// import_declaration <- TK_IMPORT package_name package_rename? TK_SEMICOLON
bool ast::parse_import_declaration(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(IMPORT_DECLARATION);

    if (cur_token != tokenizer::TK_IMPORT) {
        return failover({"<import declaration> expected"});
    }
    next();

    if (!parse_package_name(node)) {
        return false;
    }

    if (cur_token == tokenizer::TK_AS) {
        if (!parse_package_rename(node)) {
            return false;
        }
    }

    if (cur_token != tokenizer::TK_SEMICOLON) {
        return syntax_error({"expected ';' after import declaration"});
    }
    next();

    return true;
}

// package_rename <- TK_AS TK_IDENTIFIER
bool ast::parse_package_rename(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(PACKAGE_RENAME);

    if (cur_token != tokenizer::TK_AS) {
        return failover({"'as' expected"});
    }
    next();

    if (cur_token != tokenizer::TK_IDENTIFIER) {
        return failover({"<identifier> expected"});
    } else {
        parse_token(node);
    }

    return true;
}

// global_declarations <- [ variable_declaration | function_declaration | constant_declaration | type_declaration ]*
bool ast::parse_global_declarations(std::unique_ptr<ast_node>& parent) {
    while (cur_token.type == tokenizer::TK_VAR ||
        cur_token.type == tokenizer::TK_FUNCTION ||
        cur_token.type == tokenizer::TK_CONST ||
        cur_token.type == tokenizer::TK_TYPE) {
        switch (cur_token.type) {
            case tokenizer::TK_VAR: parse_variable_declaration(parent); break;
            case tokenizer::TK_FUNCTION: parse_function_declaration(parent); break;
            case tokenizer::TK_CONST: parse_constant_declaration(parent); break;
            case tokenizer::TK_TYPE: parse_type_declaration(parent); break;
            default:
                return syntax_error({"assertion: parse_global_declarations"});
        }
    }

    return true;
}

// variable_declaration <- variable_declarator ;
bool ast::parse_variable_declaration(std::unique_ptr<ast_node>& parent) {
    if (!parse_variable_declarator(parent)) {
        return false;
    }

    if (cur_token != tokenizer::TK_SEMICOLON) {
        return syntax_error({"expected ';' after variable declaration"});
    }
    next();

    return true;
}

// variable_declarator <- TK_VAR TK_IDENTIFIER type_declarator? variable_initializer?
bool ast::parse_variable_declarator(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(VARIABLE_DECLARATION);

    if (cur_token != tokenizer::TK_VAR) {
        return failover({"'var' expected"});
    }
    next();

    if (cur_token != tokenizer::TK_IDENTIFIER) {
        return failover({"<identifier> expected"});
    } else {
        parse_token(node);
    }

    if (predict_type_declarator()) {
        if (!parse_type_declarator(node)) {
            return false;
        }
    }

    if (cur_token == tokenizer::TK_ASSIGN) {
        if (!parse_variable_initializer(node)) {
            return false;
        }
    }

    return true;
}

// constant_declaration <- TK_CONST TK_IDENTIFIER type_declarator? variable_initializer? TK_SEMICOLON
bool ast::parse_constant_declaration(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(CONSTANT_DECLARATION);

    if (cur_token != tokenizer::TK_CONST) {
        return failover({"'var' expected"});
    }
    next();

    if (cur_token != tokenizer::TK_IDENTIFIER) {
        return failover({"<identifier> expected"});
    } else {
        parse_token(node);
    }

    if (predict_type_declarator()) {
        if (!parse_type_declarator(node)) {
            return false;
        }
    }

    if (cur_token == tokenizer::TK_ASSIGN) {
        if (!parse_variable_initializer(node)) {
            return false;
        }
    }

    if (cur_token != tokenizer::TK_SEMICOLON) {
        return syntax_error({"expected ';' after import declaration"});
    }
    next();

    return true;
}

// type_declarator <- TK_INT | TK_FLOAT | TK_BOOL | TK_STRING
bool ast::parse_type_declarator(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(TYPE_DECLARATOR);

    if (cur_token != tokenizer::TK_INT &&
        cur_token != tokenizer::TK_FLOAT &&
        cur_token != tokenizer::TK_BOOL &&
        cur_token != tokenizer::TK_STRING) {
        return syntax_error({"<type declarator> expected"});
    } else {
        parse_token(node);
    }

    return true;
}

// variable_initializer <- TK_ASSIGN expression
bool ast::parse_variable_initializer(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(VARIABLE_INITIALIZER);

    if (cur_token != tokenizer::TK_ASSIGN) {
        return failover({"'=' expected"});
    }
    next();

    if (!parse_expression(node)) {
        return false;
    }

    return true;
}

bool ast::parse_expression(std::unique_ptr<ast_node>& parent) {
    return parse_expression(parent, 15);
}

int ast::operator_precedence(tokenizer::token_t t) {
    switch (t) {
        case tokenizer::TK_LEFT_PAREN: // ()
        case tokenizer::TK_LEFT_BRACK: // []
            return 0;
        case tokenizer::TK_DOT:
        case tokenizer::TK_RIGHT_ARROW:
            return 1;
        // case 单目运算符:
        //     return 2;
        case tokenizer::TK_MUL:
        case tokenizer::TK_DIV:
        case tokenizer::TK_MOD:
            return 3;
        case tokenizer::TK_ADD:
        case tokenizer::TK_SUB:
            return 4;
        case tokenizer::TK_BITWISE_SHL:
        case tokenizer::TK_BITWISE_SHR:
            return 5;
        case tokenizer::TK_GREATER:
        case tokenizer::TK_LESS:
        case tokenizer::TK_GREATER_EQUAL:
        case tokenizer::TK_LESS_EQUAL:
            return 6;
        case tokenizer::TK_EQUAL:
        case tokenizer::TK_NOT_EQUAL:
            return 7;
        case tokenizer::TK_BITWISE_AND:
            return 8;
        case tokenizer::TK_BITWISE_XOR:
            return 9;
        case tokenizer::TK_BITWISE_OR:
            return 10;
        case tokenizer::TK_LOGIC_AND:
            return 11;
        case tokenizer::TK_LOGIC_OR:
            return 12;
        //case '?':
        //    return 13;
        case tokenizer::TK_ASSIGN:
        case tokenizer::TK_ASSIGN_ADD:
        case tokenizer::TK_ASSIGN_SUB:
        case tokenizer::TK_ASSIGN_MUL:
        case tokenizer::TK_ASSIGN_DIV:
        case tokenizer::TK_ASSIGN_MOD:
        case tokenizer::TK_ASSIGN_AND:
        case tokenizer::TK_ASSIGN_OR:
        case tokenizer::TK_ASSIGN_XOR:
        case tokenizer::TK_ASSIGN_SHL:
        case tokenizer::TK_ASSIGN_SHR:
            return 14;
        default:
            return -1;
    }
}

// expression <- [ basic_expression | unary_op expression ] [ binary_op expression ]?
bool ast::parse_expression(std::unique_ptr<ast_node>& parent, int precedence) {
    switch (cur_token.type) {
        case tokenizer::TK_LOGIC_NOT: // 逻辑非运算符
        case tokenizer::TK_BITWISE_NOT: // 按位取反运算符
        case tokenizer::TK_SUB: // 负号运算符
        case tokenizer::TK_INC: // 自增运算符
        case tokenizer::TK_DEC: // 自减运算符
        case tokenizer::TK_CO:  // 协程运算符
        {
            std::unique_ptr<ast_node>& unary_expression = parent->add_child(UNARY_EXPRESSION);
            if (!parse_token(unary_expression)) {
                return false;
            } 
            if (!parse_expression(unary_expression, 2)) {
                return false;
            }
            break;
        }
        default:
            if (!parse_basic_expression(parent)) {
                return false;
            }
    }

    int p = operator_precedence(cur_token.type);
    while (true) {
        if (p < 0) {
            return true;
        } else {
            switch (cur_token.type) {
                case tokenizer::TK_LOGIC_AND:
                case tokenizer::TK_LOGIC_OR:
                case tokenizer::TK_ASSIGN:
                case tokenizer::TK_ASSIGN_ADD:
                case tokenizer::TK_ASSIGN_SUB:
                case tokenizer::TK_ASSIGN_MUL:
                case tokenizer::TK_ASSIGN_DIV:
                case tokenizer::TK_ASSIGN_MOD:
                case tokenizer::TK_ASSIGN_AND:
                case tokenizer::TK_ASSIGN_OR:
                case tokenizer::TK_ASSIGN_XOR:
                case tokenizer::TK_ASSIGN_SHL:
                case tokenizer::TK_ASSIGN_SHR:
                    // 右结合操作符
                    if (p > precedence) {
                        return true;
                    }
                    break;
                default:
                    // 左结合操作符
                    if (p >= precedence) {
                        return true;
                    }
            }
        }

        tokenizer::token_t t = cur_token.type;

        std::unique_ptr<ast_node>& binary_expression = parent->replace_child(BINARY_EXPRESSION);
        if (!parse_token(binary_expression)) {
            return false;
        }

        if (t == tokenizer::TK_LEFT_PAREN) {
            if (!parse_function_call_parameter(binary_expression)) {
                return false;
            }

            if (cur_token != tokenizer::TK_RIGHT_PAREN) {
                syntax_error({"')' expected"});
            } else {
                next();
            }
        } else if (t == tokenizer::TK_LEFT_BRACK) {
            if (cur_token != tokenizer::TK_RIGHT_BRACK) {
                if (!parse_expression(binary_expression)) {
                    return false;
                }
            }

            if (cur_token != tokenizer::TK_RIGHT_BRACK) {
                syntax_error({"']' expected"});
            } else {
                next();
            }
        } else {
            if (!parse_expression(binary_expression, p)) {
                return false;
            }
        }

        p = operator_precedence(cur_token.type);
    }
}

bool ast::parse_token(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(TOKEN);

    node->set_token(cur_token);
    next();

    return true;
}

// array_literal <- TK_LEFT_BRACK [ expression [ TK_COMMA expression ]* ]? TK_RIGHT_BRACK
bool ast::parse_array_literal(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(LITERAL_ARRAY_EXPRESSION);

    if (cur_token != tokenizer::TK_LEFT_BRACK) {
        return syntax_error({"'[' expected"});
    }
    next();

    if (cur_token != tokenizer::TK_RIGHT_BRACK) {
        if (!parse_expression(node)) {
            failbefore({tokenizer::TK_COMMA, tokenizer::TK_RIGHT_BRACK});
        }
        while (cur_token == tokenizer::TK_COMMA) {
            next();
            if (!parse_expression(node)) {
                failbefore({tokenizer::TK_COMMA, tokenizer::TK_RIGHT_BRACK});
            }
        }
    }

    if (cur_token != tokenizer::TK_RIGHT_BRACK) {
        return syntax_error({"']' expected"});
    }

    return true;
}

// object_literal <- TK_LEFT_BRACE [ key_value_pair [ TK_COMMA key_value_pair ]* ]? TK_RIGHT_BRACE
bool ast::parse_object_literal(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(LITERAL_OBJECT_EXPRESSION);

    if (cur_token != tokenizer::TK_LEFT_BRACE) {
        return syntax_error({"'{' expected"});
    }
    next();

    if (cur_token != tokenizer::TK_RIGHT_BRACE) {
        if (!parse_key_value_pair(node)) {
            failbefore({tokenizer::TK_COMMA, tokenizer::TK_RIGHT_BRACE});
        }
        while (cur_token == tokenizer::TK_COMMA) {
            next();
            if (!parse_key_value_pair(node)) {
                failbefore({tokenizer::TK_COMMA, tokenizer::TK_RIGHT_BRACE});
            }
        }
    }

    if (cur_token != tokenizer::TK_RIGHT_BRACE) {
        return syntax_error({"'}' expected"});
    }

    return true;
}

// key_value_pair <- TK_STRING TK_COLON expression
bool ast::parse_key_value_pair(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(KEY_VALUE_PAIR);

    if (cur_token != tokenizer::TK_STRING) {
        syntax_error({"<string> expected"});
    }
    next();

    if (cur_token != tokenizer::TK_COLON) {
        syntax_error({"':' expected"});
    }
    next();

    if (!parse_expression(node)) {
        return false;
    }

    return true;
}

// basic_expression -> TK_LITERAL_LONG | TK_LITERAL_DOUBLE | TK_LITERAL_STRING |
//     TK_LITERAL_CHAR | TK_TRUE | TK_FALSE | TK_NULL | TK_IDENTIFIER |
//     literal | TK_LEFT_PAREN expression TK_RIGHT_PAREN
bool ast::parse_basic_expression(std::unique_ptr<ast_node>& parent) {
    switch (cur_token.type) {
        case tokenizer::TK_LITERAL_LONG:
        case tokenizer::TK_LITERAL_DOUBLE:
        case tokenizer::TK_LITERAL_STRING:
        case tokenizer::TK_LITERAL_CHAR:
        case tokenizer::TK_TRUE:
        case tokenizer::TK_FALSE:
        case tokenizer::TK_NULL:
        case tokenizer::TK_IDENTIFIER:
            return parse_token(parent);
        case tokenizer::TK_LEFT_BRACK:
            return parse_array_literal(parent);
        case tokenizer::TK_LEFT_BRACE:
            return parse_object_literal(parent);
        case tokenizer::TK_LEFT_PAREN:
            next();
            if (!parse_expression(parent)) {
                return false;
            }
            if (cur_token != tokenizer::TK_RIGHT_PAREN) {
                return syntax_error({"')' expected"});  
            }
            next();
            return true;
        default:
            return syntax_error({"<basic expression> required"});
    }
}

// function_declaration <- TK_FUNCTION TK_IDENTIFIER function_decl_parameter type_declarator? block
bool ast::parse_function_declaration(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(FUNCTION_DECLARATION);

    if (cur_token != tokenizer::TK_FUNCTION) {
        return failover({"'func' expected"});
    }
    next();

    if (cur_token != tokenizer::TK_IDENTIFIER) {
        return failbefore({"<identifier> expected"},{tokenizer::TK_LEFT_PAREN});
    } else {
        parse_token(node);
    }

    bool ret = true;

    if (!parse_function_decl_parameter(node)) {
        ret = false;
    }

    if (predict_type_declarator()) {
        if (!parse_type_declarator(node)) {
            ret = false;
        }
    }

    if (parse_block(node)) {
        ret = false;
    }

    return ret;
}

// function_call_parameter <- [ expression [ TK_COMMA expression ]* ]?
bool ast::parse_function_call_parameter(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(FUNCTION_CALL_PARAMETER);

    if (cur_token == tokenizer::TK_RIGHT_PAREN) {
        return true;
    }

    if (!parse_expression(node)) {
        return false;
    }

    while (cur_token == tokenizer::TK_COMMA) {
        next();

        if (!parse_expression(node)) {
            return failbefore({tokenizer::TK_RIGHT_PAREN});
        }
    }

    return true;
}

// function_decl_parameter <- TK_LEFT_PAREN [ variable_declarator [ TK_COMMA variable_declaration ]* ]? TK_RIGHT_PAREN
bool ast::parse_function_decl_parameter(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(FUNCTION_DECL_PARAMETER);

    if (cur_token != tokenizer::TK_LEFT_PAREN) {
        syntax_error({"'(' expected"});
    }
    next();

    while (cur_token == tokenizer::TK_VAR) {
        if (!parse_variable_declarator(node)) {
            failbefore({tokenizer::TK_VAR, tokenizer::TK_RIGHT_PAREN});
            continue;
        }

        if (cur_token == tokenizer::TK_COMMA) {
            next();
            continue;
        } else {
            break;
        }
    }

    if (cur_token != tokenizer::TK_RIGHT_PAREN) {
        if (node->is_empty()) {
            return failover({"illegal <function_decl_parameter>"}, {tokenizer::TK_RIGHT_PAREN});
        } else {
            return syntax_error({"')' expected"});
        }
    }
    next();

    return true;
}

// block <- TK_LEFT_BRACE block_statements? TK_RIGHT_BRACE
bool ast::parse_block(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(BLOCK);

    if (cur_token != tokenizer::TK_LEFT_BRACE) {
        return syntax_error({"'{' expected"});
    }
    next();

    if (!parse_block_statements(node)) {
        return false;
    }

    if (cur_token != tokenizer::TK_RIGHT_BRACE) {
        return syntax_error({"'}' expected"});
    }
    next();

    return true;
}

// block_statements <- block_statement*
bool ast::parse_block_statements(std::unique_ptr<ast_node>& parent) {
    while (true) {
        if (predict_block_statement()) {
            parse_block_statement(parent);
        } else if (cur_token == tokenizer::TK_RIGHT_BRACE) {
            break;
        } else {
            failover({"unrecognized block statement"});
        }
    }
    return true;
}

// block_statement <- variable_declaration | constant_declaration | statement
bool ast::parse_block_statement(std::unique_ptr<ast_node>& parent) {
    if (cur_token == tokenizer::TK_VAR) {
        return parse_variable_declaration(parent);
    } else if (cur_token == tokenizer::TK_CONST) {
        return parse_constant_declaration(parent);
    } else {
        return parse_statement(parent);
    }
}

// statement <- if_statement | while_statement | do_statement | for_statement | continue_statement | break_statement | switch_statement | return_statement | try_statement | co_statement | expression_statement
bool ast::parse_statement(std::unique_ptr<ast_node>& parent) {
    switch (cur_token.type) {
        case tokenizer::TK_IF: return parse_if_statement(parent);
        case tokenizer::TK_WHILE: return parse_while_statement(parent);
        case tokenizer::TK_DO: return parse_do_statement(parent);
        case tokenizer::TK_FOR: return parse_for_statement(parent);
        case tokenizer::TK_CONTINUE: return parse_continue_statement(parent);
        case tokenizer::TK_BREAK: return parse_break_statement(parent);
        case tokenizer::TK_SWITCH: return parse_switch_statement(parent);
        case tokenizer::TK_RETURN: return parse_return_statement(parent);
        case tokenizer::TK_TRY: return parse_try_statement(parent);
        default:
            if (predict_expression_statement()) {
                return parse_expression_statement(parent);
            } else {
                return failover({"<statement> expected"});
            }
    }
}

// type_declaration <- TK_TYPE TK_IDENTIFIER type_declarator TK_SEMICOLON
bool ast::parse_type_declaration(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(TYPE_DECLARATION);

    if (cur_token != tokenizer::TK_TYPE) {
        return failover({"'type' expected"});
    }
    next();

    if (cur_token != tokenizer::TK_IDENTIFIER) {
        return failover({"<identifier> expected"});
    }
    next();

    if (!parse_type_declarator(node)) {
        return failover({"<type declarator> expected"});
    }
    next();

    if (cur_token != tokenizer::TK_SEMICOLON) {
        return syntax_error({"';' expected after type declaration"});
    }
    next();

    return true;
}

// if_statement <- TK_IF TK_LEFT_PAREN expression TK_RIGHT_PAREN block [ TK_ELSE [ block | if_statement ] ]?
bool ast::parse_if_statement(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(IF_STATEMENT);

    if (cur_token != tokenizer::TK_IF) {
        return failover({"'if' expected"});
    }
    next();

    if (cur_token != tokenizer::TK_LEFT_PAREN) {
        syntax_error({"'(' expected"});
    } else {
        next();
    }

    if (cur_token != tokenizer::TK_RIGHT_PAREN) {
        if (!parse_expression(node)) {
            failbefore({tokenizer::TK_RIGHT_PAREN});
        }
    }

    if (cur_token != tokenizer::TK_RIGHT_PAREN) {
        syntax_error({"')' expected"});
    } else {
        next();
    }

    if (!parse_block(node)) {
        return false;
    }

    if (cur_token != tokenizer::TK_ELSE) {
        return true;
    }
    next();

    if (cur_token == tokenizer::TK_IF) {
        if (parse_if_statement(node)) {
            return false;
        }
    } else {
        if (!parse_block(node)) {
            return false;
        }
    }

    return true;
}


// while_statement <- TK_WHILE TK_LEFT_PAREN expression TK_RIGHT_PAREN block
bool ast::parse_while_statement(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(WHILE_STATEMENT);

    if (cur_token != tokenizer::TK_WHILE) {
        return failover({"'while' expected"});
    }
    next();

    if (cur_token != tokenizer::TK_LEFT_PAREN) {
        syntax_error({"'(' expected"});
    } else {
        next();
    }

    if (cur_token != tokenizer::TK_RIGHT_PAREN) {
        if (!parse_expression(node)) {
            failbefore({tokenizer::TK_RIGHT_PAREN});
        }
    }

    if (cur_token != tokenizer::TK_RIGHT_PAREN) {
        syntax_error({"')' expected"});
    } else {
        next();
    }

    if (!parse_block(node)) {
        return false;
    }

    return true;
}

// do_statement <- TK_DO block TK_WHILE TK_LEFT_PAREN expression TK_RIGHT_PAREN TK_SEMICOLON
bool ast::parse_do_statement(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(DO_STATEMENT);

    if (cur_token != tokenizer::TK_DO) {
        return failover({"'do' expected"});
    }
    next();

    if (!parse_block(node)) {
        return false;
    }

    if (cur_token != tokenizer::TK_WHILE) {
        return failover({"'while' expected"});
    }
    next();

    if (cur_token != tokenizer::TK_LEFT_PAREN) {
        syntax_error({"'(' expected"});
    } else {
        next();
    }

    if (cur_token != tokenizer::TK_RIGHT_PAREN) {
        if (!parse_expression(node)) {
            failbefore({tokenizer::TK_RIGHT_PAREN});
        }
    }

    if (cur_token != tokenizer::TK_RIGHT_PAREN) {
        syntax_error({"')' expected"});
    } else {
        next();
    }

    if (cur_token != tokenizer::TK_SEMICOLON) {
        return syntax_error({"';' expected after do-while statement"});
    }
    next();

    return true;
}

// for_statement <- TK_FOR TK_LEFT_PAREN [ expression | variable_declarator ] TK_SEMICOLON expression TK_SEMICOLON expression TK_RIGHT_PAREN block
bool ast::parse_for_statement(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(FOR_STATEMENT);

    if (cur_token != tokenizer::TK_FOR) {
        return failover({"'for' expected"});
    }
    next();

    if (cur_token != tokenizer::TK_LEFT_PAREN) {
        syntax_error({"'(' expected"});
    } else {
        next();
    }

    if (cur_token != tokenizer::TK_VAR) {
        if (!parse_expression(node)) {
            failbefore({tokenizer::TK_SEMICOLON});
        }
    } else {
        if (!parse_variable_declarator(node)) {
            failbefore({tokenizer::TK_SEMICOLON});
        }
    }

    if (cur_token != tokenizer::TK_SEMICOLON) {
        syntax_error({"';' expected"});
    }
    next();

    if (!parse_expression(node)) {
        failbefore({tokenizer::TK_SEMICOLON});
    }

    if (cur_token != tokenizer::TK_SEMICOLON) {
        syntax_error({"';' expected"});
    }
    next();

    if (!parse_expression(node)) {
        failbefore({tokenizer::TK_SEMICOLON});
    }

    if (cur_token != tokenizer::TK_RIGHT_PAREN) {
        syntax_error({"')' expected"});
    } else {
        next();
    }

    if (!parse_block(node)) {
        return false;
    }

    return true;
}

// continue_statement <- TK_CONTINUE TK_SEMICOLON
bool ast::parse_continue_statement(std::unique_ptr<ast_node>& parent) {
    parent->add_child(CONTINUE_STATEMENT);

    if (cur_token != tokenizer::TK_CONTINUE) {
        return failover({"'continue' expected"});
    }
    next();

    if (cur_token != tokenizer::TK_SEMICOLON) {
        return syntax_error({"';' expected after continue statement"});
    }
    next();

    return true;
}

// break_statement <- TK_BREAK TK_SEMICOLON
bool ast::parse_break_statement(std::unique_ptr<ast_node>& parent) {
    parent->add_child(BREAK_STATEMENT);

    if (cur_token != tokenizer::TK_BREAK) {
        return failover({"'break' expected"});
    }
    next();

    if (cur_token != tokenizer::TK_SEMICOLON) {
        return syntax_error({"';' expected after break statement"});
    }
    next();

    return true;
}

bool ast::parse_switch_statement(std::unique_ptr<ast_node>& parent) {
    return failover({"unsupport switch statement"});
}

// return_statement <- TK_RETURN expression? TK_SEMICOLON
bool ast::parse_return_statement(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(RETURN_STATEMENT);

    if (cur_token != tokenizer::TK_RETURN) {
        return failover({"'return' expected"});
    }
    next();

    if (cur_token != tokenizer::TK_SEMICOLON) {
        if (!parse_expression(node)) {
            return failover({tokenizer::TK_SEMICOLON});
        }
    }

    if (cur_token != tokenizer::TK_SEMICOLON) {
        return syntax_error({"';' expected after return statement"});
    }
    next();

    return true;
}

bool ast::parse_try_statement(std::unique_ptr<ast_node>& parent) {
    return failover({"unsupport try statement"});
}

// expression_statement <- expression TK_SEMICOLON
bool ast::parse_expression_statement(std::unique_ptr<ast_node>& parent) {
    std::unique_ptr<ast_node>& node = parent->add_child(EXPRESSION_STATEMENT);

    if (!parse_expression(node)) {
        return failover({tokenizer::TK_SEMICOLON});
    }

    if (cur_token != tokenizer::TK_SEMICOLON) {
        return syntax_error({"';' expected after expression statement"});
    }
    next();

    return true;
}

bool ast::predict_type_declarator() {
    if (cur_token == tokenizer::TK_INT ||
        cur_token == tokenizer::TK_FLOAT ||
        cur_token == tokenizer::TK_BOOL ||
        cur_token == tokenizer::TK_STRING) {
        return true;
    }
    return false;
}

bool ast::predict_block_statement() {
    switch (cur_token.type) {
        case tokenizer::TK_IF:
        case tokenizer::TK_WHILE:
        case tokenizer::TK_DO:
        case tokenizer::TK_FOR:
        case tokenizer::TK_CONTINUE:
        case tokenizer::TK_BREAK:
        case tokenizer::TK_SWITCH:
        case tokenizer::TK_RETURN:
        case tokenizer::TK_TRY:
        case tokenizer::TK_CO:
        case tokenizer::TK_VAR:
        case tokenizer::TK_CONST:
            return true;
        default:
            return predict_expression_statement();
    }
}

bool ast::predict_block_statements() {
    return predict_block_statement();
}

// <assignment> | <preincrement expression> | <postincrement expression> | <predecrement expression> | <postdecrement expression> | <method invocation> | <class instance creation expression>
bool ast::predict_expression_statement() {
    switch (cur_token.type){
        case tokenizer::TK_IDENTIFIER:
        case tokenizer::TK_INC:
        case tokenizer::TK_DEC:
        case tokenizer::TK_LEFT_PAREN:
        case tokenizer::TK_CO:
            return true;
        default:
            return false;
    }
}

}
