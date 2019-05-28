#include "ast.h"
#include "symbol.h"

static void ast_node_del(lgx_ast_node_t* node) {
    switch (node->type) {
        case BLOCK_STATEMENT:
            if (node->u.symbols) {
                lgx_ht_cleanup(node->u.symbols);
                xfree(node->u.symbols);
            }
            break;
        case FUNCTION_DECLARATION:
            if (node->u.regs) {
                lgx_reg_cleanup(node->u.regs);
                xfree(node->u.regs);
            }
            break;
        case FOR_STATEMENT: case WHILE_STATEMENT: case DO_STATEMENT:
        case SWITCH_STATEMENT:
            if (node->u.jmps) {
                while (!lgx_list_empty(node->u.jmps)) {
                    lgx_ast_node_list_t* n = lgx_list_first_entry(node->u.jmps, lgx_ast_node_list_t, head);
                    lgx_list_del(&n->head);
                    xfree(n);
                }
                xfree(node->u.jmps);
            }
            break;
        default:
            break;
    }

    int i;
    for (i = 0; i < node->children; ++i) {
        ast_node_del(node->child[i]);
    }

    if (node->child) {
        xfree(node->child);
    }

    xfree(node);
}

static lgx_ast_node_t* ast_node_new(lgx_ast_t* ast, lgx_ast_type_t type) {
    lgx_ast_node_t* node = (lgx_ast_node_t*)xcalloc(1, sizeof(lgx_ast_node_t));
    if (!node) {
        return NULL;
    }

    node->offset = ast->lex.milestone;
    node->length = ast->lex.offset - ast->lex.milestone;
    node->line = ast->lex.line;
    node->row = ast->lex.milestone - ast->lex.line_start;

    node->type = type;
    switch (type) {
        case BLOCK_STATEMENT:
            node->u.symbols = (lgx_ht_t*)xmalloc(sizeof(lgx_ht_t));
            if (!node->u.symbols) {
                goto error;
            }
            if (lgx_ht_init(node->u.symbols, 8) != 0) {
                goto error;
            }
            break;
        case FUNCTION_DECLARATION:
            node->u.regs = (lgx_reg_t*)xmalloc(sizeof(lgx_reg_t));
            if (!node->u.regs) {
                goto error;
            }
            if (lgx_reg_init(node->u.regs) != 0) {
                goto error;
            }
            break;
        default:
            break;
    }

    return node;

error:
    if (node) {
        ast_node_del(node);
    }

    return NULL;
}

static int ast_node_append_child(lgx_ast_node_t* parent, lgx_ast_node_t* child) {
    child->parent = parent;

    // 如果空间不足，则扩展空间
    if (parent->size <= parent->children) {
        if (parent->size == 0) {
            parent->child = (lgx_ast_node_t**)xmalloc(sizeof(lgx_ast_node_t*));
            if (!parent->child) {
                return 1;
            }
            parent->size = 1;
        } else {
            lgx_ast_node_t** p = (lgx_ast_node_t**)xrealloc(parent->child, 2 * parent->size * sizeof(lgx_ast_node_t*));
            if (!p) {
                return 1;
            }
            parent->child = p;
            parent->size *= 2;
        }
    }

    parent->child[parent->children] = child;
    parent->children ++;
    
    return 0;
}

static int ast_node_remove_child(lgx_ast_node_t* parent, lgx_ast_node_t* child) {
    assert(parent->children);

    parent->children --;
    parent->child[parent->children] = NULL;

    return 0;
}

static lgx_ast_node_t* ast_node_last_child(lgx_ast_node_t* parent) {
    assert(parent->children);

    return parent->child[parent->children - 1];
}

// 解析下一个 token
static void ast_step(lgx_ast_t* ast) {
    lgx_token_t token;
    while (1) {
        token = lgx_lex_next(&ast->lex);
        switch (token) {
            case TK_SPACE:
            case TK_COMMENT:
                // 忽略空格与注释
                break;
            case TK_EOF:
            case TK_EOL:
                // TODO 完善自动插入分号的规则
                if (ast->cur_token == TK_RIGHT_BRACE) {
                    // 符合规则，自动转换为分号
                    token = TK_SEMICOLON;
                } else {
                    if (token == TK_EOL) {
                        // 不符合规则的换行被忽略
                        break;
                    }
                }
            default:
                ast->prev_token = ast->cur_token;

                ast->cur_token = token;
                ast->cur_start = ast->lex.source.content + ast->lex.milestone;
                ast->cur_length = ast->lex.offset - ast->lex.milestone;
                return;
        }
    }
}

