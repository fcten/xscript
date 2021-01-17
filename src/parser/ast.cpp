#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include "ast.hpp"

namespace xscript::parser {

ast::ast(std::string s) :
    source(s),
    scanner(source),
    parsed(false)
{
    // TODO
    scanner.print();
    scanner.reset();
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
                        line++;
                    }
                }
                break;
            case tokenizer::TK_COMMENT_BEGIN:
                // 跳过多行注释
                while (token != tokenizer::TK_EOF && token != tokenizer::TK_COMMENT_END) {
                    token = scanner.next();
                    if (token == tokenizer::TK_EOL) {
                        line++;
                    }
                }
                break;
            case tokenizer::TK_EOL:
                // 忽略换行
                line++;
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

// 追加一条错误信息
void ast::syntax_error(std::initializer_list<std::string_view> args) {
    std::stringstream ss;
    
    ss << "[syntax error] [main.x:" << line << ":0] ";

    std::initializer_list<std::string_view>::iterator it;
    for (it = args.begin(); it != args.end(); ++it) {
        ss << *it;
    }

    ss << '\n';
    
    errors.push_back(ss.str());
}

// compilation_unit <- package_declaration? import_declarations? type_expression_declarations?
bool ast::parse() {
    if (parsed) {
        return false;
    }

    // 读取一个 token
    next();

    // TODO

    parsed = true;
    return true;
}

