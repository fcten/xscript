#include "../common/common.h"
#include "../common/val.h"
#include "scope.h"
#include "ast.h"

void ast_parse_statement(lgx_ast_t* ast, lgx_ast_node_t* parent);

void ast_parse_bsc_expression(lgx_ast_t* ast, lgx_ast_node_t* parent);
void ast_parse_pri_expression(lgx_ast_t* ast, lgx_ast_node_t* parent);
void ast_parse_suf_expression(lgx_ast_t* ast, lgx_ast_node_t* parent);
void ast_parse_sub_expression(lgx_ast_t* ast, lgx_ast_node_t* parent, int precedence);
void ast_parse_expression(lgx_ast_t* ast, lgx_ast_node_t* parent);

void ast_parse_variable_declaration(lgx_ast_t* ast, lgx_ast_node_t* parent);
void ast_parse_function_declaration(lgx_ast_t* ast, lgx_ast_node_t* parent);

lgx_ast_node_t* ast_node_new(lgx_ast_t* ast, int n) {
    lgx_ast_node_t* node = xcalloc(1, sizeof(lgx_ast_node_t) + n * sizeof(lgx_ast_node_t*));
    // TODO ERROR CHECK
    node->children = 0;
    node->size = n;

    node->line = ast->cur_line;

    return node;
}

lgx_ast_node_token_t* ast_node_token_new(lgx_ast_t* ast) {
    lgx_ast_node_token_t* node = xcalloc(1, sizeof(lgx_ast_node_token_t));

    node->tk_start = ast->cur_start;
    node->tk_length = ast->cur_length;

    node->line = ast->cur_line;
    
    return node;
}

lgx_ast_node_t* ast_node_append_child(lgx_ast_node_t* parent, lgx_ast_node_t* child) {
    if (parent->children >= parent->size) {
        parent = xrealloc(parent, sizeof(lgx_ast_node_t) + 2 * parent->size * sizeof(lgx_ast_node_t*));
        // TODO ERROR CHECK
        parent->size *= 2;
    }
    
    parent->child[parent->children++] = child;
    child->parent = parent;
    
    return parent;
}

int lgx_ast_init(lgx_ast_t* ast) {
    memset(ast, 0, sizeof(lgx_ast_t));

    ast->lex.line = 1;
    ast->cur_line = 1;

    ast->err_info = xmalloc(256);
    ast->err_len = 0;

    return 0;
}

void ast_node_cleanup(lgx_ast_node_t* node) {
    int i;
    switch (node->type) {
        case IDENTIFIER_TOKEN:
        case UNDEFINED_TOKEN:
        case LONG_TOKEN:
        case DOUBLE_TOKEN:
        case STRING_TOKEN:
        case TRUE_TOKEN:
        case FALSE_TOKEN:
            xfree(node);
            return;
        case BLOCK_STATEMENT:
            if (node->u.symbols) {
                lgx_hash_delete(node->u.symbols);
            }
            break;
        case FOR_STATEMENT:
        case WHILE_STATEMENT:
        case DO_WHILE_STATEMENT:
        case SWITCH_STATEMENT:
        case FUNCTION_DECLARATION:
            if (node->u.jmps) {
                while (!lgx_list_empty(&node->u.jmps->head)) {
                    lgx_ast_node_list_t *n = lgx_list_first_entry(&node->u.jmps->head, lgx_ast_node_list_t, head);
                    lgx_list_del(&n->head);
                    xfree(n);
                }
                xfree(node->u.jmps);
            }
            break;
    }
    for(i = 0; i < node->children; i++) {
        if (node->child[i]) {
            ast_node_cleanup(node->child[i]);
        }
    }
    xfree(node);
}

int lgx_ast_cleanup(lgx_ast_t* ast) {
    xfree(ast->err_info);
    xfree(ast->lex.source);

    ast_node_cleanup(ast->root);

    return 0;
}

void ast_error(lgx_ast_t* ast, const char *fmt, ...) {
    va_list   args;

    if (ast->errno) {
        return;
    }

    va_start(args, fmt);
    ast->err_len = vsnprintf(ast->err_info, 256, fmt, args);
    va_end(args);
    
    ast->errno = 1;
}