// 追加一条错误信息
static void ast_error(lgx_ast_t* ast, const char *fmt, ...) {
    va_list   args;

    lgx_ast_error_list_t* err = xcalloc(1, sizeof(lgx_ast_error_list_t));
    if (!err) {
        return;
    }

    lgx_list_init(&err->head);
    err->err_no = 1;

    if (lgx_str_init(&err->err_msg, 256)) {
        xfree(err);
        return;
    }

    if (ast->lex.source.path) {
        err->err_msg.length = snprintf(err->err_msg.buffer, err->err_msg.size,
            "[PARSER ERROR] [%s:%d:%d] ", ast->lex.source.path, ast->lex.line + 1, ast->lex.milestone - ast->lex.line_start);
    } else {
        err->err_msg.length = snprintf(err->err_msg.buffer, err->err_msg.size,
            "[PARSER ERROR] ");
    }

    va_start(args, fmt);
    err->err_msg.length += vsnprintf(err->err_msg.buffer + err->err_msg.length,
        256 - err->err_msg.length, fmt, args);
    va_end(args);
    
    lgx_list_add_tail(&err->head, &ast->errors);
}

static int ast_parse_identifier_token(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* identifier_token = ast_node_new(ast, IDENTIFIER_TOKEN);
    ast_node_append_child(parent, identifier_token);

    if (ast->cur_token != TK_IDENTIFIER) {
        ast_error(ast, "`<identifier>` expected before '%.*s'\n", ast->cur_length, ast->cur_start);
        return 1;
    }

    // ast->cur_token == TK_IDENTIFIER
    ast_step(ast);

    return 0;
}

static int ast_parse_string_token(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* string_token = ast_node_new(ast, STRING_TOKEN);
    ast_node_append_child(parent, string_token);

    if (ast->cur_token != TK_LITERAL_STRING) {
        ast_error(ast, "`<string>` expected before '%.*s'\n", ast->cur_length, ast->cur_start);
        return 1;
    }

    // ast->cur_token == TK_LITERAL_STRING
    ast_step(ast);

    return 0;
}

static int ast_parse_bsc_expression(lgx_ast_t* ast, lgx_ast_node_t* parent);
static int ast_parse_pri_expression(lgx_ast_t* ast, lgx_ast_node_t* parent);
static int ast_parse_suf_expression(lgx_ast_t* ast, lgx_ast_node_t* parent);
static int ast_parse_sub_expression(lgx_ast_t* ast, lgx_ast_node_t* parent, int precedence);
static int ast_parse_expression_with_parentheses(lgx_ast_t* ast, lgx_ast_node_t* parent);
static int ast_parse_expression(lgx_ast_t* ast, lgx_ast_node_t* parent);
static int ast_parse_type_expression(lgx_ast_t* ast, lgx_ast_node_t* parent);

static int ast_parse_decl_parameter(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* param_list = ast_node_new(ast, FUNCTION_DECL_PARAMETER);
    ast_node_append_child(parent, param_list);

    if (ast->cur_token == TK_RIGHT_PAREN) {
        // 参数为空
        return 0;
    }

    while (1) {
        lgx_ast_node_t* variable_declaration = ast_node_new(ast, VARIABLE_DECLARATION);
        ast_node_append_child(param_list, variable_declaration);

        if (ast_parse_identifier_token(ast, variable_declaration)) {
            return 1;
        }

        if (ast_parse_type_expression(ast, variable_declaration)) {
            return 1;
        }

        if (ast->cur_token == TK_ASSIGN) {
            ast_step(ast);

            if (ast_parse_expression(ast, variable_declaration)) {
                return 1;
            }
        }

        if (ast->cur_token != TK_COMMA) {
            return 0;
        }
        ast_step(ast);
    }
}

