#include "../common/common.h"
#include "../common/val.h"
#include "../common/fun.h"
#include "../common/obj.h"
#include "../common/str.h"
#include "../extensions/ext.h"
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
void ast_parse_class_declaration(lgx_ast_t* ast, lgx_ast_node_t* parent);
void ast_parse_interface_declaration(lgx_ast_t* ast, lgx_ast_node_t* parent);

lgx_ast_node_t* ast_node_new(lgx_ast_t* ast, int n) {
    lgx_ast_node_t* node = (lgx_ast_node_t*)xcalloc(1, sizeof(lgx_ast_node_t) + n * sizeof(lgx_ast_node_t*));
    // TODO ERROR CHECK
    node->children = 0;
    node->size = n;

    node->line = ast->cur_line;

    return node;
}

lgx_ast_node_token_t* ast_node_token_new(lgx_ast_t* ast) {
    lgx_ast_node_token_t* node = (lgx_ast_node_token_t*)xcalloc(1, sizeof(lgx_ast_node_token_t));

    node->tk_start = ast->cur_start;
    node->tk_length = ast->cur_length;

    node->line = ast->cur_line;
    
    return node;
}

lgx_ast_node_t* ast_node_append_child(lgx_ast_node_t* parent, lgx_ast_node_t* child) {
    if (parent->children >= parent->size) {
        parent = (lgx_ast_node_t*)xrealloc(parent, sizeof(lgx_ast_node_t) + 2 * parent->size * sizeof(lgx_ast_node_t*));
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

    ast->err_info = (char *)xmalloc(256);
    ast->err_len = 0;

    return 0;
}

int ast_is_class(lgx_ast_t* ast) {
    lgx_str_t s;
    s.buffer = ast->cur_start;
    s.length = ast->cur_length;

    lgx_val_t *v = lgx_scope_global_val_get(ast->root, &s);
    if (!v) {
        return 0;
    }

    if (v->type != T_OBJECT) {
        return 0;
    }

    if (lgx_str_cmp(&s, v->v.obj->name) != 0) {
        return 0;
    }

    return 1;
}

void ast_set_variable_type(lgx_val_t *t, lgx_ast_node_t* node) {
    switch (node->u.type.type) {
        case TK_INT:
            t->type = T_LONG;
            break;
        case TK_FLOAT:
            t->type = T_DOUBLE;
            break;
        case TK_BOOL:
            t->type = T_BOOL;
            break;
        case TK_STR:
            t->type = T_STRING;
            break;
        case TK_ARR:
            t->type = T_ARRAY;
            break;
        case TK_ID: {
            lgx_val_t *v = lgx_scope_global_val_get(node, node->u.type.obj_name);
            t->type = T_OBJECT;
            t->v.obj = v->v.obj;
            break;
        }
        case TK_AUTO:
        default:
            t->type = T_UNDEFINED;
    }
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
        case THIS_TOKEN:
            xfree(node);
            return;
        case BLOCK_STATEMENT:
            if (node->u.symbols) {
                // value 会被标识类型，但只有 function 类型会真正分配空间
                lgx_hash_node_t *next = node->u.symbols->head;
                while (next) {
                    if (next->v.type != T_FUNCTION) {
                        next->v.type = T_UNDEFINED;
                    }
                    next = next->order;
                }
                lgx_hash_delete(node->u.symbols);
            }
            break;
        case FOR_STATEMENT:
        case WHILE_STATEMENT:
        case DO_WHILE_STATEMENT:
        case SWITCH_STATEMENT:
            if (node->u.jmps) {
                while (!lgx_list_empty(&node->u.jmps->head)) {
                    lgx_ast_node_list_t *n = lgx_list_first_entry(&node->u.jmps->head, lgx_ast_node_list_t, head);
                    lgx_list_del(&n->head);
                    xfree(n);
                }
                xfree(node->u.jmps);
            }
            break;
        case FUNCTION_DECLARATION:
            if (node->u.type.obj_name) {
                lgx_str_delete(node->u.type.obj_name);
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

    if (ast->err_no) {
        return;
    }

    va_start(args, fmt);
    ast->err_len = vsnprintf(ast->err_info, 256, fmt, args);
    va_end(args);
    
    ast->err_no = 1;
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

        switch (ast->cur_token) {
            case TK_AUTO: case TK_INT: case TK_FLOAT:
            case TK_BOOL: case TK_STR: case TK_ARR:
                variable_declaration->u.type.type = ast->cur_token;
                break;
            case TK_ID:
                if (ast_is_class(ast)) {
                    variable_declaration->u.type.type = TK_ID;
                    variable_declaration->u.type.obj_name = lgx_str_new_ref(ast->cur_start, ast->cur_length);
                    break;
                }
            default:
                ast_error(ast, "[Error] [Line:%d] type declaration expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
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
            if (variable_declaration->children == 1) {
                ast_error(ast, "[Error] [Line:%d] expression expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
                return;
            }
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

    int children = param_list->children;
    while (1) {
        ast_parse_expression(ast, param_list);
        if (param_list->children == children && ast->cur_token != ')') {
            ast_error(ast, "[Error] [Line:%d] expression expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
            return;
        }

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

    int children = array->children;
    while (1) {
        ast_parse_expression(ast, array);
        if (array->children == children && ast->cur_token != ']') {
            ast_error(ast, "[Error] [Line:%d] expression expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
            return;
        }

        if (ast->cur_token != ',') {
            break;
        }
        ast_step(ast);
    }

    if (ast->cur_token != ']') {
        ast_error(ast, "[Error] [Line:%d] ']' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);
}

// pri_expr -> ID | '(' sub_expr ')'
void ast_parse_pri_expression(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    int children = parent->children;

    switch (ast->cur_token) {
        case '(':
            ast_step(ast);

            ast_parse_expression(ast, parent);
            if (parent->children == children) {
                ast_error(ast, "[Error] [Line:%d] expression expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
                return;
            }

            if (ast->cur_token != ')') {
                ast_error(ast, "[Error] [Line:%d] ')' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
                return;
            }
            ast_step(ast);
            break;
        case TK_ID:
            ast_parse_id_token(ast, parent);
            break;
        case TK_THIS:{
            lgx_ast_node_token_t *id = ast_node_token_new(ast);
            id->type = THIS_TOKEN;
            ast_node_append_child(parent, (lgx_ast_node_t*)id);

            ast_step(ast);
            break;
        }
        default:
            return;
    }
}

// suf_expr -> pri_expr { '->' ID | '.' ID | '[' sub_expr ']' | funcargs }
void ast_parse_suf_expression(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    ast_parse_pri_expression(ast, parent);

    lgx_ast_node_t* binary_expression;
    while (1) {
        switch (ast->cur_token) {
            case TK_PTR:
            case '.':
            case '[':
            case '(':
                break;
            default:
                return;
        }

        binary_expression = ast_node_new(ast, 2);
        binary_expression->type = BINARY_EXPRESSION;
        binary_expression->parent = parent;
        ast_node_append_child(binary_expression, parent->child[parent->children-1]);
        parent->child[parent->children-1] = binary_expression;

        switch (ast->cur_token) {
            case TK_PTR:
                binary_expression->u.op = TK_PTR;
                ast_step(ast);

                if (ast->cur_token != TK_ID) {
                    ast_error(ast, "[Error] [Line:%d] ']' <identifier> expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
                    return;
                }
                ast_parse_id_token(ast, binary_expression);
                break;
            case '.':
                binary_expression->u.op = TK_ATTR;
                ast_step(ast);

                if (ast->cur_token != TK_ID) {
                    ast_error(ast, "[Error] [Line:%d] ']' <identifier> expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
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
                    ast_error(ast, "[Error] [Line:%d] ']' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
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
                    ast_error(ast, "[Error] [Line:%d] ')' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
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
        // case () []
        // case ->
        // case 单目运算符
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
        case TK_ASSIGN_ADD:
        case TK_ASSIGN_SUB:
        case TK_ASSIGN_MUL:
        case TK_ASSIGN_DIV:
        case TK_ASSIGN_AND:
        case TK_ASSIGN_OR:
        case TK_ASSIGN_SHL:
        case TK_ASSIGN_SHR:
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
        case TK_TYPEOF: // typeof 运算符
        case TK_NEW: // new 运算符
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

    if (parent->children == 0) {
        return;
    }

    int p = ast_operator_precedence(ast->cur_token);
    lgx_ast_node_t* binary_expression;
    while (1) {
        if (p < 0) {
            return;
        } else {
            switch (ast->cur_token) {
                case TK_AND: case TK_OR: case '=':
                case TK_ASSIGN_ADD:
                case TK_ASSIGN_SUB:
                case TK_ASSIGN_MUL:
                case TK_ASSIGN_DIV:
                case TK_ASSIGN_AND:
                case TK_ASSIGN_OR:
                case TK_ASSIGN_SHL:
                case TK_ASSIGN_SHR:
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
        ast_error(ast, "[Error] [Line:%d] '(' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);

    ast_parse_sub_expression(ast, parent, 15);

    if (ast->cur_token != ')') {
        ast_error(ast, "[Error] [Line:%d] ')' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
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

            lgx_val_t *t;
            if ((t = lgx_scope_val_add(parent->child[2], &s))) {
                ast_set_variable_type(t, parent->child[1]->child[i]);
                // 函数参数不检查是否使用
                t->u.c.used = 1;
            } else {
                ast_error(ast, "[Error] [Line:%d] identifier `%.*s` has already been declared\n", ast->cur_line, s.length, s.buffer);
                return;
            }
        }
    }
    
    ast_parse_statement(ast, block_statement);
}

void ast_parse_block_statement_with_braces(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    if (ast->cur_token != '{') {
        ast_error(ast, "[Error] [Line:%d] '{' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);

    ast_parse_block_statement(ast, parent);

    if (ast->cur_token != '}') {
        ast_error(ast, "[Error] [Line:%d] '}' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
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
        ast_error(ast, "[Error] [Line:%d] expression expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    
    ast_parse_block_statement_with_braces(ast, if_statement);
    
    if (ast->cur_token != TK_ELSE) {
        return;
    }
    ast_step(ast);
    
    if_statement->type = IF_ELSE_STATEMENT;

    if (ast->cur_token == TK_IF) {
        ast_parse_if_statement(ast, if_statement);
    } else {
        ast_parse_block_statement_with_braces(ast, if_statement);
    }
}

void ast_parse_for_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* for_statement = ast_node_new(ast, 4);
    for_statement->type = FOR_STATEMENT;
    ast_node_append_child(parent, for_statement);

    // ast->cur_token == TK_FOR
    ast_step(ast);

    if (ast->cur_token != '(') {
        ast_error(ast, "[Error] [Line:%d] '(' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);

    ast_parse_expression(ast, for_statement);
    if (for_statement->children == 0) {
        for_statement->children ++;
    }

    if (ast->cur_token != ';') {
        ast_error(ast, "[Error] [Line:%d] ';' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);

    ast_parse_expression(ast, for_statement);
    if (for_statement->children == 1) {
        for_statement->children ++;
    }

    if (ast->cur_token != ';') {
        ast_error(ast, "[Error] [Line:%d] ';' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);

    ast_parse_expression(ast, for_statement);
    if (for_statement->children == 2) {
        for_statement->children ++;
    }

    if (ast->cur_token != ')') {
        ast_error(ast, "[Error] [Line:%d] ')' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
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
        ast_error(ast, "[Error] [Line:%d] expression expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
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
    if (do_statement->children == 1) {
        ast_error(ast, "[Error] [Line:%d] expression expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
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
        ast_error(ast, "[Error] [Line:%d] expression expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }

    if (ast->cur_token != ':') {
        ast_error(ast, "[Error] [Line:%d] ':' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
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
        ast_error(ast, "[Error] [Line:%d] ':' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
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
        ast_error(ast, "[Error] [Line:%d] expression expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }

    if (ast->cur_token != '{') {
        ast_error(ast, "[Error] [Line:%d] '{' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
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
        ast_error(ast, "[Error] [Line:%d] '}' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);
}

void ast_parse_try_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* try_statement = ast_node_new(ast, 128);
    try_statement->type = TRY_STATEMENT;
    ast_node_append_child(parent, try_statement);

    // ast->cur_token == TK_TRY
    ast_step(ast);

    ast_parse_block_statement_with_braces(ast, try_statement);

    while (ast->cur_token == TK_CATCH) {
        ast_step(ast);

        lgx_ast_node_t* catch_statement = ast_node_new(ast, 2);
        catch_statement->type = CATCH_STATEMENT;
        ast_node_append_child(try_statement, catch_statement);

        if (ast->cur_token != '(') {
            ast_error(ast, "[Error] [Line:%d] '(' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
            return;
        }
        ast_step(ast);

        ast_parse_decl_parameter(ast, catch_statement);
        if (catch_statement->child[0]->children != 1) {
            ast_error(ast, "[Error] [Line:%d] there must be one and only one parameter in catch statements\n", ast->cur_line);
            return;
        }

        if (ast->cur_token != ')') {
            ast_error(ast, "[Error] [Line:%d] ')' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
            return;
        }
        ast_step(ast);

        ast_parse_block_statement_with_braces(ast, catch_statement);
    }

    if (ast->cur_token == TK_FINALLY) {
        ast_step(ast);

        lgx_ast_node_t* finally_statement = ast_node_new(ast, 1);
        finally_statement->type = FINALLY_STATEMENT;
        ast_node_append_child(try_statement, finally_statement);

        ast_parse_block_statement_with_braces(ast, finally_statement);
    }
}

void ast_parse_throw_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* throw_statement = ast_node_new(ast, 1);
    throw_statement->type = THROW_STATEMENT;
    ast_node_append_child(parent, throw_statement);

    // ast->cur_token == TK_THROW
    ast_step(ast);

    ast_parse_expression(ast, throw_statement);
    if (throw_statement->children == 0) {
        ast_error(ast, "[Error] [Line:%d] expression expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
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
    if (echo_statement->children == 0) {
        ast_error(ast, "[Error] [Line:%d] expression expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
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
            case '}': case TK_EOF: case TK_CASE: case TK_DEFAULT:
                return;
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
            case TK_TRY:
                ast_parse_try_statement(ast, parent);
                continue;
            case TK_THROW:
                ast_parse_throw_statement(ast, parent);
                break;
            case TK_AUTO: case TK_INT: case TK_FLOAT:
            case TK_BOOL: case TK_STR: case TK_ARR:
                ast_parse_variable_declaration(ast, parent);
                break;
            case TK_ID:
                if (ast_is_class(ast)) {
                    ast_parse_variable_declaration(ast, parent);
                } else {
                    ast_parse_expression_statement(ast, parent);
                }
                break;
            case TK_FUNCTION:
                if (parent->parent) {
                    ast_error(ast, "[Error] [Line:%d] functions can only be defined in top-level scope\n", ast->cur_line);
                    return;
                }
                ast_parse_function_declaration(ast, parent);
                continue;
            case TK_CLASS:
                ast_parse_class_declaration(ast, parent);
                continue;
            case TK_INTERFACE:
                ast_parse_interface_declaration(ast, parent);
                continue;
            default:
                ast_parse_expression_statement(ast, parent);
        }

        if (ast->cur_token != ';') {
            // 语句应当以分号结尾。以花括号结尾的语句可以省略分号。
            ast_error(ast, "[Error] [Line:%d] ';' expected before '%.*s'\n", ast->cur_line, ast->cur_length, ast->cur_start);
            return;
        }
        ast_step(ast);
    }
}

void ast_parse_variable_declaration(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* variable_declaration;

    int var_type = ast->cur_token;
    char *type_str = ast->cur_start;
    int type_len = ast->cur_length;
    ast_step(ast);

    while (1) {
        variable_declaration = ast_node_new(ast, 2);
        variable_declaration->type = VARIABLE_DECLARATION;
        variable_declaration->u.type.type = var_type;
        variable_declaration->u.type.obj_name = lgx_str_new_ref(type_str, type_len);
        ast_node_append_child(parent, variable_declaration);

        if (ast->cur_token != TK_ID) {
            ast_error(ast, "[Error] [Line:%d] `%.*s` is not a <identifier>\n", ast->cur_line, ast->cur_length, ast->cur_start);
            return;
        } else {
            //printf("[Info] [Line:%d] variable `%.*s` declared\n", ast->cur_line, ast->cur_length, ast->cur_start);
            ast_parse_id_token(ast, variable_declaration);
        }
        
        if (ast->cur_token == '=') {
            ast_step(ast);
            
            //printf("[Info] [Line:%d] variable initialized\n", ast->cur_line);
            ast_parse_expression(ast, variable_declaration);
            if (variable_declaration->children == 1) {
                ast_error(ast, "[Error] [Line:%d] expression expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
                return;
            }
        }

        // 创建变量加入作用域
        lgx_str_t s;
        s.buffer = ((lgx_ast_node_token_t *)(variable_declaration->child[0]))->tk_start;
        s.length = ((lgx_ast_node_token_t *)(variable_declaration->child[0]))->tk_length;

        lgx_val_t *t;
        if ((t = lgx_scope_val_add(variable_declaration, &s))) {
            ast_set_variable_type(t, variable_declaration);
            // TODO 只检查局部变量
            t->u.c.used = 1;
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
        ast_error(ast, "[Error] [Line:%d] `<identifier>` expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_parse_id_token(ast, function_declaration);

    // 创建函数加入作用域
    lgx_str_t s;
    lgx_ast_node_token_t *n = (lgx_ast_node_token_t *)function_declaration->child[0];
    s.buffer = n->tk_start;
    s.length = n->tk_length;

    lgx_val_t *f;
    if ((f = lgx_scope_val_add(function_declaration, &s))) {
        f->type = T_FUNCTION;
        f->u.c.used = 1;
    } else {
        ast_error(ast, "[Error] [Line:%d] identifier `%.*s` has already been declared\n", ast->cur_line, n->tk_length, n->tk_start);
        return;
    }

    if (ast->cur_token != '(') {
        ast_error(ast, "[Error] [Line:%d] '(' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);

    ast_parse_decl_parameter(ast, function_declaration);

    if (ast->cur_token != ')') {
        ast_error(ast, "[Error] [Line:%d] ')' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);

    // 根据参数数量创建函数
    f->v.fun = lgx_fun_new(function_declaration->child[1]->children);
    int i;
    for (i = 0; i < function_declaration->child[1]->children; i++) {
        lgx_ast_node_t *n = function_declaration->child[1]->child[i];
        // 设置参数类型
        ast_set_variable_type(&f->v.fun->args[i], n);
        // 是否有默认参数
        if (n->child[1]) {
            f->v.fun->args[i].u.c.init = 1;
        }
    }

    // 解析返回值
    switch (ast->cur_token) {
        case TK_AUTO: case TK_INT: case TK_FLOAT:
        case TK_BOOL: case TK_STR: case TK_ARR: {
            function_declaration->u.type.type = ast->cur_token;
            ast_step(ast);
            break;
        }
        case TK_ID:
            if (ast_is_class(ast)) {
                function_declaration->u.type.type = ast->cur_token;
                function_declaration->u.type.obj_name = lgx_str_new_ref(ast->cur_start, ast->cur_length);
            } else {
                ast_error(ast, "[Error] [Line:%d] type declaration expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
                return;
            }
            ast_step(ast);
            break;
        default:
            function_declaration->u.type.type = TK_AUTO;
    }
    ast_set_variable_type(&f->v.fun->ret, function_declaration);

    ast_parse_block_statement_with_braces(ast, function_declaration);
}

void ast_parse_modifer(lgx_ast_t* ast, char *is_static, char *is_const, char *access) {
    *is_static = -1;
    *is_const = -1;
    *access = -1;

    while (1) {
        if (ast->cur_token == TK_PUBLIC) {
            if (*access >= 0) {
                ast_error(ast, "[Error] [Line:%d] access modifier already seen before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
                return;
            } else {
                *access = P_PUBLIC;
                ast_step(ast);
            }
        } else if (ast->cur_token == TK_PROTECTED) {
            if (*access >= 0) {
                ast_error(ast, "[Error] [Line:%d] access modifier already seen before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
                return;
            } else {
                *access = P_PROTECTED;
                ast_step(ast);
            }
        } else if (ast->cur_token == TK_PRIVATE) {
            if (*access >= 0) {
                ast_error(ast, "[Error] [Line:%d] access modifier already seen before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
                return;
            } else {
                *access = P_PRIVATE;
                ast_step(ast);
            }
        } else if (ast->cur_token == TK_STATIC) {
            if (*is_static >= 0) {
                ast_error(ast, "[Error] [Line:%d] static modifier already seen before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
                return;
            } else {
                *is_static = 1;
                ast_step(ast);
            }
        } else if (ast->cur_token == TK_CONST) {
            if (*is_const >= 0) {
                ast_error(ast, "[Error] [Line:%d] const modifier already seen before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
                return;
            } else {
                *is_const = 1;
                ast_step(ast);
            }
        } else {
            break;
        }
    }

    if (*access < 0) {
        *access = P_PACKAGE;
    }
    if (*is_static < 0) {
        *is_static = 0;
    }
    if (*is_const < 0) {
        *is_const = 0;
    }
}

// 根据变量类型初始化变量的值
void ast_init_value(lgx_val_t *v) {
    switch (v->type) {
        case T_UNDEFINED: break;
        case T_LONG:      v->v.l = 0; break;
        case T_DOUBLE:    v->v.d = 0; break;
        case T_BOOL:      v->v.l = 0; break;
        case T_REDERENCE: break;
        case T_STRING:    v->v.str = lgx_str_new_ref("", 0); break;
        case T_ARRAY:     v->v.arr = lgx_hash_new(8); break;
        case T_OBJECT:    break;
        case T_FUNCTION:  break;
        case T_RESOURCE:  break;
        default: assert(0);
    }
}

void ast_parse_class_declaration(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* class_declaration = ast_node_new(ast, 2);
    class_declaration->type = CLASS_DECLARATION;
    ast_node_append_child(parent, class_declaration);

    // ast->cur_token == TK_CLASS
    ast_step(ast);

    if (ast->cur_token != TK_ID) {
        ast_error(ast, "[Error] [Line:%d] `<identifier>` expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_parse_id_token(ast, class_declaration);

    // 创建类加入作用域
    lgx_str_t s;
    lgx_ast_node_token_t *n = (lgx_ast_node_token_t *)class_declaration->child[0];
    s.buffer = n->tk_start;
    s.length = n->tk_length;

    lgx_val_t *f;
    if ((f = lgx_scope_val_add(class_declaration, &s))) {
        f->type = T_OBJECT;
        f->v.obj = lgx_obj_create(&s);
        f->u.c.used = 1;
    } else {
        ast_error(ast, "[Error] [Line:%d] identifier `%.*s` has already been declared\n", ast->cur_line, n->tk_length, n->tk_start);
        return;
    }

    if (ast->cur_token != '{') {
        ast_error(ast, "[Error] [Line:%d] '{' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);

    lgx_ast_node_t* block_statement = ast_node_new(ast, 128);
    block_statement->type = BLOCK_STATEMENT;
    ast_node_append_child(class_declaration, block_statement);

    // 创建新的作用域
    block_statement->u.symbols = lgx_hash_new(32);

    int endloop = 0;
    while (!endloop) {
        char is_static, is_const, access;
        ast_parse_modifer(ast, &is_static, &is_const, &access);

        if (ast->cur_token == TK_FUNCTION) {
            if (is_const) {
                ast_error(ast, "[Error] [Line:%d] method could not be defined as const\n", ast->cur_line);
                return;
            }

            lgx_ast_node_t* method_declaration = ast_node_new(ast, 1);
            method_declaration->type = METHOD_DECLARATION;
            method_declaration->u.modifier.is_static = is_static;
            method_declaration->u.modifier.is_const = is_const;
            method_declaration->u.modifier.access = access;
            ast_node_append_child(block_statement, method_declaration);

            ast_parse_function_declaration(ast, method_declaration);
            if (ast->err_no) {
                return;
            }

            lgx_hash_node_t n;
            lgx_ast_node_token_t *method_name = (lgx_ast_node_token_t *)method_declaration->child[0]->child[0];
            n.k.type = T_STRING;
            n.k.v.str = lgx_str_new_ref(method_name->tk_start, method_name->tk_length);
            n.v = *lgx_scope_local_val_get(block_statement, n.k.v.str);

            n.v.v.fun->is_static = is_static;
            n.v.v.fun->access = access;

            lgx_obj_add_method(f->v.obj, &n);
        } else {
            switch (ast->cur_token) {
                case TK_AUTO: case TK_INT: case TK_FLOAT:
                case TK_BOOL: case TK_STR: case TK_ARR: case TK_ID: {
                    if (ast->cur_token == TK_ID) {
                        if (!ast_is_class(ast)) {
                            ast_error(ast, "[Error] [Line:%d] type declaration expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
                            return;
                        }
                    }

                    lgx_ast_node_t* property_declaration = ast_node_new(ast, 1);
                    property_declaration->type = PROPERTY_DECLARATION;
                    property_declaration->u.modifier.is_static = is_static;
                    property_declaration->u.modifier.is_const = is_const;
                    property_declaration->u.modifier.access = access;
                    ast_node_append_child(block_statement, property_declaration);

                    ast_parse_variable_declaration(ast, property_declaration);
                    if (ast->err_no) {
                        return;
                    }

                    lgx_hash_node_t n;
                    lgx_ast_node_token_t *property_name = (lgx_ast_node_token_t *)property_declaration->child[0]->child[0];
                    n.k.type = T_STRING;
                    n.k.v.str = lgx_str_new_ref(property_name->tk_start, property_name->tk_length);

                    lgx_val_t *v = lgx_scope_local_val_get(block_statement, n.k.v.str);
                    n.v.type = v->type;
                    ast_init_value(&n.v);

                    n.v.u.c.is_static = is_static;
                    n.v.u.c.is_const = is_const;
                    n.v.u.c.access = access;

                    lgx_obj_add_property(f->v.obj, &n);

                    if (ast->cur_token != ';') {
                        ast_error(ast, "[Error] [Line:%d] ';' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
                        return;
                    }
                    ast_step(ast);
                    break;
                }
                default:
                    endloop = 1;
            }
        }
    }

    if (ast->cur_token != '}') {
        ast_error(ast, "[Error] [Line:%d] '}' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);
}

void ast_parse_interface_declaration(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* interface_declaration = ast_node_new(ast, 128);
    interface_declaration->type = INTERFACE_DECLARATION;
    ast_node_append_child(parent, interface_declaration);

    // ast->cur_token == TK_INTERFACE
    ast_step(ast);

    if (ast->cur_token != TK_ID) {
        ast_error(ast, "[Error] [Line:%d] `<identifier>` expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_parse_id_token(ast, interface_declaration);

    if (ast->cur_token != '{') {
        ast_error(ast, "[Error] [Line:%d] '{' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);

    if (ast->cur_token != '}') {
        ast_error(ast, "[Error] [Line:%d] '}' expected before `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);
}

int lgx_ast_parser(lgx_ast_t* ast) {
    ast->root = ast_node_new(ast, 128);
    ast->root->type = BLOCK_STATEMENT;

    // 全局作用域
    ast->root->u.symbols = lgx_hash_new(32);

    // 扩展接口
    lgx_ext_load_symbols(ast->root->u.symbols);

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
        case TRY_STATEMENT:
            printf("%*s%s\n", indent, "", "TRY_STATEMENT");
            break;
        case THROW_STATEMENT:
            printf("%*s%s\n", indent, "", "THROW_STATEMENT");
            lgx_ast_print(node->child[0], indent+2);
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
        case CLASS_DECLARATION:
            printf("%*s%s\n", indent, "", "CLASS_DECLARATION");
            for(i = 0; i < node->children; i++) {
                lgx_ast_print(node->child[i], indent+2);
            }
            break;
        case INTERFACE_DECLARATION:
            printf("%*s%s\n", indent, "", "INTERFACE_DECLARATION");
            for(i = 0; i < node->children; i++) {
                lgx_ast_print(node->child[i], indent+2);
            }
            break;
        case PROPERTY_DECLARATION:
            printf("%*s%s\n", indent, "", "PROPERTY_DECLARATION");
            lgx_ast_print(node->child[0], indent+2);
            break;
        case METHOD_DECLARATION:
            printf("%*s%s\n", indent, "", "METHOD_DECLARATION");
            lgx_ast_print(node->child[0], indent+2);
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