/*
bool ast::parse_string_token(std::shared_ptr<ast_node> parent) {
    std::shared_ptr<ast_node> string_token = std::make_shared<ast_node>(STRING_TOKEN);
    parent->add_child(string_token);

    if (cur_token != tokenizer::TK_LITERAL_STRING) {
        syntax_error({"`<string>` expected before `",cur_token.literal,"`"});
        return false;
    }

    // cur_token == TK_LITERAL_STRING
    next();

    return true;
}

bool parse_decl_parameter(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* param_list = ast_node_new(ast, FUNCTION_DECL_PARAMETER);
    ast_node_append_child(parent, param_list);

    if (ast->cur_token == TK_RIGHT_PAREN) {
        // 参数为空
        return 0;
    }

    while (1) {
        lgx_ast_node_t* variable_declaration = ast_node_new(ast, VARIABLE_DECLARATION);
        ast_node_append_child(param_list, variable_declaration);

        if (parse_identifier_token(ast, variable_declaration)) {
            return 1;
        }

        if (parse_type_expression(ast, variable_declaration)) {
            return 1;
        }

        if (ast->cur_token == TK_ASSIGN) {
            ast_step(ast);

            if (parse_expression(ast, variable_declaration)) {
                return 1;
            }
        }

        if (ast->cur_token != TK_COMMA) {
            return 0;
        }
        ast_step(ast);
    }
}

bool parse_decl_parameter_with_parentheses(std::shared_ptr<ast_node> parent) {
    if (ast->cur_token != TK_LEFT_PAREN) {
        ast_error(ast, "'(' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    if (parse_decl_parameter(parent)) {
        return 1;
    }

    if (ast->cur_token != TK_RIGHT_PAREN) {
        ast_error(ast, "')' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    return 0;
}

bool parse_call_parameter(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* param_list = ast_node_new(ast, FUNCTION_CALL_PARAMETER);
    ast_node_append_child(parent, param_list);

    if (ast->cur_token == TK_RIGHT_PAREN) {
        // 参数为空
        return 0;
    }

    while (1) {
        if (parse_expression(ast, param_list)) {
            return 1;
        }

        if (ast->cur_token != TK_COMMA) {
            return 0;
        }
        ast_step(ast);
    }
}

bool parse_block_statement_with_braces(std::shared_ptr<ast_node> parent);

bool parse_type_expression_function_parameter(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* function_parameter = ast_node_new(ast, FUNCTION_TYPE_DECL_PARAMETER);
    ast_node_append_child(parent, function_parameter);

    if (ast->cur_token != TK_LEFT_PAREN) {
        ast_error(ast, "'(' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    while (1) {
        if (parse_type_expression(ast, function_parameter)) {
            return 1;
        }

        if (ast->cur_token != TK_COMMA) {
            break;
        }
        ast_step(ast);
    }

    if (ast->cur_token != TK_RIGHT_PAREN) {
        ast_error(ast, "')' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    return 0;
}

bool parse_type_expression_struct(std::shared_ptr<ast_node> parent) {
    return 0;
}

bool parse_type_expression_interface(std::shared_ptr<ast_node> parent) {
    return 0;
}

bool parse_type_expression(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* type_expression = ast_node_new(ast, TYPE_EXPRESSION);
    ast_node_append_child(parent, type_expression);

    switch (ast->cur_token) {
        case TK_INT:
            type_expression->u.type = T_LONG;
            ast_step(ast);     
            break;
        case TK_FLOAT:
            type_expression->u.type = T_DOUBLE;
            ast_step(ast);
            break;
        case TK_BOOL:
            type_expression->u.type = T_BOOL;
            ast_step(ast);
            break;
        case TK_STRING:
            type_expression->u.type = T_STRING;
            ast_step(ast);
            break;
        case TK_IDENTIFIER:
            type_expression->u.type = T_CUSTOM;
            if (parse_identifier_token(ast, type_expression)) {
                return 1;
            }
            break;
        case TK_LEFT_BRACK:
            ast_step(ast);

            if (ast->cur_token == TK_RIGHT_BRACK) {
                type_expression->u.type = T_ARRAY;
                ast_step(ast);

                if (parse_type_expression(ast, type_expression)) {
                    return 1;
                }
            } else {
                type_expression->u.type = T_MAP;

                // 解析 key
                if (parse_type_expression(ast, type_expression)) {
                    return 1;
                }

                if (ast->cur_token != TK_RIGHT_BRACK) {
                    ast_error(ast, "`]` expected before '%.*s'\n", ast->cur_length, ast->cur_start);
                    return 1;
                }
                ast_step(ast);

                // 解析 value
                if (parse_type_expression(ast, type_expression)) {
                    return 1;
                }
            }
            break;
        case TK_STRUCT:
            type_expression->u.type = T_STRUCT;
            ast_step(ast);

            if (parse_type_expression_struct(ast, type_expression)) {
                return 1;
            }
            break;
        case TK_INTERFACE:
            type_expression->u.type = T_INTERFACE;
            ast_step(ast);

            if (parse_block_statement_with_braces(ast, type_expression)) {
                return 1;
            }
            break;
        case TK_FUNCTION:
            type_expression->u.type = T_FUNCTION;
            ast_step(ast);

            if (parse_type_expression_function_parameter(ast, type_expression)) {
                return 1;
            }

            if (parse_type_expression(ast, type_expression)) {
                return 1;
            }
            break;
        default:
            break;
    }

    return 0;
}

bool parse_array_expression(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* array_expression = ast_node_new(ast, ARRAY_EXPRESSION);
    ast_node_append_child(parent, array_expression);

    if (ast->cur_token != TK_LEFT_BRACK) {
        ast_error(ast, "`[` expected before '%.*s'\n", ast->cur_length, ast->cur_start);
        return 1;
    }

    // cur_token == '['
    ast_step(ast);

    while (1) {
        if (ast->cur_token == TK_RIGHT_BRACK) {
            break;
        }

        if (parse_expression(ast, array_expression)) {
            return 1;
        }

        if (ast->cur_token != TK_COMMA) {
            break;
        }
        ast_step(ast);
    }

    if (ast->cur_token != TK_RIGHT_BRACK) {
        ast_error(ast, "']' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    array_expression->length = ast->lex.offset - array_expression->offset;
    ast_step(ast);

    return 0;
}

bool parse_struct_expression(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* struct_expression = ast_node_new(ast, STRUCT_EXPRESSION);
    ast_node_append_child(parent, struct_expression);

    if (ast->cur_token != TK_LEFT_BRACE) {
        ast_error(ast, "`{` expected before '%.*s'\n", ast->cur_length, ast->cur_start);
        return 1;
    }

    // cur_token == '{'
    ast_step(ast);

    while (1) {
        if (ast->cur_token == TK_RIGHT_BRACE) {
            break;
        }

        if (parse_expression(ast, struct_expression)) {
            return 1;
        }

        if (ast->cur_token != TK_COLON) {
            ast_error(ast, "':' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
            return 1;
        }
        ast_step(ast);

        if (parse_expression(ast, struct_expression)) {
            return 1;
        }

        if (ast->cur_token != TK_COMMA) {
            break;
        }
        ast_step(ast);
    }

    if (ast->cur_token != TK_RIGHT_BRACE) {
        ast_error(ast, "'}' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    return 0;
}

// pri_expr -> ID | '(' sub_expr ')'
bool parse_pri_expression(std::shared_ptr<ast_node> parent) {
    switch (ast->cur_token) {
        case TK_LEFT_PAREN:
            return parse_expression_with_parentheses(parent);
        case TK_IDENTIFIER:
            return parse_identifier_token(parent);
        default:
            ast_error(ast, "`<expression>` expected before `%.*s`\n", ast->cur_length, ast->cur_start);
            return 1;
    }
}

// suf_expr -> pri_expr { '->' ID | '.' ID | '[' sub_expr ']' | funcargs }
bool parse_suf_expression(std::shared_ptr<ast_node> parent) {
    if (parse_pri_expression(parent)) {
        return 1;
    }

    while (1) {
        switch (ast->cur_token) {
            case TK_ARROW:
            case TK_DOT:
            case TK_LEFT_BRACK:
            case TK_LEFT_PAREN:
                break;
            default:
                return 0;
        }

        lgx_ast_node_t* binary_expression = ast_node_new(ast, BINARY_EXPRESSION);

        lgx_ast_node_t* last_child = ast_node_last_child(parent);
        binary_expression->parent = last_child->parent;
        binary_expression->offset = last_child->offset;
        binary_expression->length = last_child->length;
        binary_expression->line = last_child->line;
        binary_expression->row = last_child->row;

        ast_node_remove_child(parent, last_child);
        ast_node_append_child(binary_expression, last_child);
        ast_node_append_child(parent, binary_expression);

        switch (ast->cur_token) {
            case TK_ARROW:
            case TK_DOT:
                binary_expression->u.op = ast->cur_token;
                ast_step(ast);

                if (parse_identifier_token(ast, binary_expression)) {
                    return 1;
                }
                break;
            case TK_LEFT_BRACK:
                // 数组访问操作符
                binary_expression->u.op = TK_LEFT_BRACK;
                ast_step(ast);

                if (ast->cur_token != TK_RIGHT_BRACK) {
                    if (parse_expression(ast, binary_expression)) {
                        return 1;
                    }
                }

                if (ast->cur_token != TK_RIGHT_BRACK) {
                    ast_error(ast,  "']' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
                    return 1;
                }
                ast_step(ast);
                break;
            case TK_LEFT_PAREN:
                // 函数调用操作符
                binary_expression->u.op = TK_LEFT_PAREN;
                ast_step(ast);

                if (parse_call_parameter(ast, binary_expression)) {
                    return 1;
                }

                if (ast->cur_token != TK_RIGHT_PAREN) {
                    ast_error(ast, "')' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
                    return 1;
                }
                ast_step(ast);
                break;
            default:
                break;
        }
    }
}

// bsc_expr -> NUMBER | STRING | ARRAY | STRUCT | true | false | null | suf_expr
bool parse_bsc_expression(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* node;
    switch (ast->cur_token) {
        case TK_LITERAL_LONG:
            node = ast_node_new(ast, LONG_TOKEN);
            ast_node_append_child(parent, node);

            ast_step(ast);
            break;
        case TK_LITERAL_DOUBLE:
            node = ast_node_new(ast, DOUBLE_TOKEN);
            ast_node_append_child(parent, node);

            ast_step(ast);
            break;
        case TK_LITERAL_STRING:
            node = ast_node_new(ast, STRING_TOKEN);
            ast_node_append_child(parent, node);

            ast_step(ast);
            break;
        case TK_LITERAL_CHAR:
            node = ast_node_new(ast, CHAR_TOKEN);
            ast_node_append_child(parent, node);

            ast_step(ast);
            break;
        case TK_TRUE:
            node = ast_node_new(ast, TRUE_TOKEN);
            ast_node_append_child(parent, node);

            ast_step(ast);
            break;
        case TK_FALSE:
            node = ast_node_new(ast, FALSE_TOKEN);
            ast_node_append_child(parent, node);

            ast_step(ast);
            break;
        case TK_NULL:
            node = ast_node_new(ast, NULL_TOKEN);
            ast_node_append_child(parent, node);

            ast_step(ast);
            break;
        case TK_LEFT_BRACK:
            // 数组字面量
            return parse_array_expression(parent);
        case TK_LEFT_BRACE:
            // 结构体字面量
            return parse_struct_expression(parent);
        default:
            return parse_suf_expression(parent);
    }

    return 0;
}

bool ast_operator_precedence(int token) {
    switch (token) {
        // case () []
        // case ->
        // case 单目运算符
        case TK_MUL:
        case TK_DIV:
        case TK_MOD:
            return 3;
        case TK_ADD:
        case TK_SUB:
            return 4;
        case TK_SHL:
        case TK_SHR:
            return 5;
        case TK_GREATER:
        case TK_LESS:
        case TK_GREATER_EQUAL:
        case TK_LESS_EQUAL:
            return 6;
        case TK_EQUAL:
        case TK_NOT_EQUAL:
            return 7;
        case TK_AND:
            return 8;
        case TK_XOR:
            return 9;
        case TK_OR:
            return 10;
        case TK_LOGIC_AND:
            return 11;
        case TK_LOGIC_OR:
            return 12;
        //case '?':
        //    return 13;
        case TK_ASSIGN:
        case TK_ADD_ASSIGN:
        case TK_SUB_ASSIGN:
        case TK_MUL_ASSIGN:
        case TK_DIV_ASSIGN:
        case TK_MOD_ASSIGN:
        case TK_AND_ASSIGN:
        case TK_OR_ASSIGN:
        case TK_NOT_ASSIGN:
        case TK_XOR_ASSIGN:
        case TK_SHL_ASSIGN:
        case TK_SHR_ASSIGN:
            return 14;
        default:
            return -1;
    }
}

// sub_expr -> (bsc_expr | unary_op sub_expr) { binary_op sub_expr }
bool parse_sub_expression(std::shared_ptr<ast_node> parent, int precedence) {
    switch (ast->cur_token) {
        case TK_LOGIC_NOT: // 逻辑非运算符
        case TK_NOT: // 按位取反运算符
        case TK_SUB: // 负号运算符
        case TK_INC: // 自增运算符
        case TK_DEC: // 自减运算符
        {
            // 单目运算符
            lgx_ast_node_t* unary_expression = ast_node_new(ast, UNARY_EXPRESSION);
            unary_expression->u.op = ast->cur_token;
            ast_node_append_child(parent, unary_expression);

            ast_step(ast);

            if (parse_sub_expression(ast, unary_expression, 2)) {
                return 1;
            }
            break;
        }
        default:
            if (parse_bsc_expression(parent)) {
                return 1;
            }
    }

    int p = ast_operator_precedence(ast->cur_token);
    while (1) {
        if (p < 0) {
            return 0;
        } else {
            switch (ast->cur_token) {
                case TK_LOGIC_AND:
                case TK_LOGIC_OR:
                case TK_ASSIGN:
                case TK_ADD_ASSIGN:
                case TK_SUB_ASSIGN:
                case TK_MUL_ASSIGN:
                case TK_DIV_ASSIGN:
                case TK_MOD_ASSIGN:
                case TK_AND_ASSIGN:
                case TK_OR_ASSIGN:
                case TK_NOT_ASSIGN:
                case TK_XOR_ASSIGN:
                case TK_SHL_ASSIGN:
                case TK_SHR_ASSIGN:
                    // 右结合操作符
                    if (p > precedence) {
                        return 0;
                    }
                    break;
                default:
                    // 左结合操作符
                    if (p >= precedence) {
                        return 0;
                    }
            }
        }

        lgx_ast_node_t* binary_expression = ast_node_new(ast, BINARY_EXPRESSION);
        binary_expression->u.op = ast->cur_token;

        ast_step(ast);

        lgx_ast_node_t* last_child = ast_node_last_child(parent);
        binary_expression->parent = last_child->parent;
        binary_expression->offset = last_child->offset;
        binary_expression->length = last_child->length;
        binary_expression->line = last_child->line;
        binary_expression->row = last_child->row;

        ast_node_remove_child(parent, last_child);
        ast_node_append_child(binary_expression, last_child);
        ast_node_append_child(parent, binary_expression);

        if (parse_sub_expression(ast, binary_expression, p)) {
            return 1;
        }

        p = ast_operator_precedence(ast->cur_token);
    }
}

// 解析带括号的表达式
bool parse_expression_with_parentheses(std::shared_ptr<ast_node> parent) {
    if (ast->cur_token != TK_LEFT_PAREN) {
        ast_error(ast, "'(' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    if (parse_sub_expression(ast, parent, 15)) {
        return 1;
    }

    if (ast->cur_token != TK_RIGHT_PAREN) {
        ast_error(ast, "')' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    return 0;
}

bool parse_expression(std::shared_ptr<ast_node> parent) {
    return parse_sub_expression(ast, parent, 15);
}

bool parse_block_statement(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* block_statement = ast_node_new(ast, BLOCK_STATEMENT);
    ast_node_append_child(parent, block_statement);
    
    return parse_statement(ast, block_statement);
}

bool parse_block_statement_with_braces(std::shared_ptr<ast_node> parent) {
    if (ast->cur_token != TK_LEFT_BRACE) {
        ast_error(ast, "'{' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    if (parse_block_statement(parent)) {
        return 1;
    }

    if (ast->cur_token != TK_RIGHT_BRACE) {
        ast_error(ast, "'}' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    return 0;
}

bool parse_if_statement(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* if_statement = ast_node_new(ast, IF_STATEMENT);
    ast_node_append_child(parent, if_statement);

    assert(ast->cur_token == TK_IF);
    ast_step(ast);
    
    if (parse_expression_with_parentheses(ast, if_statement)) {
        return 1;
    }
    
    if (parse_block_statement_with_braces(ast, if_statement)) {
        return 1;
    }
    
    if (ast->cur_token != TK_ELSE) {
        return 0;
    }
    ast_step(ast);
    
    if_statement->type = IF_ELSE_STATEMENT;

    if (ast->cur_token == TK_IF) {
        return parse_if_statement(ast, if_statement);
    } else {
        return parse_block_statement_with_braces(ast, if_statement);
    }
}

bool parse_for_statement(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* for_statement = ast_node_new(ast, FOR_STATEMENT);
    ast_node_append_child(parent, for_statement);

    assert(ast->cur_token == TK_FOR);
    ast_step(ast);

    if (ast->cur_token != TK_LEFT_PAREN) {
        ast_error(ast, "'(' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    // 循环前表达式
    lgx_ast_node_t* for_condition = ast_node_new(ast, FOR_CONDITION);
    ast_node_append_child(for_statement, for_condition);

    if (ast->cur_token != TK_SEMICOLON) {
        if (parse_expression(ast, for_condition)) {
            return 1;
        }
    }

    if (ast->cur_token != TK_SEMICOLON) {
        ast_error(ast, "';' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    // 循环条件表达式
    for_condition = ast_node_new(ast, FOR_CONDITION);
    ast_node_append_child(for_statement, for_condition);

    if (ast->cur_token != TK_SEMICOLON) {
        if (parse_expression(ast, for_condition)) {
            return 1;
        }
    }

    if (ast->cur_token != TK_SEMICOLON) {
        ast_error(ast, "';' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    // 单次循环完毕表达式
    for_condition = ast_node_new(ast, FOR_CONDITION);
    ast_node_append_child(for_statement, for_condition);

    if (ast->cur_token != TK_RIGHT_PAREN) {
        if (parse_expression(ast, for_condition)) {
            return 1;
        }
    }

    if (ast->cur_token != TK_RIGHT_PAREN) {
        ast_error(ast, "')' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    return parse_block_statement_with_braces(ast, for_statement);
}

bool parse_while_statement(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* while_statement = ast_node_new(ast, WHILE_STATEMENT);
    ast_node_append_child(parent, while_statement);

    assert(ast->cur_token == TK_WHILE);
    ast_step(ast);
    
    if (parse_expression_with_parentheses(ast, while_statement)) {
        return 1;
    }
    
    return parse_block_statement_with_braces(ast, while_statement);
}

bool parse_do_statement(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* do_statement = ast_node_new(ast, DO_STATEMENT);
    ast_node_append_child(parent, do_statement);

    assert(ast->cur_token == TK_DO);
    ast_step(ast);
    
    if (parse_block_statement_with_braces(ast, do_statement)) {
        return 1;
    }

    if (ast->cur_token != TK_WHILE) {
        ast_error(ast, "'while' expected, not `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);
    
    return parse_expression_with_parentheses(ast, do_statement);
}

bool parse_break_statement(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* break_statement = ast_node_new(ast, BREAK_STATEMENT);
    ast_node_append_child(parent, break_statement);

    assert(ast->cur_token == TK_BREAK);
    ast_step(ast);

    return 0;
}

bool parse_continue_statement(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* continue_statement = ast_node_new(ast, CONTINUE_STATEMENT);
    ast_node_append_child(parent, continue_statement);

    assert(ast->cur_token == TK_CONTINUE);
    ast_step(ast);

    return 0;
}

bool parse_case_statement(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* case_statement = ast_node_new(ast, CASE_STATEMENT);
    ast_node_append_child(parent, case_statement);

    assert(ast->cur_token == TK_CASE);
    ast_step(ast);

    if (parse_expression(ast, case_statement)) {
        return 0;
    }

    if (ast->cur_token != TK_COLON) {
        ast_error(ast, "':' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    if (ast->cur_token == TK_LEFT_BRACE) {
        return parse_block_statement_with_braces(ast, case_statement);
    } else {
        return parse_block_statement(ast, case_statement);
    }
}

bool parse_default_statement(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* default_statement = ast_node_new(ast, DEFAULT_STATEMENT);
    ast_node_append_child(parent, default_statement);

    assert(ast->cur_token == TK_DEFAULT);
    ast_step(ast);

    if (ast->cur_token != TK_COLON) {
        ast_error(ast, "':' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    if (ast->cur_token == TK_LEFT_BRACE) {
        return parse_block_statement_with_braces(ast, default_statement);
    } else {
        return parse_block_statement(ast, default_statement);
    }
}

bool parse_switch_statement(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* switch_statement = ast_node_new(ast, SWITCH_STATEMENT);
    ast_node_append_child(parent, switch_statement);

    assert(ast->cur_token == TK_SWITCH);
    ast_step(ast);

    if (parse_expression_with_parentheses(ast, switch_statement)) {
        return 1;
    }

    if (ast->cur_token != TK_LEFT_BRACE) {
        ast_error(ast, "'{' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    // 解析所有 case 和 default
    while (1) {
        if (ast->cur_token == TK_CASE) {
            if (parse_case_statement(ast, switch_statement)) {
                return 1;
            }
        } else if (ast->cur_token == TK_DEFAULT) {
            if (parse_default_statement(ast, switch_statement)) {
                return 1;
            }
        } else {
            break;
        }
    }

    if (ast->cur_token != TK_RIGHT_BRACE) {
        ast_error(ast, "'}' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    return 0;
}

bool parse_try_statement(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* try_statement = ast_node_new(ast, TRY_STATEMENT);
    ast_node_append_child(parent, try_statement);

    assert(ast->cur_token == TK_TRY);
    ast_step(ast);

    if (parse_block_statement_with_braces(ast, try_statement)) {
        return 1;
    }

    while (ast->cur_token == TK_CATCH) {
        ast_step(ast);

        lgx_ast_node_t* catch_statement = ast_node_new(ast, CATCH_STATEMENT);
        ast_node_append_child(try_statement, catch_statement);

        if (parse_decl_parameter_with_parentheses(ast, catch_statement)) {
            return 1;
        }

        catch_statement->child[0]->type = EXCEPTION_DECL_PARAMETER;

        if (catch_statement->child[0]->children != 1) {
            ast_error(ast, "there must be one and only one parameter for catch block\n", ast->cur_length, ast->cur_start);
            return 1;
        }

        if (parse_block_statement_with_braces(ast, catch_statement)) {
            return 1;
        }
    }

    return 0;
}

bool parse_throw_statement(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* throw_statement = ast_node_new(ast, THROW_STATEMENT);
    ast_node_append_child(parent, throw_statement);

    assert(ast->cur_token == TK_THROW);
    ast_step(ast);

    return parse_expression(ast, throw_statement);
}

bool parse_return_statement(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* return_statement = ast_node_new(ast, RETURN_STATEMENT);
    ast_node_append_child(parent, return_statement);

    assert(ast->cur_token == TK_RETURN);
    ast_step(ast);

    return parse_expression(ast, return_statement);
}

bool parse_echo_statement(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* echo_statement = ast_node_new(ast, ECHO_STATEMENT);
    ast_node_append_child(parent, echo_statement);

    assert(ast->cur_token == TK_ECHO);
    ast_step(ast);

    return parse_expression(ast, echo_statement);
}

bool parse_co_statement(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* co_statement = ast_node_new(ast, CO_STATEMENT);
    ast_node_append_child(parent, co_statement);

    assert(ast->cur_token == TK_CO);
    ast_step(ast);

    return parse_expression(ast, co_statement);
}

bool parse_expression_statement(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* expression_statement = ast_node_new(ast, EXPRESSION_STATEMENT);
    ast_node_append_child(parent, expression_statement);
    
    return parse_expression(ast, expression_statement);
}

bool parse_import_declaration(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* import_declaration = ast_node_new(ast, IMPORT_DECLARATION);
    ast_node_append_child(parent, import_declaration);

    assert(ast->cur_token == TK_IMPORT);
    ast_step(ast);

    if (ast->cur_token == TK_IDENTIFIER) {
        if (parse_identifier_token(ast, import_declaration)) {
            return 1;
        }
    }

    return parse_string_token(ast, import_declaration);
}

bool parse_export_declaration(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* export_declaration = ast_node_new(ast, EXPORT_DECLARATION);
    ast_node_append_child(parent, export_declaration);

    assert(ast->cur_token == TK_EXPORT);
    ast_step(ast);

    // TODO 指定被导出的方法

    return 0;
}

bool parse_variable_declaration(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* variable_declaration = ast_node_new(ast, VARIABLE_DECLARATION);
    ast_node_append_child(parent, variable_declaration);

    assert(ast->cur_token == TK_VAR);
    ast_step(ast);

    while (1) {
        if (!variable_declaration) {
            variable_declaration = ast_node_new(ast, VARIABLE_DECLARATION);
            ast_node_append_child(parent, variable_declaration);
        }

        if (parse_identifier_token(ast, variable_declaration)) {
            return 1;
        }

        if (parse_type_expression(ast, variable_declaration)) {
            return 1;
        }

        if (ast->cur_token == TK_ASSIGN) {
            ast_step(ast);

            if (parse_expression(ast, variable_declaration)) {
                return 1;
            }
        }

        variable_declaration = NULL;

        switch (ast->cur_token) {
            case TK_COMMA:
                ast_step(ast);
                break;
            default:
                return 0;
        }
    }

    return 0;
}

bool parse_constant_declaration(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* constant_declaration = ast_node_new(ast, CONSTANT_DECLARATION);
    ast_node_append_child(parent, constant_declaration);

    assert(ast->cur_token == TK_CONST);
    ast_step(ast);

    while (1) {
        if (!constant_declaration) {
            constant_declaration = ast_node_new(ast, CONSTANT_DECLARATION);
            ast_node_append_child(parent, constant_declaration);
        }

        if (parse_identifier_token(ast, constant_declaration)) {
            return 1;
        }

        if (parse_type_expression(ast, constant_declaration)) {
            return 1;
        }

        if (ast->cur_token != TK_ASSIGN) {
            ast_error(ast, "`=` expected, constant must be initialized\n");
            return 1;
        }
        ast_step(ast);

        if (parse_expression(ast, constant_declaration)) {
            return 1;
        }

        constant_declaration = NULL;

        switch (ast->cur_token) {
            case TK_COMMA:
                ast_step(ast);
                break;
            default:
                return 0;
        }
    }

    return 0;
}

bool parse_function_receiver(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* function_receiver = ast_node_new(ast, FUNCTION_RECEIVER);
    ast_node_append_child(parent, function_receiver);\

    if (ast->cur_token != TK_LEFT_PAREN) {
        return 0;
    }

    return parse_decl_parameter_with_parentheses(ast, function_receiver);
}

bool parse_function_declaration(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* function_declaration = ast_node_new(ast, FUNCTION_DECLARATION);
    ast_node_append_child(parent, function_declaration);

    assert(ast->cur_token == TK_FUNCTION);
    ast_step(ast);

    // 接收者
    if (parse_function_receiver(ast, function_declaration)) {
        return 1;
    }

    // 函数名
    if (parse_identifier_token(ast, function_declaration)) {
        return 1;
    }

    // 参数
    if (parse_decl_parameter_with_parentheses(ast, function_declaration)) {
        return 1;
    }

    // 返回值
    if (parse_type_expression(ast, function_declaration)) {
        return 1;
    }

    // 函数体
    if (parse_block_statement_with_braces(ast, function_declaration)) {
        return 1;
    }

    return 0;
}

bool parse_type_declaration(std::shared_ptr<ast_node> parent) {
    lgx_ast_node_t* type_declaration = ast_node_new(ast, TYPE_DECLARATION);
    ast_node_append_child(parent, type_declaration);

    assert(ast->cur_token == TK_TYPE);
    ast_step(ast);

    if (parse_identifier_token(ast, type_declaration)) {
        return 1;
    }

    if (parse_type_expression(ast, type_declaration)) {
        return 1;
    }

    return 0;
}
*/