static int ast_parse_decl_parameter_with_parentheses(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    if (ast->cur_token != TK_LEFT_PAREN) {
        ast_error(ast, "'(' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    if (ast_parse_decl_parameter(ast, parent)) {
        return 1;
    }

    if (ast->cur_token != TK_RIGHT_PAREN) {
        ast_error(ast, "')' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    return 0;
}

static int ast_parse_call_parameter(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* param_list = ast_node_new(ast, FUNCTION_CALL_PARAMETER);
    ast_node_append_child(parent, param_list);

    if (ast->cur_token == TK_RIGHT_PAREN) {
        // 参数为空
        return 0;
    }

    while (1) {
        if (ast_parse_expression(ast, param_list)) {
            return 1;
        }

        if (ast->cur_token != TK_COMMA) {
            return 0;
        }
        ast_step(ast);
    }
}

static int ast_parse_block_statement_with_braces(lgx_ast_t* ast, lgx_ast_node_t* parent);

static int ast_parse_type_expression(lgx_ast_t* ast, lgx_ast_node_t* parent) {
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
            if (ast_parse_identifier_token(ast, type_expression)) {
                return 1;
            }
            break;
        case TK_LEFT_BRACK:
            ast_step(ast);

            if (ast->cur_token == TK_RIGHT_BRACK) {
                type_expression->u.type = T_ARRAY;
                ast_step(ast);

                if (ast_parse_type_expression(ast, type_expression)) {
                    return 1;
                }
            } else {
                type_expression->u.type = T_MAP;

                // 解析 key
                if (ast_parse_type_expression(ast, type_expression)) {
                    return 1;
                }

                if (ast->cur_token != TK_RIGHT_BRACK) {
                    ast_error(ast, "`]` expected before '%.*s'\n", ast->cur_length, ast->cur_start);
                    return 1;
                }
                ast_step(ast);

                // 解析 value
                if (ast_parse_type_expression(ast, type_expression)) {
                    return 1;
                }
            }
            break;
        case TK_STRUCT:
            type_expression->u.type = T_STRUCT;
            ast_step(ast);

            if (ast_parse_block_statement_with_braces(ast, type_expression)) {
                return 1;
            }
            break;
        case TK_INTERFACE:
            type_expression->u.type = T_INTERFACE;
            ast_step(ast);

            if (ast_parse_block_statement_with_braces(ast, type_expression)) {
                return 1;
            }
            break;
        case TK_FUNCTION:
            type_expression->u.type = T_FUNCTION;
            ast_step(ast);

            if (ast_parse_decl_parameter_with_parentheses(ast, type_expression)) {
                return 1;
            }

            if (ast_parse_type_expression(ast, type_expression)) {
                return 1;
            }
            break;
        default:
            break;
    }

    return 0;
}

static int ast_parse_array_expression(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* array_expression = ast_node_new(ast, ARRAY_EXPRESSION);
    ast_node_append_child(parent, array_expression);

    if (ast->cur_token != TK_LEFT_BRACK) {
        ast_error(ast, "`[` expected before '%.*s'\n", ast->cur_length, ast->cur_start);
        return 1;
    }

    // ast->cur_token == '['
    ast_step(ast);

    while (1) {
        if (ast->cur_token == TK_RIGHT_BRACK) {
            break;
        }

        if (ast_parse_expression(ast, array_expression)) {
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
    ast_step(ast);

    return 0;
}

static int ast_parse_struct_expression(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* struct_expression = ast_node_new(ast, STRUCT_EXPRESSION);
    ast_node_append_child(parent, struct_expression);

    if (ast->cur_token != TK_LEFT_BRACE) {
        ast_error(ast, "`{` expected before '%.*s'\n", ast->cur_length, ast->cur_start);
        return 1;
    }

    // ast->cur_token == '{'
    ast_step(ast);

    while (1) {
        if (ast->cur_token == TK_RIGHT_BRACE) {
            break;
        }

        if (ast_parse_expression(ast, struct_expression)) {
            return 1;
        }

        if (ast->cur_token != TK_COLON) {
            ast_error(ast, "':' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
            return 1;
        }
        ast_step(ast);

        if (ast_parse_expression(ast, struct_expression)) {
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
static int ast_parse_pri_expression(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    switch (ast->cur_token) {
        case TK_LEFT_PAREN:
            return ast_parse_expression_with_parentheses(ast, parent);
        case TK_IDENTIFIER:
            return ast_parse_identifier_token(ast, parent);
        default:
            ast_error(ast, "`<expression>` expected before `%.*s`\n", ast->cur_length, ast->cur_start);
            return 1;
    }
}

// suf_expr -> pri_expr { '->' ID | '.' ID | '[' sub_expr ']' | funcargs }
static int ast_parse_suf_expression(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    if (ast_parse_pri_expression(ast, parent)) {
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

                if (ast_parse_identifier_token(ast, binary_expression)) {
                    return 1;
                }
                break;
            case TK_LEFT_BRACK:
                // 数组访问操作符
                binary_expression->u.op = TK_LEFT_BRACK;
                ast_step(ast);

                if (ast->cur_token != TK_RIGHT_BRACK) {
                    if (ast_parse_expression(ast, binary_expression)) {
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

                if (ast_parse_call_parameter(ast, binary_expression)) {
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
static int ast_parse_bsc_expression(lgx_ast_t* ast, lgx_ast_node_t* parent) {
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
            return ast_parse_array_expression(ast, parent);
        case TK_LEFT_BRACE:
            // 结构体字面量
            return ast_parse_struct_expression(ast, parent);
        default:
            return ast_parse_suf_expression(ast, parent);
    }

    return 0;
}

static int ast_operator_precedence(int token) {
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
static int ast_parse_sub_expression(lgx_ast_t* ast, lgx_ast_node_t* parent, int precedence) {
    switch (ast->cur_token) {
        case TK_LOGIC_NOT: // 逻辑非运算符
        case TK_NOT: // 按位取反运算符
        case TK_SUB: // 负号运算符
        case TK_INC: // 自增运算符
        case TK_DEC: // 自减运算符
        case TK_CO: { // co 运算符
            // 单目运算符
            lgx_ast_node_t* unary_expression = ast_node_new(ast, UNARY_EXPRESSION);
            unary_expression->u.op = ast->cur_token;
            ast_node_append_child(parent, unary_expression);

            ast_step(ast);

            if (ast_parse_sub_expression(ast, unary_expression, 2)) {
                return 1;
            }
            break;
        }
        default:
            if (ast_parse_bsc_expression(ast, parent)) {
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

        if (ast_parse_sub_expression(ast, binary_expression, p)) {
            return 1;
        }

        p = ast_operator_precedence(ast->cur_token);
    }
}

// 解析带括号的表达式
static int ast_parse_expression_with_parentheses(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    if (ast->cur_token != TK_LEFT_PAREN) {
        ast_error(ast, "'(' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    if (ast_parse_sub_expression(ast, parent, 15)) {
        return 1;
    }

    if (ast->cur_token != TK_RIGHT_PAREN) {
        ast_error(ast, "')' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    return 0;
}

static int ast_parse_expression(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    return ast_parse_sub_expression(ast, parent, 15);
}

static int ast_parse_statement(lgx_ast_t* ast, lgx_ast_node_t* parent);

static int ast_parse_block_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* block_statement = ast_node_new(ast, BLOCK_STATEMENT);
    ast_node_append_child(parent, block_statement);
    
    return ast_parse_statement(ast, block_statement);
}

static int ast_parse_block_statement_with_braces(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    if (ast->cur_token != TK_LEFT_BRACE) {
        ast_error(ast, "'{' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    if (ast_parse_block_statement(ast, parent)) {
        return 1;
    }

    if (ast->cur_token != TK_RIGHT_BRACE) {
        ast_error(ast, "'}' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    return 0;
}

static int ast_parse_if_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* if_statement = ast_node_new(ast, IF_STATEMENT);
    ast_node_append_child(parent, if_statement);

    assert(ast->cur_token == TK_IF);
    ast_step(ast);
    
    if (ast_parse_expression_with_parentheses(ast, if_statement)) {
        return 1;
    }
    
    if (ast_parse_block_statement_with_braces(ast, if_statement)) {
        return 1;
    }
    
    if (ast->cur_token != TK_ELSE) {
        return 0;
    }
    ast_step(ast);
    
    if_statement->type = IF_ELSE_STATEMENT;

    if (ast->cur_token == TK_IF) {
        return ast_parse_if_statement(ast, if_statement);
    } else {
        return ast_parse_block_statement_with_braces(ast, if_statement);
    }
}

static int ast_parse_for_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
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
        if (ast_parse_expression(ast, for_condition)) {
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
        if (ast_parse_expression(ast, for_condition)) {
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
        if (ast_parse_expression(ast, for_condition)) {
            return 1;
        }
    }

    if (ast->cur_token != TK_RIGHT_PAREN) {
        ast_error(ast, "')' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    return ast_parse_block_statement_with_braces(ast, for_statement);
}

static int ast_parse_while_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* while_statement = ast_node_new(ast, WHILE_STATEMENT);
    ast_node_append_child(parent, while_statement);

    assert(ast->cur_token == TK_WHILE);
    ast_step(ast);
    
    if (ast_parse_expression_with_parentheses(ast, while_statement)) {
        return 1;
    }
    
    return ast_parse_block_statement_with_braces(ast, while_statement);
}

static int ast_parse_do_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* do_statement = ast_node_new(ast, DO_STATEMENT);
    ast_node_append_child(parent, do_statement);

    assert(ast->cur_token == TK_DO);
    ast_step(ast);
    
    if (ast_parse_block_statement_with_braces(ast, do_statement)) {
        return 1;
    }

    if (ast->cur_token != TK_WHILE) {
        ast_error(ast, "'while' expected, not `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);
    
    return ast_parse_expression_with_parentheses(ast, do_statement);
}

static int ast_parse_break_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* break_statement = ast_node_new(ast, BREAK_STATEMENT);
    ast_node_append_child(parent, break_statement);

    assert(ast->cur_token == TK_BREAK);
    ast_step(ast);

    return 0;
}

static int ast_parse_continue_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* continue_statement = ast_node_new(ast, CONTINUE_STATEMENT);
    ast_node_append_child(parent, continue_statement);

    assert(ast->cur_token == TK_CONTINUE);
    ast_step(ast);

    return 0;
}

static int ast_parse_case_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* case_statement = ast_node_new(ast, CASE_STATEMENT);
    ast_node_append_child(parent, case_statement);

    assert(ast->cur_token == TK_CASE);
    ast_step(ast);

    if (ast_parse_expression(ast, case_statement)) {
        return 0;
    }

    if (ast->cur_token != TK_COLON) {
        ast_error(ast, "':' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    if (ast->cur_token == TK_LEFT_BRACE) {
        return ast_parse_block_statement_with_braces(ast, case_statement);
    } else {
        return ast_parse_block_statement(ast, case_statement);
    }
}

static int ast_parse_default_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* default_statement = ast_node_new(ast, DEFAULT_STATEMENT);
    ast_node_append_child(parent, default_statement);

    assert(ast->cur_token == TK_DEFAULT);
    ast_step(ast);

    if (ast->cur_token != TK_SEMICOLON) {
        ast_error(ast, "':' expected before `%.*s`\n", ast->cur_length, ast->cur_start);
        return 1;
    }
    ast_step(ast);

    if (ast->cur_token == TK_LEFT_BRACE) {
        return ast_parse_block_statement_with_braces(ast, default_statement);
    } else {
        return ast_parse_block_statement(ast, default_statement);
    }
}

static int ast_parse_switch_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* switch_statement = ast_node_new(ast, SWITCH_STATEMENT);
    ast_node_append_child(parent, switch_statement);

    assert(ast->cur_token == TK_SWITCH);
    ast_step(ast);

    if (ast_parse_expression_with_parentheses(ast, switch_statement)) {
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
            if (ast_parse_case_statement(ast, switch_statement)) {
                return 1;
            }
        } else if (ast->cur_token == TK_DEFAULT) {
            if (ast_parse_default_statement(ast, switch_statement)) {
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

static int ast_parse_try_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* try_statement = ast_node_new(ast, TRY_STATEMENT);
    ast_node_append_child(parent, try_statement);

    assert(ast->cur_token == TK_TRY);
    ast_step(ast);

    if (ast_parse_block_statement_with_braces(ast, try_statement)) {
        return 1;
    }

    while (ast->cur_token == TK_CATCH) {
        ast_step(ast);

        lgx_ast_node_t* catch_statement = ast_node_new(ast, CATCH_STATEMENT);
        ast_node_append_child(try_statement, catch_statement);

        if (ast_parse_decl_parameter_with_parentheses(ast, catch_statement)) {
            return 1;
        }

        if (ast_parse_block_statement_with_braces(ast, catch_statement)) {
            return 1;
        }
    }

    return 0;
}

static int ast_parse_throw_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* throw_statement = ast_node_new(ast, THROW_STATEMENT);
    ast_node_append_child(parent, throw_statement);

    assert(ast->cur_token == TK_THROW);
    ast_step(ast);

    return ast_parse_expression(ast, throw_statement);
}

static int ast_parse_return_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* return_statement = ast_node_new(ast, RETURN_STATEMENT);
    ast_node_append_child(parent, return_statement);

    assert(ast->cur_token == TK_RETURN);
    ast_step(ast);

    return ast_parse_expression(ast, return_statement);
}

static int ast_parse_expression_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* expression_statement = ast_node_new(ast, EXPRESSION_STATEMENT);
    ast_node_append_child(parent, expression_statement);
    
    return ast_parse_expression(ast, expression_statement);
}

static int ast_parse_package_declaration(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* package_declaration = ast_node_new(ast, PACKAGE_DECLARATION);
    ast_node_append_child(parent, package_declaration);

    assert(ast->cur_token == TK_PACKAGE);
    ast_step(ast);

    return ast_parse_identifier_token(ast, package_declaration);
}

static int ast_parse_import_declaration(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* import_declaration = ast_node_new(ast, IMPORT_DECLARATION);
    ast_node_append_child(parent, import_declaration);

    assert(ast->cur_token == TK_IMPORT);
    ast_step(ast);

    if (ast->cur_token == TK_IDENTIFIER) {
        if (ast_parse_identifier_token(ast, import_declaration)) {
            return 1;
        }
    }

    return ast_parse_string_token(ast, import_declaration);
}

static int ast_parse_export_declaration(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* export_declaration = ast_node_new(ast, EXPORT_DECLARATION);
    ast_node_append_child(parent, export_declaration);

    assert(ast->cur_token == TK_EXPORT);
    ast_step(ast);

    // TODO 指定被导出的方法

    return 0;
}

static int ast_parse_variable_declaration(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* variable_declaration = ast_node_new(ast, VARIABLE_DECLARATION);
    ast_node_append_child(parent, variable_declaration);

    assert(ast->cur_token == TK_VAR);
    ast_step(ast);

    while (1) {
        if (!variable_declaration) {
            variable_declaration = ast_node_new(ast, VARIABLE_DECLARATION);
            ast_node_append_child(parent, variable_declaration);
        }

        if (ast_parse_identifier_token(ast, variable_declaration)) {
            return 1;
        }

        if (ast_parse_type_expression(ast, variable_declaration)) {
            return 1;
        }

        if (ast->cur_token == TK_ASSIGN) {
            ast_step(ast);

            if (ast_parse_expression(ast, variable_declaration)) {
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

static int ast_parse_constant_declaration(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* constant_declaration = ast_node_new(ast, CONSTANT_DECLARATION);
    ast_node_append_child(parent, constant_declaration);

    assert(ast->cur_token == TK_CONST);
    ast_step(ast);

    while (1) {
        if (!constant_declaration) {
            constant_declaration = ast_node_new(ast, CONSTANT_DECLARATION);
            ast_node_append_child(parent, constant_declaration);
        }

        if (ast_parse_identifier_token(ast, constant_declaration)) {
            return 1;
        }

        if (ast_parse_type_expression(ast, constant_declaration)) {
            return 1;
        }

        if (ast->cur_token != TK_ASSIGN) {
            ast_error(ast, "`=` expected, constant must be initialized\n");
            return 1;
        }
        ast_step(ast);

        if (ast_parse_expression(ast, constant_declaration)) {
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

static int ast_parse_function_receiver(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* function_receiver = ast_node_new(ast, FUNCTION_RECEIVER);
    ast_node_append_child(parent, function_receiver);\

    if (ast->cur_token != TK_LEFT_PAREN) {
        return 0;
    }

    return ast_parse_decl_parameter_with_parentheses(ast, function_receiver);
}

static int ast_parse_function_declaration(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* function_declaration = ast_node_new(ast, FUNCTION_DECLARATION);
    ast_node_append_child(parent, function_declaration);

    assert(ast->cur_token == TK_FUNCTION);
    ast_step(ast);

    // 接收者
    if (ast_parse_function_receiver(ast, function_declaration)) {
        return 1;
    }

    // 函数名
    if (ast_parse_identifier_token(ast, function_declaration)) {
        return 1;
    }

    // 参数
    if (ast_parse_decl_parameter_with_parentheses(ast, function_declaration)) {
        return 1;
    }

    // 返回值
    if (ast_parse_type_expression(ast, function_declaration)) {
        return 1;
    }

    // 函数体
    if (ast_parse_block_statement_with_braces(ast, function_declaration)) {
        return 1;
    }

    return 0;
}

static int ast_parse_type_declaration(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* type_declaration = ast_node_new(ast, TYPE_DECLARATION);
    ast_node_append_child(parent, type_declaration);

    assert(ast->cur_token == TK_TYPE);
    ast_step(ast);

    if (ast_parse_identifier_token(ast, type_declaration)) {
        return 1;
    }

    if (ast_parse_type_expression(ast, type_declaration)) {
        return 1;
    }

    return 0;
}

static int ast_parse_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    while(1) {
        switch (ast->cur_token) {
            case TK_RIGHT_BRACE:
                return 0;
            case TK_IF:
                if (ast_parse_if_statement(ast, parent)) {
                    return 1;
                }
                break;
            case TK_FOR:
                if (ast_parse_for_statement(ast, parent)) {
                    return 1;
                }
                break;
            case TK_WHILE:
                if (ast_parse_while_statement(ast, parent)) {
                    return 1;
                }
                break;
            case TK_DO:
                if (ast_parse_do_statement(ast, parent)) {
                    return 1;
                }
                break;
            case TK_BREAK:
                if (ast_parse_break_statement(ast, parent)) {
                    return 1;
                }
                break;
            case TK_CONTINUE:
                if (ast_parse_continue_statement(ast, parent)) {
                    return 1;
                }
                break;
            case TK_SWITCH:
                if (ast_parse_switch_statement(ast, parent)) {
                    return 1;
                }
                break;
            case TK_RETURN:
                if (ast_parse_return_statement(ast, parent)) {
                    return 1;
                }
                break;
            case TK_TRY:
                if (ast_parse_try_statement(ast, parent)) {
                    return 1;
                }
                break;
            case TK_THROW:
                if (ast_parse_throw_statement(ast, parent)) {
                    return 1;
                }
                break;
            case TK_VAR:
                if (ast_parse_variable_declaration(ast, parent)) {
                    return 1;
                }
                break;
            case TK_CONST:
                if (ast_parse_constant_declaration(ast, parent)) {
                    return 1;
                }
                break;
            case TK_TYPE:
                if (ast_parse_type_declaration(ast, parent)) {
                    return 1;
                }
                break;
            default:
                if (ast_parse_expression_statement(ast, parent)) {
                    return 1;
                }
        }

        // 语句应当以分号结尾
        if (ast->cur_token != TK_SEMICOLON) {
            if (ast->prev_token != TK_SEMICOLON) {
                ast_error(ast, "`;` expected before '%.*s'\n", ast->cur_length, ast->cur_start);
            } else {
                ast_error(ast, "unexpected token '%.*s'\n", ast->cur_length, ast->cur_start);
            }
            return 1;
        }
        ast_step(ast);
    }
}

static int ast_parse(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    while (1) {
        // 解析语句
        switch (ast->cur_token) {
            case TK_EOF:
                // 读取到文件末尾，解析结束
                return 0;
            case TK_PACKAGE:
                if (ast_parse_package_declaration(ast, parent)) {
                    return 1;
                }
                break;
            case TK_IMPORT:
                if (ast_parse_import_declaration(ast, parent)) {
                    return 1;
                }
                break;
            case TK_EXPORT:
                if (ast_parse_export_declaration(ast, parent)) {
                    return 1;
                }
                break;
            case TK_VAR:
                if (ast_parse_variable_declaration(ast, parent)) {
                    return 1;
                }
                break;
            case TK_CONST:
                if (ast_parse_constant_declaration(ast, parent)) {
                    return 1;
                }
                break;
            case TK_FUNCTION:
                if (ast_parse_function_declaration(ast, parent)) {
                    return 1;
                }
                break;
            case TK_TYPE:
                if (ast_parse_type_declaration(ast, parent)) {
                    return 1;
                }
                break;
            default:
                break;
        }
        
        // 语句应当以分号结尾
        if (ast->cur_token != TK_SEMICOLON) {
            if (ast->prev_token != TK_SEMICOLON) {
                ast_error(ast, "`;` expected before '%.*s'\n", ast->cur_length, ast->cur_start);
            } else {
                ast_error(ast, "unexpected token '%.*s'\n", ast->cur_length, ast->cur_start);
            }
            return 1;
        }
        ast_step(ast);
    }
}

int lgx_ast_init(lgx_ast_t* ast, char* file) {
    // 初始化
    memset(ast, 0, sizeof(lgx_ast_t));
    lgx_list_init(&ast->errors);

    if (lgx_lex_init(&ast->lex, file) != 0) {
        ast_error(ast, "can't open input file: %s\n", file);
        return 1;
    }

    // 初始化根节点
    ast->root = ast_node_new(ast, BLOCK_STATEMENT);
    if (!ast->root) {
        return 2;
    }

    // 读取一个 token
    ast_step(ast);

    if (ast_parse(ast, ast->root)) {
        return 3;
    }

    return lgx_symbol_init(ast);
}

int lgx_ast_cleanup(lgx_ast_t* ast) {
    lgx_lex_cleanup(&ast->lex);

    lgx_symbol_cleanup(ast);

    if (ast->root) {
        ast_node_del(ast->root);
        ast->root = NULL;
    }

    // 释放错误信息链表
    while (!lgx_list_empty(&ast->errors)) {
        lgx_ast_error_list_t *n = lgx_list_first_entry(&ast->errors, lgx_ast_error_list_t, head);
        lgx_list_del(&n->head);
        lgx_str_cleanup(&n->err_msg);
        xfree(n);
    }

    return 0;
}

static void ast_print(lgx_ast_node_t* node, int indent);

static void ast_print_child(lgx_ast_node_t* node, int indent) {
    if (!node->children) {
        return;
    }

    int i;
    for (i = 0; i < node->children; ++i) {
        ast_print(node->child[i], indent + 2);
    }
}

static void ast_print(lgx_ast_node_t* node, int indent) {
    if (!node) {
        return;
    }

    switch(node->type) {
        // Statement
        case BLOCK_STATEMENT:
            printf("%*s%s\n", indent, "", "{");
            ast_print_child(node, indent);
            printf("%*s%s\n", indent, "", "}");
            break;
        case IF_STATEMENT:
            printf("%*s%s\n", indent, "", "IF_STATEMENT");
            ast_print_child(node, indent);
            break;
        case IF_ELSE_STATEMENT:
            printf("%*s%s\n", indent, "", "IF_ELSE_STATEMENT");
            ast_print_child(node, indent);
            break;
        case FOR_STATEMENT:
            printf("%*s%s\n", indent, "", "FOR_STATEMENT");
            ast_print_child(node, indent);
            break;
        case WHILE_STATEMENT:
            printf("%*s%s\n", indent, "", "WHILE_STATEMENT");
            ast_print_child(node, indent);
            break;
        case DO_STATEMENT:
            printf("%*s%s\n", indent, "", "DO_WHILE_STATEMENT");
            ast_print_child(node, indent);
            break;
        case CONTINUE_STATEMENT:
            printf("%*s%s\n", indent, "", "CONTINUE_STATEMENT");
            break;
        case BREAK_STATEMENT:
            printf("%*s%s\n", indent, "", "BREAK_STATEMENT");
            break;
        case SWITCH_STATEMENT:
            printf("%*s%s\n", indent, "", "SWITCH_STATEMENT");
            ast_print_child(node, indent);
            break;
        case CASE_STATEMENT:
            printf("%*s%s\n", indent, "", "CASE_STATEMENT");
            ast_print_child(node, indent);
            break;
        case DEFAULT_STATEMENT:
            printf("%*s%s\n", indent, "", "DEFAULT_STATEMENT");
            ast_print_child(node, indent);
            break;
        case RETURN_STATEMENT:
            printf("%*s%s\n", indent, "", "RETURN_STATEMENT");
            ast_print_child(node, indent);
            break;
        case EXPRESSION_STATEMENT:
            printf("%*s%s\n", indent, "", "EXPRESSION_STATEMENT");
            ast_print_child(node, indent);
            break;
        case TRY_STATEMENT:
            printf("%*s%s\n", indent, "", "TRY_STATEMENT");
            ast_print_child(node, indent);
            break;
        case CATCH_STATEMENT:
            printf("%*s%s\n", indent, "", "CATCH_STATEMENT");
            ast_print_child(node, indent);
            break;
        case THROW_STATEMENT:
            printf("%*s%s\n", indent, "", "THROW_STATEMENT");
            ast_print_child(node, indent);
            break;
        // Declaration
        case PACKAGE_DECLARATION:
            printf("%*s%s\n", indent, "", "PACKAGE_DECLARATION");
            ast_print_child(node, indent);
            break;
        case IMPORT_DECLARATION:
            printf("%*s%s\n", indent, "", "IMPORT_DECLARATION");
            ast_print_child(node, indent);
            break;
        case EXPORT_DECLARATION:
            printf("%*s%s\n", indent, "", "EXPORT_DECLARATION");
            ast_print_child(node, indent);
            break;
        case VARIABLE_DECLARATION:
            printf("%*s%s\n", indent, "", "VARIABLE_DECLARATION");
            ast_print_child(node, indent);
            break;
        case CONSTANT_DECLARATION:
            printf("%*s%s\n", indent, "", "CONSTANT_DECLARATION");
            ast_print_child(node, indent);
            break;
        case FUNCTION_DECLARATION:
            printf("%*s%s\n", indent, "", "FUNCTION_DECLARATION");
            ast_print_child(node, indent);
            break;
        case TYPE_DECLARATION:
            printf("%*s%s\n", indent, "", "TYPE_DECLARATION");
            ast_print_child(node, indent);
            break;
        // Parameter
        case FUNCTION_CALL_PARAMETER:
        case FUNCTION_DECL_PARAMETER:
            printf("%*s%s\n", indent, "", "(");
            ast_print_child(node, indent);
            printf("%*s%s\n", indent, "", ")");
            break;
        // Expression
        case BINARY_EXPRESSION:
            printf("%*s%s\n", indent, "", "(");
            printf("%*s%d\n", indent, "", node->u.op);
            ast_print_child(node, indent);
            printf("%*s%s\n", indent, "", ")");
            break;
        case UNARY_EXPRESSION:
            printf("%*s%s\n", indent, "", "(");
            printf("%*s%c\n", indent, "", node->u.op);
            ast_print_child(node, indent);
            printf("%*s%s\n", indent, "", ")");
            break;
        case ARRAY_EXPRESSION:
            printf("%*s%s\n", indent, "", "[");
            ast_print_child(node, indent);
            printf("%*s%s\n", indent, "", "]");
            break;
        case STRUCT_EXPRESSION:
            printf("%*s%s\n", indent, "", "{");
            ast_print_child(node, indent);
            printf("%*s%s\n", indent, "", "}");
            break;
        case TYPE_EXPRESSION:
            printf("%*s%s\n", indent, "", "TYPE_EXPRESSION");
            ast_print_child(node, indent);
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
        case CHAR_TOKEN:
            printf("%*s%s\n", indent, "", "CHAR_TOKEN");
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
        case NULL_TOKEN:
            printf("%*s%s\n", indent, "", "NULL_TOKEN");
            break;
        case FOR_CONDITION:
            printf("%*s%s\n", indent, "", "FOR_CONDITION");
            ast_print_child(node, indent);
            break;
        case FUNCTION_RECEIVER:
            printf("%*s%s\n", indent, "", "FUNCTION_RECEIVER");
            ast_print_child(node, indent);
            break;
        default:
            printf("%*s%s %d\n", indent, "", "ERROR!", node->type);
    }
}

void lgx_ast_print(lgx_ast_t* ast) {
    ast_print(ast->root, 0);
}

void lgx_ast_print_error(lgx_ast_t* ast) {
    lgx_ast_error_list_t* err;
    lgx_list_for_each_entry(err, lgx_ast_error_list_t, &ast->errors, head) {
        printf("%.*s", err->err_msg.length, err->err_msg.buffer);
    }
}