int ast_next(lgx_ast_t* ast) {
    unsigned int token;
    while(1) {
        token = lgx_lex(&ast->lex);
        switch (token) {
            case TK_SPACE:
            case TK_COMMENT:
            case TK_EOL:
                break;
            default:
                return token;
        }
    }
}

void ast_step(lgx_ast_t* ast) {
    ast->cur_token = ast_next(ast);
    ast->cur_start = ast->lex.source + ast->lex.milestone;
    ast->cur_length = ast->lex.offset - ast->lex.milestone;
    ast->cur_line = ast->lex.line;
}

void ast_parse_id_token(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_token_t* id = ast_node_token_new(ast);
    id->type = IDENTIFIER_TOKEN;
    ast_node_append_child(parent, (lgx_ast_node_t*)id);

    ast_step(ast);
}

void ast_parse_decl_parameter(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* param_list = ast_node_new(ast, 128);
    param_list->type = FUNCTION_DECL_PARAMETER;
    ast_node_append_child(parent, param_list);

    if (ast->cur_token == ')') {
        // 参数为空
        return;
    }

    while (1) {
        lgx_ast_node_t* variable_declaration = ast_node_new(ast, 2);
        variable_declaration->type = VARIABLE_DECLARATION;
        ast_node_append_child(param_list, variable_declaration);

        if (ast->cur_token != TK_AUTO) {
            ast_error(ast, "[Error] [Line:%d] type declaration expected\n", ast->cur_line);
            return;
        }
        ast_step(ast);

        if (ast->cur_token != TK_ID) {
            ast_error(ast, "[Error] [Line:%d] `%.*s` is not a <identifier>\n", ast->cur_line, ast->cur_length, ast->cur_start);
            return;
        } else {
            ast_parse_id_token(ast, variable_declaration);
        }
        
        if (ast->cur_token == '=') {
            ast_step(ast);

            ast_parse_expression(ast, variable_declaration);
        }

        if (ast->cur_token != ',') {
            return;
        }
        ast_step(ast);
    }
}

void ast_parse_call_parameter(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* param_list = ast_node_new(ast, 128);
    param_list->type = FUNCTION_CALL_PARAMETER;
    ast_node_append_child(parent, param_list);

    while (1) {
        ast_parse_expression(ast, param_list);

        if (ast->cur_token != ',') {
            return;
        }
        ast_step(ast);
    }
}

void ast_parse_array_expression(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* array = ast_node_new(ast, 128);
    array->type = ARRAY_TOKEN;
    ast_node_append_child(parent, array);

    // ast->cur_token == '['
    ast_step(ast);

    while (1) {
        ast_parse_expression(ast, array);

        if (ast->cur_token != ',') {
            break;
        }
        ast_step(ast);
    }

    if (ast->cur_token != ']') {
        ast_error(ast, "[Error] [Line:%d] ']' expected\n", ast->cur_line);
        return;
    }
    ast_step(ast);
}

// pri_expr -> ID | '(' sub_expr ')'
void ast_parse_pri_expression(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    switch (ast->cur_token) {
        case '(':
            ast_step(ast);

            ast_parse_expression(ast, parent);

            if (ast->cur_token != ')') {
                ast_error(ast, "[Error] [Line:%d] ')' expected\n", ast->cur_line);
                return;
            }
            ast_step(ast);
            break;
        case TK_ID:
            ast_parse_id_token(ast, parent);
            break;
        default:
            return;
    }
}