/*
bool ast::parse_statement(std::shared_ptr<ast_node> parent) {
    while(true) {
        switch (cur_token.type) {
            case tokenizer::TK_RIGHT_BRACE:
            case tokenizer::TK_CASE:
            case tokenizer::TK_DEFAULT:
                return 0;
            case tokenizer::TK_IF:
                if (parse_if_statement(parent)) {
                    return 1;
                }
                break;
            case tokenizer::TK_FOR:
                if (parse_for_statement(parent)) {
                    return 1;
                }
                break;
            case tokenizer::TK_WHILE:
                if (parse_while_statement(parent)) {
                    return 1;
                }
                break;
            case tokenizer::TK_DO:
                if (parse_do_statement(parent)) {
                    return 1;
                }
                break;
            case tokenizer::TK_BREAK:
                if (parse_break_statement(parent)) {
                    return 1;
                }
                break;
            case tokenizer::TK_CONTINUE:
                if (parse_continue_statement(parent)) {
                    return 1;
                }
                break;
            case tokenizer::TK_SWITCH:
                if (parse_switch_statement(parent)) {
                    return 1;
                }
                break;
            case tokenizer::TK_RETURN:
                if (parse_return_statement(parent)) {
                    return 1;
                }
                break;
            case tokenizer::TK_TRY:
                if (parse_try_statement(parent)) {
                    return 1;
                }
                break;
            case tokenizer::TK_THROW:
                if (parse_throw_statement(parent)) {
                    return 1;
                }
                break;
            case tokenizer::TK_ECHO:
                if (parse_echo_statement(parent)) {
                    return 1;
                }
                break;
            case tokenizer::TK_CO:
                if (parse_co_statement(parent)) {
                    return 1;
                }
                break;
            case tokenizer::TK_VAR:
                if (parse_variable_declaration(parent)) {
                    return 1;
                }
                break;
            case tokenizer::TK_CONST:
                if (parse_constant_declaration(parent)) {
                    return 1;
                }
                break;
            case tokenizer::TK_TYPE:
                if (parse_type_declaration(parent)) {
                    return 1;
                }
                break;
            default:
                if (parse_expression_statement(parent)) {
                    return 1;
                }
        }

        // 语句可以以分号结尾
        if (cur_token == tokenizer::TK_SEMICOLON) {
            next();
        }
    }
}

bool ast::parse_declaration(std::shared_ptr<ast_node> parent) {
    // 解析语句
    while (true) {
        switch (cur_token.type) {
            case tokenizer::TK_EOF:
                // 读取到文件末尾，解析结束
                return true;
            case tokenizer::TK_IMPORT:
                if (parse_import_declaration(parent)) {
                    return false;
                }
                break;
            case tokenizer::TK_EXPORT:
                if (parse_export_declaration(parent)) {
                    return false;
                }
                break;
            case tokenizer::TK_VAR:
                if (parse_variable_declaration(parent)) {
                    return false;
                }
                break;
            case tokenizer::TK_CONST:
                if (parse_constant_declaration(parent)) {
                    return false;
                }
                break;
            case tokenizer::TK_FUNCTION:
                if (parse_function_declaration(parent)) {
                    return false;
                }
                break;
            case tokenizer::TK_TYPE:
                if (parse_type_declaration(parent)) {
                    return false;
                }
                break;
            default:
                break;
        }
        
        // 语句可以以分号结尾
        if (cur_token == tokenizer::TK_SEMICOLON) {
            next();
        }
    }
}

bool ast::parse_package_declaration(std::shared_ptr<ast_node> parent) {
    assert(cur_token == tokenizer::TK_PACKAGE);

    // cur_token == TK_PACKAGE
    next();

    // TODO 解析包名
    if (cur_token != tokenizer::TK_IDENTIFIER) {
        syntax_error({"`<identifier>` expected before '",cur_token.literal,"'\n"});
        return 1;
    }

    // cur_token == TK_IDENTIFIER
    next();

    return true;
}


*/
}