// suf_expr -> pri_expr { '.' ID | '[' sub_expr ']' | funcargs }
void ast_parse_suf_expression(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    ast_parse_pri_expression(ast, parent);

    lgx_ast_node_t* binary_expression;
    while (1) {
        if (ast->cur_token != '.' && ast->cur_token != '[' && ast->cur_token != '(') {
            return;
        }

        binary_expression = ast_node_new(ast, 2);
        binary_expression->type = BINARY_EXPRESSION;
        binary_expression->parent = parent;
        ast_node_append_child(binary_expression, parent->child[parent->children-1]);
        parent->child[parent->children-1] = binary_expression;

        switch (ast->cur_token) {
            case '.':
                binary_expression->u.op = TK_ATTR;
                ast_step(ast);

                if (ast->cur_token != TK_ID) {
                    ast_error(ast, "[Error] [Line:%d] ']' <identifier> expected\n", ast->cur_line);
                    return;
                }
                ast_parse_id_token(ast, binary_expression);
                break;
            case '[':
                // 数组访问操作符
                binary_expression->u.op = TK_INDEX;
                ast_step(ast);

                ast_parse_expression(ast, binary_expression);

                if (ast->cur_token != ']') {
                    ast_error(ast, "[Error] [Line:%d] ']' expected\n", ast->cur_line);
                    return;
                }
                ast_step(ast);
                break;
            case '(':
                // 函数调用操作符
                binary_expression->u.op = TK_CALL;
                ast_step(ast);

                ast_parse_call_parameter(ast, binary_expression);

                if (ast->cur_token != ')') {
                    ast_error(ast, "[Error] [Line:%d] ')' expected\n", ast->cur_line);
                    return;
                }
                ast_step(ast);
                break;
        }
    }
}

// bsc_expr -> NUMBER | STRING | ARRAY | OBJECT | true | false | suf_expr
void ast_parse_bsc_expression(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_token_t* id;
    switch (ast->cur_token) {
        case TK_LONG:
            id = ast_node_token_new(ast);
            id->type = LONG_TOKEN;
            ast_node_append_child(parent, (lgx_ast_node_t*)id);

            ast_step(ast);
            break;
        case TK_DOUBLE:
            id = ast_node_token_new(ast);
            id->type = DOUBLE_TOKEN;
            ast_node_append_child(parent, (lgx_ast_node_t*)id);

            ast_step(ast);
            break;
        case TK_STRING:
            id = ast_node_token_new(ast);
            id->type = STRING_TOKEN;
            ast_node_append_child(parent, (lgx_ast_node_t*)id);

            ast_step(ast);
            break;
        case TK_TRUE:
            id = ast_node_token_new(ast);
            id->type = TRUE_TOKEN;
            ast_node_append_child(parent, (lgx_ast_node_t*)id);

            ast_step(ast);
            break;
        case TK_FALSE:
            id = ast_node_token_new(ast);
            id->type = FALSE_TOKEN;
            ast_node_append_child(parent, (lgx_ast_node_t*)id);

            ast_step(ast);
            break;
        case TK_UNDEFINED:
            id = ast_node_token_new(ast);
            id->type = UNDEFINED_TOKEN;
            ast_node_append_child(parent, (lgx_ast_node_t*)id);

            ast_step(ast);
            break;
        case '[':
            // 数组字面量
            ast_parse_array_expression(ast, parent);
            break;
        default:
            ast_parse_suf_expression(ast, parent);
    }
}

int ast_operator_precedence(int token) {
    switch (token) {
        // () []     = 1
        // 单目运算符 = 2
        case '*':
        case '/':
        case '%':
            return 3;
        case '+':
        case '-':
            return 4;
        case TK_SHL:
        case TK_SHR:
            return 5;
        case '>':
        case '<':
        case TK_GE:
        case TK_LE:
            return 6;
        case TK_EQ:
        case TK_NE:
            return 7;
        case '&':
            return 8;
        case '^':
            return 9;
        case '|':
            return 10;
        case TK_AND:
            return 11;
        case TK_OR:
            return 12;
        case '?':
            return 13;
        case '=':
            return 14;
        default:
            return -1;
    }
}

// sub_expr -> (bsc_expr | unary_op sub_expr) { binary_op sub_expr }
void ast_parse_sub_expression(lgx_ast_t* ast, lgx_ast_node_t* parent, int precedence) {
    lgx_ast_node_t* unary_expression;
    switch (ast->cur_token) {
        case '!': // 逻辑非运算符
        case '~': // 按位取反运算符
        case '-': // 负号运算符
            // 单目运算符
            unary_expression = ast_node_new(ast, 1);
            unary_expression->type = UNARY_EXPRESSION;
            unary_expression->u.op = ast->cur_token;
            ast_node_append_child(parent, unary_expression);

            ast_step(ast);

            ast_parse_sub_expression(ast, unary_expression, 2);
            break;
        default:
            ast_parse_bsc_expression(ast, parent);
    }

    int p = ast_operator_precedence(ast->cur_token);
    lgx_ast_node_t* binary_expression;
    while (1) {
        if (p < 0) {
            return;
        } else {
            switch (ast->cur_token) {
                case TK_AND: case TK_OR: case '=':
                    // 右结合操作符
                    if (p > precedence) {
                        return;
                    }
                    break;
                default:
                    // 左结合操作符
                    if (p >= precedence) {
                        return;
                    }
            }
        }

        binary_expression = ast_node_new(ast, 2);
        binary_expression->type = BINARY_EXPRESSION;
        binary_expression->parent = parent;
        binary_expression->u.op = ast->cur_token;
        ast_node_append_child(binary_expression, parent->child[parent->children-1]);
        parent->child[parent->children-1] = binary_expression;

        ast_step(ast);

        ast_parse_sub_expression(ast, binary_expression, p);
        if (binary_expression->children == 1) {
            ast_error(ast, "[Error] [Line:%d] unexpected right value for binary operation near `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
            return;
        }

        p = ast_operator_precedence(ast->cur_token);
    }
}

void ast_parse_expression_with_parentheses(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    if (ast->cur_token != '(') {
        ast_error(ast, "[Error] [Line:%d] '(' expected near `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);

    ast_parse_sub_expression(ast, parent, 15);

    if (ast->cur_token != ')') {
        ast_error(ast, "[Error] [Line:%d] ')' expected near `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);
}

void ast_parse_expression(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    ast_parse_sub_expression(ast, parent, 15);
}

void ast_parse_block_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* block_statement = ast_node_new(ast, 128);
    block_statement->type = BLOCK_STATEMENT;
    ast_node_append_child(parent, block_statement);

    // 创建新的作用域
    block_statement->u.symbols = lgx_hash_new(32);

    // 优先把函数参数添加到符号表
    if (parent->type == FUNCTION_DECLARATION) {
        lgx_str_t s;
        for (int i = 0; i < parent->child[1]->children; i++) {
            s.buffer = ((lgx_ast_node_token_t *)parent->child[1]->child[i]->child[0])->tk_start;
            s.length = ((lgx_ast_node_token_t *)parent->child[1]->child[i]->child[0])->tk_length;

            lgx_scope_val_add(parent->child[2], &s);
        }
    }
    
    ast_parse_statement(ast, block_statement);
}

void ast_parse_block_statement_with_braces(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    if (ast->cur_token != '{') {
        ast_error(ast, "[Error] [Line:%d] '{' expected near `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);

    ast_parse_block_statement(ast, parent);

    if (ast->cur_token != '}') {
        ast_error(ast, "[Error] [Line:%d] '}' expected near `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);
}

void ast_parse_if_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* if_statement = ast_node_new(ast, 3);
    if_statement->type = IF_STATEMENT;
    ast_node_append_child(parent, if_statement);

    // ast->cur_token == TK_IF
    ast_step(ast);
    
    ast_parse_expression_with_parentheses(ast, if_statement);
    if (if_statement->children == 0) {
        ast_error(ast, "[Error] [Line:%d] expression expected near `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
    }
    
    ast_parse_block_statement_with_braces(ast, if_statement);
    
    if (ast->cur_token != TK_ELSE) {
        return;
    }
    ast_step(ast);
    
    if_statement->type = IF_ELSE_STATEMENT;
    ast_parse_block_statement_with_braces(ast, if_statement);
}

void ast_parse_for_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* for_statement = ast_node_new(ast, 4);
    for_statement->type = FOR_STATEMENT;
    ast_node_append_child(parent, for_statement);

    // ast->cur_token == TK_FOR
    ast_step(ast);

    if (ast->cur_token != '(') {
        ast_error(ast, "[Error] [Line:%d] '(' expected near `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);

    ast_parse_expression(ast, for_statement);
    if (for_statement->children == 0) {
        for_statement->children ++;
    }

    if (ast->cur_token != ';') {
        ast_error(ast, "[Error] [Line:%d] ';' expected near `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);

    ast_parse_expression(ast, for_statement);
    if (for_statement->children == 1) {
        for_statement->children ++;
    }

    if (ast->cur_token != ';') {
        ast_error(ast, "[Error] [Line:%d] ';' expected\n", ast->cur_line);
        return;
    }
    ast_step(ast);

    ast_parse_expression(ast, for_statement);
    if (for_statement->children == 2) {
        for_statement->children ++;
    }

    if (ast->cur_token != ')') {
        ast_error(ast, "[Error] [Line:%d] ')' expected\n", ast->cur_line);
        return;
    }
    ast_step(ast);

    ast_parse_block_statement_with_braces(ast, for_statement);
}

void ast_parse_while_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* while_statement = ast_node_new(ast, 2);
    while_statement->type = WHILE_STATEMENT;
    ast_node_append_child(parent, while_statement);

    // ast->cur_token == TK_WHILE
    ast_step(ast);
    
    ast_parse_expression_with_parentheses(ast, while_statement);
    if (while_statement->children == 0) {
        ast_error(ast, "[Error] [Line:%d] expression expected near `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
    }
    
    ast_parse_block_statement_with_braces(ast, while_statement);
}

void ast_parse_do_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* do_statement = ast_node_new(ast, 2);
    do_statement->type = DO_WHILE_STATEMENT;
    ast_node_append_child(parent, do_statement);

    // ast->cur_token == TK_DO
    ast_step(ast);
    
    ast_parse_block_statement_with_braces(ast, do_statement);

    if (ast->cur_token != TK_WHILE) {
        ast_error(ast, "[Error] [Line:%d] 'while' expected, not `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);
    
    ast_parse_expression_with_parentheses(ast, do_statement);
}

void ast_parse_break_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* break_statement = ast_node_new(ast, 0);
    break_statement->type = BREAK_STATEMENT;
    ast_node_append_child(parent, break_statement);

    // ast->cur_token == TK_BREAK
    ast_step(ast);
}

void ast_parse_continue_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* continue_statement = ast_node_new(ast, 0);
    continue_statement->type = CONTINUE_STATEMENT;
    ast_node_append_child(parent, continue_statement);

    // ast->cur_token == TK_CONTINUE
    ast_step(ast);
}

void ast_parse_case_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* case_statement = ast_node_new(ast, 2);
    case_statement->type = CASE_STATEMENT;
    ast_node_append_child(parent, case_statement);

    // ast->cur_token == TK_CASE
    ast_step(ast);

    ast_parse_expression(ast, case_statement);
    if (case_statement->children == 0) {
        ast_error(ast, "[Error] [Line:%d] expression expected near `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
    }

    if (ast->cur_token != ':') {
        ast_error(ast, "[Error] [Line:%d] ':' expected near `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);

    if (ast->cur_token == '{') {
        ast_parse_block_statement_with_braces(ast, case_statement);
    } else {
        ast_parse_block_statement(ast, case_statement);
    }
}

void ast_parse_default_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* default_statement = ast_node_new(ast, 1);
    default_statement->type = DEFAULT_STATEMENT;
    ast_node_append_child(parent, default_statement);

    // ast->cur_token == TK_DEFAULT
    ast_step(ast);

    if (ast->cur_token != ':') {
        ast_error(ast, "[Error] [Line:%d] ':' expected near `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);

    if (ast->cur_token == '{') {
        ast_parse_block_statement_with_braces(ast, default_statement);
    } else {
        ast_parse_block_statement(ast, default_statement);
    }
}

void ast_parse_switch_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* switch_statement = ast_node_new(ast, 128);
    switch_statement->type = SWITCH_STATEMENT;
    ast_node_append_child(parent, switch_statement);

    // ast->cur_token == TK_SWITCH
    ast_step(ast);

    ast_parse_expression_with_parentheses(ast, switch_statement);
    if (switch_statement->children == 0) {
        ast_error(ast, "[Error] [Line:%d] expression expected near `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
    }

    if (ast->cur_token != '{') {
        ast_error(ast, "[Error] [Line:%d] '{' expected near `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);

    // 解析所有 case 和 default
    int has_default = 0;
    while (1) {
        if (ast->cur_token == TK_CASE) {
            ast_parse_case_statement(ast, switch_statement);
        } else if (ast->cur_token == TK_DEFAULT) {
            if (has_default) {
                ast_error(ast, "[Error] [Line:%d] switch statement should only contain one default label\n", ast->cur_line);
                return;
            }
            has_default = 1;

            ast_parse_default_statement(ast, switch_statement);
        } else {
            break;
        }
    }

    if (ast->cur_token != '}') {
        ast_error(ast, "[Error] [Line:%d] '}' expected near `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);
}

void ast_parse_return_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* return_statement = ast_node_new(ast, 1);
    return_statement->type = RETURN_STATEMENT;;
    ast_node_append_child(parent, return_statement);

    // ast->cur_token == TK_RETURN
    ast_step(ast);

    ast_parse_expression(ast, return_statement);
}

void ast_parse_echo_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* echo_statement = ast_node_new(ast, 1);
    echo_statement->type = ECHO_STATEMENT;;
    ast_node_append_child(parent, echo_statement);

    // ast->cur_token == TK_ECHO
    ast_step(ast);
    
    ast_parse_expression(ast, echo_statement);
}

void ast_parse_expression_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* assign_statement = ast_node_new(ast, 1);
    assign_statement->type = EXPRESSION_STATEMENT;
    ast_node_append_child(parent, assign_statement);
    
    ast_parse_expression(ast, assign_statement);
}

void ast_parse_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    while(1) {
        switch (ast->cur_token) {
            case ';':
                // null statement
                ast_step(ast);
                break;
            case TK_IF:
                ast_parse_if_statement(ast, parent);
                continue;
            case TK_FOR:
                ast_parse_for_statement(ast, parent);
                continue;
            case TK_WHILE:
                ast_parse_while_statement(ast, parent);
                continue;
            case TK_DO:
                ast_parse_do_statement(ast, parent);
                break;
            case TK_BREAK:
                ast_parse_break_statement(ast, parent);
                break;
            case TK_CONTINUE:
                ast_parse_continue_statement(ast, parent);
                break;
            case TK_SWITCH:
                ast_parse_switch_statement(ast, parent);
                continue;
            case TK_RETURN:
                ast_parse_return_statement(ast, parent);
                break;
            case TK_ECHO:
                ast_parse_echo_statement(ast, parent);
                break;
            case TK_ID:
                ast_parse_expression_statement(ast, parent);
                break;
            case TK_AUTO:
                ast_parse_variable_declaration(ast, parent);
                break;
            case TK_FUNCTION:
                if (parent->parent) {
                    ast_error(ast, "[Error] [Line:%d] functions can only be defined in top-level scope\n", ast->cur_line);
                    return;
                }
                ast_parse_function_declaration(ast, parent);
                continue;
            default:
                return;
        }

        if (ast->cur_token != ';') {
            // 语句应当以分号结尾。以花括号结尾的语句可以省略分号。
            ast_error(ast, "[Error] [Line:%d] ';' expected near '%.*s'\n", ast->cur_line, ast->cur_length, ast->cur_start);
            return;
        }
        ast_step(ast);
    }
}

void ast_parse_variable_declaration(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* variable_declaration;

    // ast->cur_token == TK_AUTO
    ast_step(ast);

    while (1) {
        variable_declaration = ast_node_new(ast, 2);
        variable_declaration->type = VARIABLE_DECLARATION;
        ast_node_append_child(parent, variable_declaration);

        if (ast->cur_token != TK_ID) {
            ast_error(ast, "[Error] [Line:%d] `%.*s` is not a <identifier>\n", ast->cur_line, ast->cur_length, ast->cur_start);
            return;
        } else {
            //printf("[Info] [Line:%d] variable `%.*s` declared\n", ast->cur_line, ast->cur_length, ast->cur_start);
            ast_parse_id_token(ast, variable_declaration);
        }
        
        if (ast->cur_token == '=') {
            variable_declaration->u.op = ast->cur_token;
            ast_step(ast);
            
            //printf("[Info] [Line:%d] variable initialized\n", ast->cur_line);
            ast_parse_expression(ast, variable_declaration);
        }

        // 创建变量加入作用域
        lgx_str_t s;
        s.buffer = ((lgx_ast_node_token_t *)(variable_declaration->child[0]))->tk_start;
        s.length = ((lgx_ast_node_token_t *)(variable_declaration->child[0]))->tk_length;

        if (parent->parent && lgx_scope_local_val_get(variable_declaration, &s) == NULL) {
            lgx_scope_val_add(variable_declaration, &s);
        } else if (parent->parent == NULL && lgx_scope_global_val_get(variable_declaration, &s) == NULL) {
            lgx_scope_val_add(variable_declaration, &s);
        } else {
            ast_error(ast, "[Error] [Line:%d] identifier `%.*s` has already been declared\n", ast->cur_line, s.length, s.buffer);
            return;
        }

        switch (ast->cur_token) {
            case ',':
                ast_step(ast);
                break;
            default:
                return;
        }
    }
}

void ast_parse_function_declaration(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* function_declaration = ast_node_new(ast, 3);
    function_declaration->type = FUNCTION_DECLARATION;
    ast_node_append_child(parent, function_declaration);

    // ast->cur_token == TK_FUNCTION
    ast_step(ast);

    if (ast->cur_token != TK_ID) {
        ast_error(ast, "[Error] [Line:%d] `<identifier>` expected\n", ast->cur_line);
        return;
    }
    ast_parse_id_token(ast, function_declaration);

    // 创建函数加入作用域
    lgx_str_t s;
    lgx_ast_node_token_t *n = (lgx_ast_node_token_t *)function_declaration->child[0];
    s.buffer = n->tk_start;
    s.length = n->tk_length;

    if (parent->parent == NULL && lgx_scope_global_val_get(function_declaration, &s) == NULL) {
        lgx_scope_val_add(function_declaration, &s);
    } else {
        ast_error(ast, "[Error] [Line:%d] identifier `%.*s` has already been declared\n", ast->cur_line, n->tk_length, n->tk_start);
        return;
    }

    if (ast->cur_token != '(') {
        ast_error(ast, "[Error] [Line:%d] '(' expected\n", ast->cur_line);
        return;
    }
    ast_step(ast);

    ast_parse_decl_parameter(ast, function_declaration);

    if (ast->cur_token != ')') {
        ast_error(ast, "[Error] [Line:%d] ')' expected\n", ast->cur_line);
        return;
    }
    ast_step(ast);
    
    ast_parse_block_statement_with_braces(ast, function_declaration);
}

int lgx_ast_parser(lgx_ast_t* ast) {
    ast->root = ast_node_new(ast, 128);
    ast->root->type = BLOCK_STATEMENT;

    // 全局作用域
    ast->root->u.symbols = lgx_hash_new(32);

    ast_step(ast);

    ast_parse_statement(ast, ast->root);

    switch (ast->cur_token) {
        case TK_EOF:
            break;
        default:
            ast_error(ast, "[Error] [Line:%d] unexpected `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
    }

    return 0;
}

void lgx_ast_print(lgx_ast_node_t* node, int indent) {
    int i;
    switch(node->type) {
        // Statement
        case BLOCK_STATEMENT:
            printf("%*s%s\n", indent, "", "{");
            for(i = 0; i < node->children; i++) {
                lgx_ast_print(node->child[i], indent+2);
            }
            printf("%*s%s\n", indent, "", "}");
            break;
        case IF_STATEMENT:
            printf("%*s%s\n", indent, "", "IF_STATEMENT");
            lgx_ast_print(node->child[0], indent+2);
            lgx_ast_print(node->child[1], indent+2);
            break;
        case IF_ELSE_STATEMENT:
            printf("%*s%s\n", indent, "", "IF_ELSE_STATEMENT");
            lgx_ast_print(node->child[0], indent+2);
            lgx_ast_print(node->child[1], indent+2);
            lgx_ast_print(node->child[2], indent+2);
            break;
        case FOR_STATEMENT:
            printf("%*s%s\n", indent, "", "FOR_STATEMENT");
            break;
        case WHILE_STATEMENT:
            printf("%*s%s\n", indent, "", "WHILE_STATEMENT");
            lgx_ast_print(node->child[0], indent+2);
            lgx_ast_print(node->child[1], indent+2);
            break;
        case DO_WHILE_STATEMENT:
            printf("%*s%s\n", indent, "", "DO_WHILE_STATEMENT");
            lgx_ast_print(node->child[0], indent+2);
            lgx_ast_print(node->child[1], indent+2);
            break;
        case CONTINUE_STATEMENT:
            printf("%*s%s\n", indent, "", "CONTINUE_STATEMENT");
            break;
        case BREAK_STATEMENT:
            printf("%*s%s\n", indent, "", "BREAK_STATEMENT");
            break;
        case SWITCH_STATEMENT:
            printf("%*s%s\n", indent, "", "SWITCH_STATEMENT");
            break;
        case RETURN_STATEMENT:
            printf("%*s%s\n", indent, "", "RETURN_STATEMENT");
            if (node->child[0])
                lgx_ast_print(node->child[0], indent+2);
            break;
        case ECHO_STATEMENT:
            printf("%*s%s\n", indent, "", "ECHO_STATEMENT");
            if (node->child[0])
                lgx_ast_print(node->child[0], indent+2);
            break;
        case EXPRESSION_STATEMENT:
            printf("%*s%s\n", indent, "", "EXPRESSION_STATEMENT");
            lgx_ast_print(node->child[0], indent+2);
            break;
        // Declaration
        case FUNCTION_DECLARATION:
            printf("%*s%s\n", indent, "", "FUNCTION_DECLARATION");
            lgx_ast_print(node->child[0], indent+2);
            lgx_ast_print(node->child[1], indent+2);
            lgx_ast_print(node->child[2], indent+2);
            break;
        case VARIABLE_DECLARATION:
            printf("%*s%s\n", indent, "", "VARIABLE_DECLARATION");
            lgx_ast_print(node->child[0], indent+2);
            if (node->child[1]) {
                printf("%*s%s\n", indent+2, "", "=");
                lgx_ast_print(node->child[1], indent+2);
            }
            break;
        // Expression
        case CONDITIONAL_EXPRESSION:
            printf("%*s%s\n", indent, "", "CONDITIONAL_EXPRESSION");
            break;
        case BINARY_EXPRESSION:
            printf("%*s%s\n", indent, "", "(");
            lgx_ast_print(node->child[0], indent+2);
            printf("%*s%d\n", indent, "", node->u.op);
            if (node->child[1]) {
                lgx_ast_print(node->child[1], indent+2);
            }
            printf("%*s%s\n", indent, "", ")");
            break;
        case UNARY_EXPRESSION:
            printf("%*s%s\n", indent, "", "(");
            printf("%*s%c\n", indent, "", node->u.op);
            lgx_ast_print(node->child[0], indent+2);
            printf("%*s%s\n", indent, "", ")");
            break;
        // Other
        case IDENTIFIER_TOKEN:
            printf("%*s%s\n", indent, "", "IDENTIFIER_TOKEN");
            break;
        case LONG_TOKEN:
            printf("%*s%s\n", indent, "", "LONG_TOKEN");
            break;
        case DOUBLE_TOKEN:
            printf("%*s%s\n", indent, "", "DOUBLE_TOKEN");
            break;
        case STRING_TOKEN:
            printf("%*s%s\n", indent, "", "STRING_TOKEN");
            break;
        case TRUE_TOKEN:
            printf("%*s%s\n", indent, "", "TRUE_TOKEN");
            break;
        case FALSE_TOKEN:
            printf("%*s%s\n", indent, "", "FALSE_TOKEN");
            break;
        case UNDEFINED_TOKEN:
            printf("%*s%s\n", indent, "", "UNDEFINED_TOKEN");
            break;
        case ARRAY_TOKEN:
            printf("%*s%s\n", indent, "", "[");
            for(i = 0; i < node->children; i++) {
                lgx_ast_print(node->child[i], indent+2);
            }
            printf("%*s%s\n", indent, "", "]");
            break;
        case FUNCTION_CALL_PARAMETER:
        case FUNCTION_DECL_PARAMETER:
            printf("%*s%s\n", indent, "", "(");
            for(i = 0; i < node->children; i++) {
                lgx_ast_print(node->child[i], indent+2);
            }
            printf("%*s%s\n", indent, "", ")");
            break;
        default:
            printf("%*s%s %d\n", indent, "", "ERROR!", node->type);
    }
}