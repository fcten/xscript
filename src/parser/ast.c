#include <linux/limits.h>
#include <unistd.h>

#include "../common/common.h"
#include "../common/val.h"
#include "../common/fun.h"
#include "../common/obj.h"
#include "../common/str.h"
#include "../extensions/ext.h"
#include "scope.h"
#include "ast.h"

void ast_parse_statement(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent);

void ast_parse_bsc_expression(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent);
void ast_parse_pri_expression(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent);
void ast_parse_suf_expression(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent);
void ast_parse_sub_expression(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent, int precedence);
void ast_parse_expression(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent);

void ast_parse_variable_declaration(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent, lgx_modifier_t *modifier);
void ast_parse_function_declaration(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent, lgx_modifier_t *modifier);
void ast_parse_class_declaration(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent);
void ast_parse_interface_declaration(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent);

lgx_ast_node_t* ast_node_new(lgx_ast_t* ast, lgx_package_t *pkg, int n) {
    lgx_ast_node_t* node = (lgx_ast_node_t*)xcalloc(1, sizeof(lgx_ast_node_t) + n * sizeof(lgx_ast_node_t*));
    // TODO ERROR CHECK
    node->children = 0;
    node->size = n;

    node->file = pkg->lex.file;
    node->line = pkg->cur_line;

    return node;
}

lgx_ast_node_token_t* ast_node_token_new(lgx_ast_t* ast, lgx_package_t *pkg) {
    lgx_ast_node_token_t* node = (lgx_ast_node_token_t*)xcalloc(1, sizeof(lgx_ast_node_token_t));

    node->tk_start = pkg->cur_start;
    node->tk_length = pkg->cur_length;

    node->file = pkg->lex.file;
    node->line = pkg->cur_line;
    
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

    ast->err_info = (char *)xmalloc(256);
    ast->err_len = 0;

    return 0;
}

int ast_is_class(lgx_ast_t* ast, lgx_package_t *pkg) {
    lgx_str_t s;
    s.buffer = pkg->cur_start;
    s.length = pkg->cur_length;

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
                while (!wbt_list_empty(&node->u.jmps->head)) {
                    lgx_ast_node_list_t *n = wbt_list_first_entry(&node->u.jmps->head, lgx_ast_node_list_t, head);
                    wbt_list_del(&n->head);
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
        default: // todo ?
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
    // TODO 释放所有源文件
    //xfree(ast->lex.source);

    ast_node_cleanup(ast->root);

    return 0;
}

void ast_error(lgx_ast_t* ast, lgx_package_t* pkg, const char *fmt, ...) {
    va_list   args;

    if (ast->err_no) {
        return;
    }

    char *file = NULL;

    if (pkg->lex.file) {
    #ifndef WIN32
        file = strrchr (pkg->lex.file, '/');
    #else
        file = strrchr (pkg->lex.file, '\\');
    #endif
    }

    if (file) {
        file += 1;
    } else {
        file = pkg->lex.file;
    }

    if (file) {
        ast->err_len = snprintf(ast->err_info, 256, "[ERROR] [%s:%d] ", file, pkg->cur_line + 1);
    } else {
        ast->err_len = snprintf(ast->err_info, 256, "[ERROR] ");
    }

    va_start(args, fmt);
    ast->err_len += vsnprintf(ast->err_info + ast->err_len, 256 - ast->err_len, fmt, args);
    va_end(args);
    
    ast->err_no = 1;
}

int ast_next(lgx_package_t* pkg) {
    unsigned int token;
    while(1) {
        token = lgx_lex(&pkg->lex);
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

void ast_step(lgx_package_t* pkg) {
    pkg->cur_token = ast_next(pkg);
    pkg->cur_start = pkg->lex.source + pkg->lex.milestone;
    pkg->cur_length = pkg->lex.offset - pkg->lex.milestone;
    pkg->cur_line = pkg->lex.line;
}

void ast_parse_id_token(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_token_t* id = ast_node_token_new(ast, pkg);
    id->type = IDENTIFIER_TOKEN;
    ast_node_append_child(parent, (lgx_ast_node_t*)id);

    ast_step(pkg);
}

void ast_parse_decl_parameter(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_t* param_list = ast_node_new(ast, pkg, 128);
    param_list->type = FUNCTION_DECL_PARAMETER;
    ast_node_append_child(parent, param_list);

    if (pkg->cur_token == ')') {
        // 参数为空
        return;
    }

    while (1) {
        lgx_ast_node_t* variable_declaration = ast_node_new(ast, pkg, 2);
        variable_declaration->type = VARIABLE_DECLARATION;
        ast_node_append_child(param_list, variable_declaration);

        switch (pkg->cur_token) {
            case TK_AUTO: case TK_INT: case TK_FLOAT:
            case TK_BOOL: case TK_STR: case TK_ARR:
                variable_declaration->u.type.type = pkg->cur_token;
                break;
            case TK_ID:
                if (ast_is_class(ast, pkg)) {
                    variable_declaration->u.type.type = TK_ID;
                    variable_declaration->u.type.obj_name = lgx_str_new_ref(pkg->cur_start, pkg->cur_length);
                    break;
                }
            default:
                ast_error(ast, pkg, "type declaration expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
                return;
        }
        ast_step(pkg);

        if (pkg->cur_token != TK_ID) {
            ast_error(ast, pkg, "`%.*s` is not a <identifier>\n", pkg->cur_length, pkg->cur_start);
            return;
        } else {
            ast_parse_id_token(ast, pkg, variable_declaration);
        }
        
        if (pkg->cur_token == '=') {
            ast_step(pkg);

            ast_parse_expression(ast, pkg, variable_declaration);
            if (variable_declaration->children == 1) {
                ast_error(ast, pkg, "expression expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
                return;
            }
        }

        if (pkg->cur_token != ',') {
            return;
        }
        ast_step(pkg);
    }
}

void ast_parse_call_parameter(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_t* param_list = ast_node_new(ast, pkg, 128);
    param_list->type = FUNCTION_CALL_PARAMETER;
    ast_node_append_child(parent, param_list);

    int children = param_list->children;
    while (1) {
        ast_parse_expression(ast, pkg, param_list);
        if (param_list->children == children && pkg->cur_token != ')') {
            ast_error(ast, pkg, "expression expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
            return;
        }

        if (pkg->cur_token != ',') {
            return;
        }
        ast_step(pkg);
    }
}

void ast_parse_array_expression(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_t* array = ast_node_new(ast, pkg, 128);
    array->type = ARRAY_TOKEN;
    ast_node_append_child(parent, array);

    // pkg->cur_token == '['
    ast_step(pkg);

    int children = array->children;
    while (1) {
        ast_parse_expression(ast, pkg, array);
        if (array->children == children && pkg->cur_token != ']') {
            ast_error(ast, pkg, "expression expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
            return;
        }

        if (pkg->cur_token != ',') {
            break;
        }
        ast_step(pkg);
    }

    if (pkg->cur_token != ']') {
        ast_error(ast, pkg, "']' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_step(pkg);
}

// pri_expr -> ID | '(' sub_expr ')'
void ast_parse_pri_expression(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    int children = parent->children;

    switch (pkg->cur_token) {
        case '(':
            ast_step(pkg);

            ast_parse_expression(ast, pkg, parent);
            if (parent->children == children) {
                ast_error(ast, pkg, "expression expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
                return;
            }

            if (pkg->cur_token != ')') {
                ast_error(ast, pkg, "')' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
                return;
            }
            ast_step(pkg);
            break;
        case TK_ID:
            ast_parse_id_token(ast, pkg, parent);
            break;
        case TK_THIS:{
            lgx_ast_node_token_t *id = ast_node_token_new(ast, pkg);
            id->type = THIS_TOKEN;
            ast_node_append_child(parent, (lgx_ast_node_t*)id);

            ast_step(pkg);
            break;
        }
        default:
            return;
    }
}

// suf_expr -> pri_expr { '->' ID | '.' ID | '[' sub_expr ']' | funcargs }
void ast_parse_suf_expression(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    ast_parse_pri_expression(ast, pkg, parent);

    lgx_ast_node_t* binary_expression;
    while (1) {
        switch (pkg->cur_token) {
            case TK_PTR:
            case '.':
            case '[':
            case '(':
                break;
            default:
                return;
        }

        binary_expression = ast_node_new(ast, pkg, 2);
        binary_expression->type = BINARY_EXPRESSION;
        binary_expression->parent = parent;
        ast_node_append_child(binary_expression, parent->child[parent->children-1]);
        parent->child[parent->children-1] = binary_expression;

        switch (pkg->cur_token) {
            case TK_PTR:
                binary_expression->u.op = TK_PTR;
                ast_step(pkg);

                if (pkg->cur_token != TK_ID) {
                    ast_error(ast, pkg, "']' <identifier> expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
                    return;
                }
                ast_parse_id_token(ast, pkg, binary_expression);
                break;
            case '.':
                binary_expression->u.op = TK_ATTR;
                ast_step(pkg);

                if (pkg->cur_token != TK_ID) {
                    ast_error(ast, pkg, "']' <identifier> expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
                    return;
                }
                ast_parse_id_token(ast, pkg, binary_expression);
                break;
            case '[':
                // 数组访问操作符
                binary_expression->u.op = TK_INDEX;
                ast_step(pkg);

                ast_parse_expression(ast, pkg, binary_expression);

                if (pkg->cur_token != ']') {
                    ast_error(ast, pkg, "']' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
                    return;
                }
                ast_step(pkg);
                break;
            case '(':
                // 函数调用操作符
                binary_expression->u.op = TK_CALL;
                ast_step(pkg);

                ast_parse_call_parameter(ast, pkg, binary_expression);

                if (pkg->cur_token != ')') {
                    ast_error(ast, pkg, "')' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
                    return;
                }
                ast_step(pkg);
                break;
        }
    }
}

// bsc_expr -> NUMBER | STRING | ARRAY | OBJECT | true | false | suf_expr
void ast_parse_bsc_expression(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_token_t* id;
    switch (pkg->cur_token) {
        case TK_LONG:
            id = ast_node_token_new(ast, pkg);
            id->type = LONG_TOKEN;
            ast_node_append_child(parent, (lgx_ast_node_t*)id);

            ast_step(pkg);
            break;
        case TK_DOUBLE:
            id = ast_node_token_new(ast, pkg);
            id->type = DOUBLE_TOKEN;
            ast_node_append_child(parent, (lgx_ast_node_t*)id);

            ast_step(pkg);
            break;
        case TK_STRING:
            id = ast_node_token_new(ast, pkg);
            id->type = STRING_TOKEN;
            ast_node_append_child(parent, (lgx_ast_node_t*)id);

            ast_step(pkg);
            break;
        case TK_TRUE:
            id = ast_node_token_new(ast, pkg);
            id->type = TRUE_TOKEN;
            ast_node_append_child(parent, (lgx_ast_node_t*)id);

            ast_step(pkg);
            break;
        case TK_FALSE:
            id = ast_node_token_new(ast, pkg);
            id->type = FALSE_TOKEN;
            ast_node_append_child(parent, (lgx_ast_node_t*)id);

            ast_step(pkg);
            break;
        case TK_UNDEFINED:
            id = ast_node_token_new(ast, pkg);
            id->type = UNDEFINED_TOKEN;
            ast_node_append_child(parent, (lgx_ast_node_t*)id);

            ast_step(pkg);
            break;
        case '[':
            // 数组字面量
            ast_parse_array_expression(ast, pkg, parent);
            break;
        default:
            ast_parse_suf_expression(ast, pkg, parent);
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
void ast_parse_sub_expression(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent, int precedence) {
    lgx_ast_node_t* unary_expression;
    switch (pkg->cur_token) {
        case '!': // 逻辑非运算符
        case '~': // 按位取反运算符
        case '-': // 负号运算符
        case TK_TYPEOF: // typeof 运算符
        case TK_NEW: // new 运算符
        case TK_AWAIT: // await 运算符
            // 单目运算符
            unary_expression = ast_node_new(ast, pkg, 1);
            unary_expression->type = UNARY_EXPRESSION;
            unary_expression->u.op = pkg->cur_token;
            ast_node_append_child(parent, unary_expression);

            ast_step(pkg);

            ast_parse_sub_expression(ast, pkg, unary_expression, 2);
            break;    
        default:
            ast_parse_bsc_expression(ast, pkg, parent);
    }

    if (parent->children == 0) {
        return;
    }

    int p = ast_operator_precedence(pkg->cur_token);
    lgx_ast_node_t* binary_expression;
    while (1) {
        if (p < 0) {
            return;
        } else {
            switch (pkg->cur_token) {
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

        binary_expression = ast_node_new(ast, pkg, 2);
        binary_expression->type = BINARY_EXPRESSION;
        binary_expression->parent = parent;
        binary_expression->u.op = pkg->cur_token;
        ast_node_append_child(binary_expression, parent->child[parent->children-1]);
        parent->child[parent->children-1] = binary_expression;

        ast_step(pkg);

        ast_parse_sub_expression(ast, pkg, binary_expression, p);
        if (binary_expression->children == 1) {
            ast_error(ast, pkg, "unexpected right value for binary operation near `%.*s`\n", pkg->cur_length, pkg->cur_start);
            return;
        }

        p = ast_operator_precedence(pkg->cur_token);
    }
}

void ast_parse_expression_with_parentheses(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    if (pkg->cur_token != '(') {
        ast_error(ast, pkg, "'(' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_step(pkg);

    ast_parse_sub_expression(ast, pkg, parent, 15);

    if (pkg->cur_token != ')') {
        ast_error(ast, pkg, "')' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_step(pkg);
}

void ast_parse_expression(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    ast_parse_sub_expression(ast, pkg, parent, 15);
}

void ast_parse_block_statement(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_t* block_statement = ast_node_new(ast, pkg, 128);
    block_statement->type = BLOCK_STATEMENT;
    ast_node_append_child(parent, block_statement);

    // 创建新的作用域
    block_statement->u.symbols = lgx_hash_new(32);

    // 把 函数/catch block 的参数添加到符号表
    if (parent->type == FUNCTION_DECLARATION ||
        parent->type == CATCH_STATEMENT) {
        lgx_ast_node_t *n = NULL;
        switch (parent->type) {
            case FUNCTION_DECLARATION: n = parent->child[1]; break;
            case CATCH_STATEMENT: n = parent->child[0]; break;
            default: break; // todo error
        }
        lgx_str_t s;
        int i;
        for (i = 0; i < n->children; i++) {
            s.buffer = ((lgx_ast_node_token_t *)n->child[i]->child[0])->tk_start;
            s.length = ((lgx_ast_node_token_t *)n->child[i]->child[0])->tk_length;

            lgx_val_t *t;
            if ((t = lgx_scope_val_add(block_statement, &s))) {
                ast_set_variable_type(t, n->child[i]);
                // 函数参数不检查是否使用
                t->u.c.used = 1;
            } else {
                ast_error(ast, pkg, "identifier `%.*s` has already been declared\n", s.length, s.buffer);
                return;
            }
        }
    }
    
    ast_parse_statement(ast, pkg, block_statement);
}

void ast_parse_block_statement_with_braces(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    if (pkg->cur_token != '{') {
        ast_error(ast, pkg, "'{' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_step(pkg);

    ast_parse_block_statement(ast, pkg, parent);

    if (pkg->cur_token != '}') {
        ast_error(ast, pkg, "'}' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_step(pkg);
}

void ast_parse_if_statement(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_t* if_statement = ast_node_new(ast, pkg, 3);
    if_statement->type = IF_STATEMENT;
    ast_node_append_child(parent, if_statement);

    // pkg->cur_token == TK_IF
    ast_step(pkg);
    
    ast_parse_expression_with_parentheses(ast, pkg, if_statement);
    if (if_statement->children == 0) {
        ast_error(ast, pkg, "expression expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    
    ast_parse_block_statement_with_braces(ast, pkg, if_statement);
    
    if (pkg->cur_token != TK_ELSE) {
        return;
    }
    ast_step(pkg);
    
    if_statement->type = IF_ELSE_STATEMENT;

    if (pkg->cur_token == TK_IF) {
        ast_parse_if_statement(ast, pkg, if_statement);
    } else {
        ast_parse_block_statement_with_braces(ast, pkg, if_statement);
    }
}

void ast_parse_for_statement(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_t* for_statement = ast_node_new(ast, pkg, 4);
    for_statement->type = FOR_STATEMENT;
    ast_node_append_child(parent, for_statement);

    // pkg->cur_token == TK_FOR
    ast_step(pkg);

    if (pkg->cur_token != '(') {
        ast_error(ast, pkg, "'(' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_step(pkg);

    ast_parse_expression(ast, pkg, for_statement);
    if (for_statement->children == 0) {
        for_statement->children ++;
    }

    if (pkg->cur_token != ';') {
        ast_error(ast, pkg, "';' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_step(pkg);

    ast_parse_expression(ast, pkg, for_statement);
    if (for_statement->children == 1) {
        for_statement->children ++;
    }

    if (pkg->cur_token != ';') {
        ast_error(ast, pkg, "';' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_step(pkg);

    ast_parse_expression(ast, pkg, for_statement);
    if (for_statement->children == 2) {
        for_statement->children ++;
    }

    if (pkg->cur_token != ')') {
        ast_error(ast, pkg, "')' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_step(pkg);

    ast_parse_block_statement_with_braces(ast, pkg, for_statement);
}

void ast_parse_while_statement(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_t* while_statement = ast_node_new(ast, pkg, 2);
    while_statement->type = WHILE_STATEMENT;
    ast_node_append_child(parent, while_statement);

    // pkg->cur_token == TK_WHILE
    ast_step(pkg);
    
    ast_parse_expression_with_parentheses(ast, pkg, while_statement);
    if (while_statement->children == 0) {
        ast_error(ast, pkg, "expression expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    
    ast_parse_block_statement_with_braces(ast, pkg, while_statement);
}

void ast_parse_do_statement(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_t* do_statement = ast_node_new(ast, pkg, 2);
    do_statement->type = DO_WHILE_STATEMENT;
    ast_node_append_child(parent, do_statement);

    // pkg->cur_token == TK_DO
    ast_step(pkg);
    
    ast_parse_block_statement_with_braces(ast, pkg, do_statement);

    if (pkg->cur_token != TK_WHILE) {
        ast_error(ast, pkg, "'while' expected, not `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_step(pkg);
    
    ast_parse_expression_with_parentheses(ast, pkg, do_statement);
    if (do_statement->children == 1) {
        ast_error(ast, pkg, "expression expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
}

void ast_parse_break_statement(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_t* break_statement = ast_node_new(ast, pkg, 0);
    break_statement->type = BREAK_STATEMENT;
    ast_node_append_child(parent, break_statement);

    // pkg->cur_token == TK_BREAK
    ast_step(pkg);
}

void ast_parse_continue_statement(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_t* continue_statement = ast_node_new(ast, pkg, 0);
    continue_statement->type = CONTINUE_STATEMENT;
    ast_node_append_child(parent, continue_statement);

    // pkg->cur_token == TK_CONTINUE
    ast_step(pkg);
}

void ast_parse_case_statement(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_t* case_statement = ast_node_new(ast, pkg, 2);
    case_statement->type = CASE_STATEMENT;
    ast_node_append_child(parent, case_statement);

    // pkg->cur_token == TK_CASE
    ast_step(pkg);

    ast_parse_expression(ast, pkg, case_statement);
    if (case_statement->children == 0) {
        ast_error(ast, pkg, "expression expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }

    if (pkg->cur_token != ':') {
        ast_error(ast, pkg, "':' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_step(pkg);

    if (pkg->cur_token == '{') {
        ast_parse_block_statement_with_braces(ast, pkg, case_statement);
    } else {
        ast_parse_block_statement(ast, pkg, case_statement);
    }
}

void ast_parse_default_statement(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_t* default_statement = ast_node_new(ast, pkg, 1);
    default_statement->type = DEFAULT_STATEMENT;
    ast_node_append_child(parent, default_statement);

    // pkg->cur_token == TK_DEFAULT
    ast_step(pkg);

    if (pkg->cur_token != ':') {
        ast_error(ast, pkg, "':' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_step(pkg);

    if (pkg->cur_token == '{') {
        ast_parse_block_statement_with_braces(ast, pkg, default_statement);
    } else {
        ast_parse_block_statement(ast, pkg, default_statement);
    }
}

void ast_parse_switch_statement(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_t* switch_statement = ast_node_new(ast, pkg, 128);
    switch_statement->type = SWITCH_STATEMENT;
    ast_node_append_child(parent, switch_statement);

    // pkg->cur_token == TK_SWITCH
    ast_step(pkg);

    ast_parse_expression_with_parentheses(ast, pkg, switch_statement);
    if (switch_statement->children == 0) {
        ast_error(ast, pkg, "expression expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }

    if (pkg->cur_token != '{') {
        ast_error(ast, pkg, "'{' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_step(pkg);

    // 解析所有 case 和 default
    int has_default = 0;
    while (1) {
        if (pkg->cur_token == TK_CASE) {
            ast_parse_case_statement(ast, pkg, switch_statement);
        } else if (pkg->cur_token == TK_DEFAULT) {
            if (has_default) {
                ast_error(ast, pkg, "switch statement should only contain one default label\n");
                return;
            }
            has_default = 1;

            ast_parse_default_statement(ast, pkg, switch_statement);
        } else {
            break;
        }
    }

    if (pkg->cur_token != '}') {
        ast_error(ast, pkg, "'}' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_step(pkg);
}

void ast_parse_try_statement(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_t* try_statement = ast_node_new(ast, pkg, 128);
    try_statement->type = TRY_STATEMENT;
    ast_node_append_child(parent, try_statement);

    // pkg->cur_token == TK_TRY
    ast_step(pkg);

    ast_parse_block_statement_with_braces(ast, pkg, try_statement);

    while (pkg->cur_token == TK_CATCH) {
        ast_step(pkg);

        lgx_ast_node_t* catch_statement = ast_node_new(ast, pkg, 2);
        catch_statement->type = CATCH_STATEMENT;
        ast_node_append_child(try_statement, catch_statement);

        if (pkg->cur_token != '(') {
            ast_error(ast, pkg, "'(' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
            return;
        }
        ast_step(pkg);

        ast_parse_decl_parameter(ast, pkg, catch_statement);
        if (catch_statement->child[0]->children != 1) {
            ast_error(ast, pkg, "there must be one and only one parameter in catch statements\n");
            return;
        }

        if (pkg->cur_token != ')') {
            ast_error(ast, pkg, "')' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
            return;
        }
        ast_step(pkg);

        ast_parse_block_statement_with_braces(ast, pkg, catch_statement);
    }

    if (pkg->cur_token == TK_FINALLY) {
        ast_step(pkg);

        lgx_ast_node_t* finally_statement = ast_node_new(ast, pkg, 1);
        finally_statement->type = FINALLY_STATEMENT;
        ast_node_append_child(try_statement, finally_statement);

        ast_parse_block_statement_with_braces(ast, pkg, finally_statement);
    }
}

void ast_parse_throw_statement(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_t* throw_statement = ast_node_new(ast, pkg, 1);
    throw_statement->type = THROW_STATEMENT;
    ast_node_append_child(parent, throw_statement);

    // pkg->cur_token == TK_THROW
    ast_step(pkg);

    ast_parse_expression(ast, pkg, throw_statement);
    if (throw_statement->children == 0) {
        ast_error(ast, pkg, "expression expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
}

void ast_parse_return_statement(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_t* return_statement = ast_node_new(ast, pkg, 1);
    return_statement->type = RETURN_STATEMENT;;
    ast_node_append_child(parent, return_statement);

    // pkg->cur_token == TK_RETURN
    ast_step(pkg);

    ast_parse_expression(ast, pkg, return_statement);
}

void ast_parse_echo_statement(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_t* echo_statement = ast_node_new(ast, pkg, 1);
    echo_statement->type = ECHO_STATEMENT;;
    ast_node_append_child(parent, echo_statement);

    // pkg->cur_token == TK_ECHO
    ast_step(pkg);
    
    ast_parse_expression(ast, pkg, echo_statement);
    if (echo_statement->children == 0) {
        ast_error(ast, pkg, "expression expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
}

void ast_parse_expression_statement(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_t* assign_statement = ast_node_new(ast, pkg, 1);
    assign_statement->type = EXPRESSION_STATEMENT;
    ast_node_append_child(parent, assign_statement);
    
    ast_parse_expression(ast, pkg, assign_statement);
}

void ast_parse_modifier(lgx_ast_t* ast, lgx_package_t *pkg, lgx_modifier_t *modifier) {
    modifier->access = P_PACKAGE;
    modifier->is_const = 0;
    modifier->is_async = 0;
    modifier->is_static = 0;
    modifier->is_abstract = 0;

    while (1) {
        if (pkg->cur_token == TK_PUBLIC) {
            if (modifier->access) {
                ast_error(ast, pkg, "access modifier already seen before `%.*s`\n", pkg->cur_length, pkg->cur_start);
                return;
            } else {
                modifier->access = P_PUBLIC;
                ast_step(pkg);
            }
        } else if (pkg->cur_token == TK_PROTECTED) {
            if (modifier->access) {
                ast_error(ast, pkg, "access modifier already seen before `%.*s`\n", pkg->cur_length, pkg->cur_start);
                return;
            } else {
                modifier->access = P_PROTECTED;
                ast_step(pkg);
            }
        } else if (pkg->cur_token == TK_PRIVATE) {
            if (modifier->access) {
                ast_error(ast, pkg, "access modifier already seen before `%.*s`\n", pkg->cur_length, pkg->cur_start);
                return;
            } else {
                modifier->access = P_PRIVATE;
                ast_step(pkg);
            }
        } else if (pkg->cur_token == TK_STATIC) {
            if (modifier->is_static) {
                ast_error(ast, pkg, "static modifier already seen before `%.*s`\n", pkg->cur_length, pkg->cur_start);
                return;
            } else {
                modifier->is_static = 1;
                ast_step(pkg);
            }
        } else if (pkg->cur_token == TK_CONST) {
            if (modifier->is_const) {
                ast_error(ast, pkg, "const modifier already seen before `%.*s`\n", pkg->cur_length, pkg->cur_start);
                return;
            } else {
                modifier->is_const = 1;
                ast_step(pkg);
            }
        } else if (pkg->cur_token == TK_ASYNC) {
            if (modifier->is_async) {
                ast_error(ast, pkg, "async modifier already seen before `%.*s`\n", pkg->cur_length, pkg->cur_start);
                return;
            } else {
                modifier->is_async = 1;
                ast_step(pkg);
            }
        } else {
            break;
        }
    }
}

void ast_parse_import(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    // pkg->cur_token == TK_IMPORT
    ast_step(pkg);

    if (pkg->cur_token == TK_STRING) {
        char path[PATH_MAX];
        memcpy(path, pkg->cur_start + 1, pkg->cur_length - 2);
        path[pkg->cur_length - 2] = '\0';

        lgx_ast_import(ast, pkg, parent, path);

        ast_step(pkg);
    } else {
        ast_error(ast, pkg, "string expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
}

void ast_parse_package(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    // pkg->cur_token == TK_PACKAGE
    ast_step(pkg);

    // TODO
}

void ast_parse_statement(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    while(1) {
        switch (pkg->cur_token) {
            case ';':
                // null statement
                ast_step(pkg);
                break;
            case '}': case TK_EOF: case TK_CASE: case TK_DEFAULT:
                return;
            case TK_IF:
                ast_parse_if_statement(ast, pkg, parent);
                continue;
            case TK_FOR:
                ast_parse_for_statement(ast, pkg, parent);
                continue;
            case TK_WHILE:
                ast_parse_while_statement(ast, pkg, parent);
                continue;
            case TK_DO:
                ast_parse_do_statement(ast, pkg, parent);
                break;
            case TK_BREAK:
                ast_parse_break_statement(ast, pkg, parent);
                break;
            case TK_CONTINUE:
                ast_parse_continue_statement(ast, pkg, parent);
                break;
            case TK_SWITCH:
                ast_parse_switch_statement(ast, pkg, parent);
                continue;
            case TK_RETURN:
                ast_parse_return_statement(ast, pkg, parent);
                break;
            case TK_ECHO:
                ast_parse_echo_statement(ast, pkg, parent);
                break;
            case TK_TRY:
                ast_parse_try_statement(ast, pkg, parent);
                continue;
            case TK_THROW:
                ast_parse_throw_statement(ast, pkg, parent);
                break;
            case TK_AUTO: case TK_INT: case TK_FLOAT:
            case TK_BOOL: case TK_STR: case TK_ARR: {
                lgx_modifier_t modifier;
                ast_parse_modifier(ast, pkg, &modifier);
                ast_parse_variable_declaration(ast, pkg, parent, &modifier);
                break;
            }
            case TK_ID: {
                if (ast_is_class(ast, pkg)) {
                    lgx_modifier_t modifier;
                    ast_parse_modifier(ast, pkg, &modifier);
                    ast_parse_variable_declaration(ast, pkg, parent, &modifier);
                } else {
                    ast_parse_expression_statement(ast, pkg, parent);
                }
                break;
            }
            case TK_FUNCTION:
            case TK_ASYNC:
            case TK_STATIC:
            case TK_CONST:
            case TK_PUBLIC:
            case TK_PROTECTED:
            case TK_PRIVATE: {
                lgx_modifier_t modifier;
                ast_parse_modifier(ast, pkg, &modifier);

                if (modifier.access) {
                    ast_error(ast, pkg, "unexpected access modifier before `%.*s`\n", pkg->cur_length, pkg->cur_start);
                    return;
                }

                if (pkg->cur_token == TK_FUNCTION) {
                    if (modifier.is_const) {
                        ast_error(ast, pkg, "function could not be defined as const\n");
                        return;
                    }
                    if (parent->parent) {
                        ast_error(ast, pkg, "functions can only be defined in top-level scope\n");
                        return;
                    }
                    ast_parse_function_declaration(ast, pkg, parent, &modifier);
                    continue;
                } else {
                    if (modifier.is_async) {
                        ast_error(ast, pkg, "variable could not be defined as async\n");
                        return;
                    }
                    ast_parse_variable_declaration(ast, pkg, parent, &modifier);
                    break;
                }
            }
            case TK_CLASS:
                ast_parse_class_declaration(ast, pkg, parent);
                continue;
            case TK_INTERFACE:
                ast_parse_interface_declaration(ast, pkg, parent);
                continue;
            case TK_IMPORT:
                ast_parse_import(ast, pkg, parent);
                break;
            case TK_PACKAGE:
                ast_parse_package(ast, pkg, parent);
                break;
            default:
                ast_parse_expression_statement(ast, pkg, parent);
        }

        if (pkg->cur_token != ';') {
            // 语句应当以分号结尾。以花括号结尾的语句可以省略分号。
            ast_error(ast, pkg, "';' expected before '%.*s'\n", pkg->cur_length, pkg->cur_start);
            return;
        }
        ast_step(pkg);
    }
}

void ast_parse_variable_declaration(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent, lgx_modifier_t *modifier) {
    lgx_ast_node_t* variable_declaration;

    int var_type = pkg->cur_token;
    char *type_str = pkg->cur_start;
    int type_len = pkg->cur_length;
    ast_step(pkg);

    while (1) {
        variable_declaration = ast_node_new(ast, pkg, 2);
        variable_declaration->type = VARIABLE_DECLARATION;
        variable_declaration->u.type.type = var_type;
        variable_declaration->u.type.obj_name = lgx_str_new_ref(type_str, type_len);
        ast_node_append_child(parent, variable_declaration);

        if (pkg->cur_token != TK_ID) {
            ast_error(ast, pkg, "`%.*s` is not a <identifier>\n", pkg->cur_length, pkg->cur_start);
            return;
        } else {
            //printf("[Info] [Line:%d] variable `%.*s` declared\n", pkg->cur_length, pkg->cur_start);
            ast_parse_id_token(ast, pkg, variable_declaration);
        }
        
        if (pkg->cur_token == '=') {
            ast_step(pkg);
            
            //printf("[Info] [Line:%d] variable initialized\n");
            ast_parse_expression(ast, pkg, variable_declaration);
            if (variable_declaration->children == 1) {
                ast_error(ast, pkg, "expression expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
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
            t->u.c.modifier = *modifier;
        } else {
            ast_error(ast, pkg, "identifier `%.*s` has already been declared\n", s.length, s.buffer);
            return;
        }

        switch (pkg->cur_token) {
            case ',':
                ast_step(pkg);
                break;
            default:
                return;
        }
    }
}

void ast_parse_function_declaration(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent, lgx_modifier_t *modifier) {
    lgx_ast_node_t* function_declaration = ast_node_new(ast, pkg, 3);
    function_declaration->type = FUNCTION_DECLARATION;
    ast_node_append_child(parent, function_declaration);

    // pkg->cur_token == TK_FUNCTION
    ast_step(pkg);

    if (pkg->cur_token != TK_ID) {
        ast_error(ast, pkg, "`<identifier>` expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_parse_id_token(ast, pkg, function_declaration);

    // 创建函数加入作用域
    lgx_str_t s;
    lgx_ast_node_token_t *n = (lgx_ast_node_token_t *)function_declaration->child[0];
    s.buffer = n->tk_start;
    s.length = n->tk_length;

    lgx_val_t *f;
    if ((f = lgx_scope_val_add(function_declaration, &s))) {
        f->type = T_FUNCTION;
        f->u.c.used = 1;
        f->u.c.modifier = *modifier;
    } else {
        ast_error(ast, pkg, "identifier `%.*s` has already been declared\n", n->tk_length, n->tk_start);
        return;
    }

    if (pkg->cur_token != '(') {
        ast_error(ast, pkg, "'(' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_step(pkg);

    ast_parse_decl_parameter(ast, pkg, function_declaration);

    if (pkg->cur_token != ')') {
        ast_error(ast, pkg, "')' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_step(pkg);

    // 根据参数数量创建函数
    f->v.fun = lgx_fun_new(function_declaration->child[1]->children);
    f->v.fun->modifier = *modifier;
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
    switch (pkg->cur_token) {
        case TK_AUTO: case TK_INT: case TK_FLOAT:
        case TK_BOOL: case TK_STR: case TK_ARR: {
            function_declaration->u.type.type = pkg->cur_token;
            ast_step(pkg);
            break;
        }
        case TK_ID:
            if (ast_is_class(ast, pkg)) {
                function_declaration->u.type.type = pkg->cur_token;
                function_declaration->u.type.obj_name = lgx_str_new_ref(pkg->cur_start, pkg->cur_length);
            } else {
                ast_error(ast, pkg, "type declaration expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
                return;
            }
            ast_step(pkg);
            break;
        default:
            function_declaration->u.type.type = TK_AUTO;
    }
    ast_set_variable_type(&f->v.fun->ret, function_declaration);

    if (parent->type == BLOCK_STATEMENT) {
        ast_parse_block_statement_with_braces(ast, pkg, function_declaration);
    } else if (parent->type == METHOD_DECLARATION) {
        if (parent->u.modifier.is_abstract) {
            if (pkg->cur_token == ';') {
                ast_step(pkg);
            } else {
                ast_error(ast, pkg, "';' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
                return;
            }
        } else {
            ast_parse_block_statement_with_braces(ast, pkg, function_declaration);
        }
    } else {
        ast_error(ast, pkg, "invalid function declaration before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
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

void ast_parse_class_declaration(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_t* class_declaration = ast_node_new(ast, pkg, 2);
    class_declaration->type = CLASS_DECLARATION;
    ast_node_append_child(parent, class_declaration);

    // pkg->cur_token == TK_CLASS
    ast_step(pkg);

    if (pkg->cur_token != TK_ID) {
        ast_error(ast, pkg, "`<identifier>` expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_parse_id_token(ast, pkg, class_declaration);

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
        ast_error(ast, pkg, "identifier `%.*s` has already been declared\n", n->tk_length, n->tk_start);
        return;
    }

    // 处理继承。只支持单继承。
    if (pkg->cur_token == TK_EXTENDS) {
        ast_step(pkg);
        if (pkg->cur_token != TK_ID) {
            ast_error(ast, pkg, "`identifier` expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
            return;
        }

        s.buffer = pkg->cur_start;
        s.length = pkg->cur_length;

        lgx_val_t *v = lgx_scope_global_val_get(ast->root, &s);
        if (!v || v->type != T_OBJECT || v->v.obj->is_interface) {
            ast_error(ast, pkg, "`%.*s` is not a class\n", pkg->cur_length, pkg->cur_start);
            return;
        }

        f->v.obj->parent = v->v.obj;

        ast_step(pkg);
    }

    if (pkg->cur_token == TK_IMPLEMENTS) {
        ast_step(pkg);

        while (1) {
            if (pkg->cur_token != TK_ID) {
                ast_error(ast, pkg, "`identifier` expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
                return;
            }

            s.buffer = pkg->cur_start;
            s.length = pkg->cur_length;

            lgx_val_t *v = lgx_scope_global_val_get(ast->root, &s);
            if (!v || v->type != T_OBJECT || !v->v.obj->is_interface) {
                ast_error(ast, pkg, "`%.*s` is not a interface\n", pkg->cur_length, pkg->cur_start);
                return;
            }

            // TODO 处理 interface

            ast_step(pkg);

            if (pkg->cur_token != ',') {
                break;
            }
            ast_step(pkg);
        }
    }

    if (pkg->cur_token != '{') {
        ast_error(ast, pkg, "'{' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_step(pkg);

    lgx_ast_node_t* block_statement = ast_node_new(ast, pkg, 128);
    block_statement->type = BLOCK_STATEMENT;
    ast_node_append_child(class_declaration, block_statement);

    // 创建新的作用域
    block_statement->u.symbols = lgx_hash_new(32);

    int endloop = 0;
    while (!endloop) {
        lgx_modifier_t modifier;
        ast_parse_modifier(ast, pkg, &modifier);

        if (pkg->cur_token == TK_FUNCTION) {
            if (modifier.is_const) {
                ast_error(ast, pkg, "method could not be defined as const\n");
                return;
            }

            lgx_ast_node_t* method_declaration = ast_node_new(ast, pkg, 1);
            method_declaration->type = METHOD_DECLARATION;
            method_declaration->u.modifier = modifier;
            ast_node_append_child(block_statement, method_declaration);

            ast_parse_function_declaration(ast, pkg, method_declaration, &modifier);
            if (ast->err_no) {
                return;
            }

            lgx_hash_node_t n;
            lgx_ast_node_token_t *method_name = (lgx_ast_node_token_t *)method_declaration->child[0]->child[0];
            n.k.type = T_STRING;
            n.k.v.str = lgx_str_new_ref(method_name->tk_start, method_name->tk_length);
            n.v = *lgx_scope_local_val_get(block_statement, n.k.v.str);

            // TODO 检查继承关系
            lgx_obj_add_method(f->v.obj, &n);
        } else {
            if (modifier.is_async) {
                ast_error(ast, pkg, "propery could not be defined as async\n");
                return;
            }

            switch (pkg->cur_token) {
                case TK_AUTO: case TK_INT: case TK_FLOAT:
                case TK_BOOL: case TK_STR: case TK_ARR: case TK_ID: {
                    if (pkg->cur_token == TK_ID) {
                        if (!ast_is_class(ast, pkg)) {
                            ast_error(ast, pkg, "type declaration expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
                            return;
                        }
                    }

                    lgx_ast_node_t* property_declaration = ast_node_new(ast, pkg, 1);
                    property_declaration->type = PROPERTY_DECLARATION;
                    property_declaration->u.modifier = modifier;
                    ast_node_append_child(block_statement, property_declaration);

                    ast_parse_variable_declaration(ast, pkg, property_declaration, &modifier);
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

                    // TODO 检查继承关系
                    lgx_obj_add_property(f->v.obj, &n);

                    if (pkg->cur_token != ';') {
                        ast_error(ast, pkg, "';' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
                        return;
                    }
                    ast_step(pkg);
                    break;
                }
                default:
                    endloop = 1;
            }
        }
    }

    if (pkg->cur_token != '}') {
        ast_error(ast, pkg, "'}' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_step(pkg);

    // TODO 检查 implements
}

void ast_parse_interface_declaration(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    lgx_ast_node_t* interface_declaration = ast_node_new(ast, pkg, 128);
    interface_declaration->type = INTERFACE_DECLARATION;
    ast_node_append_child(parent, interface_declaration);

    // pkg->cur_token == TK_INTERFACE
    ast_step(pkg);

    if (pkg->cur_token != TK_ID) {
        ast_error(ast, pkg, "`<identifier>` expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_parse_id_token(ast, pkg, interface_declaration);

    // 创建接口加入作用域
    lgx_str_t s;
    lgx_ast_node_token_t *n = (lgx_ast_node_token_t *)interface_declaration->child[0];
    s.buffer = n->tk_start;
    s.length = n->tk_length;

    lgx_val_t *f;
    if ((f = lgx_scope_val_add(interface_declaration, &s))) {
        f->type = T_OBJECT;
        f->v.obj = lgx_obj_create(&s);
        f->u.c.used = 1;
        f->v.obj->is_interface = 1;
    } else {
        ast_error(ast, pkg, "identifier `%.*s` has already been declared\n", n->tk_length, n->tk_start);
        return;
    }

    if (pkg->cur_token != '{') {
        ast_error(ast, pkg, "'{' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_step(pkg);

    lgx_ast_node_t* block_statement = ast_node_new(ast, pkg, 128);
    block_statement->type = BLOCK_STATEMENT;
    ast_node_append_child(interface_declaration, block_statement);

    // 创建新的作用域
    block_statement->u.symbols = lgx_hash_new(32);

    int endloop = 0;
    while (!endloop) {
        lgx_modifier_t modifier;
        ast_parse_modifier(ast, pkg, &modifier);

        // interface 中的方法一定是 abstract 的
        modifier.is_abstract = 1;

        if (pkg->cur_token == TK_FUNCTION) {
            if (modifier.is_const) {
                ast_error(ast, pkg, "method could not be defined as const\n");
                return;
            } else if (modifier.is_static) {
                ast_error(ast, pkg, "static method is not allowed in interface\n");
                return;
            } else if (modifier.access != P_PUBLIC) {
                ast_error(ast, pkg, "method must be public in interface\n");
                return;
            }

            lgx_ast_node_t* method_declaration = ast_node_new(ast, pkg, 1);
            method_declaration->type = METHOD_DECLARATION;
            method_declaration->u.modifier = modifier;
            ast_node_append_child(block_statement, method_declaration);

            ast_parse_function_declaration(ast, pkg, method_declaration, &modifier);
            if (ast->err_no) {
                return;
            }

            lgx_hash_node_t n;
            lgx_ast_node_token_t *method_name = (lgx_ast_node_token_t *)method_declaration->child[0]->child[0];
            n.k.type = T_STRING;
            n.k.v.str = lgx_str_new_ref(method_name->tk_start, method_name->tk_length);
            n.v = *lgx_scope_local_val_get(block_statement, n.k.v.str);

            lgx_obj_add_method(f->v.obj, &n);
        } else {
            endloop = 1;
        }
    }

    if (pkg->cur_token != '}') {
        ast_error(ast, pkg, "'}' expected before `%.*s`\n", pkg->cur_length, pkg->cur_start);
        return;
    }
    ast_step(pkg);
}

int ast_lex_init(lgx_ast_t *ast, lgx_package_t *pkg, char *file) {
    FILE* fp;
    fp = fopen(file, "rb");
    if (!fp) {
        ast_error(ast, pkg, "can't open file `%s`\n", file);
        return 1;
    }

    /* 读取绝对路径 */
    char resolved_path[PATH_MAX];
    if (realpath(file, resolved_path) == NULL) {
        ast_error(ast, pkg, "can't get realpath of file `%s`\n", file);
        return 1;
    }
    pkg->lex.file = strdup(resolved_path);

    /* 定位到文件末尾 */
    fseek(fp, 0L, SEEK_END);
    /* 得到文件大小 */
    int flen = ftell(fp);

    /* 根据文件大小动态分配内存空间 */
    pkg->lex.source = (char*)xmalloc(flen);
    if (pkg->lex.source == NULL) {
        fclose(fp);
        ast_error(ast, pkg, "malloc %d bytes failed\n", flen);
        return 1;
    }

    /* 定位到文件开头 */
    fseek(fp, 0L, SEEK_SET);
    /* 一次性读取全部文件内容 */
    pkg->lex.length = fread(pkg->lex.source, 1, flen, fp);

    fclose(fp);

    if (pkg->lex.length != flen) {
        ast_error(ast, pkg, "read file `%s` failed\n", file);
        return 1;
    }

    return 0;
}

void lgx_ast_parse(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent) {
    // TODO 解析 package 语句
    //ast_parse_package(ast);

    ast_parse_statement(ast, pkg, parent);

    switch (pkg->cur_token) {
        case TK_EOF:
            break;
        default:
            ast_error(ast, pkg, "unexpected `%.*s`\n", pkg->cur_length, pkg->cur_start);
    }
}

int lgx_ast_parser(lgx_ast_t* ast, char *file) {
    memset(&ast->imported, 0, sizeof(lgx_package_t));
    wbt_list_init(&ast->imported.head);

    // 读取源文件
    if (ast_lex_init(ast, &ast->imported, file) != 0) {
        return 1;
    }

    ast->root = ast_node_new(ast, &ast->imported, 128);
    ast->root->type = BLOCK_STATEMENT;

    // 全局作用域
    ast->root->u.symbols = lgx_hash_new(32);

    // 扩展接口
    lgx_ext_load_symbols(ast->root->u.symbols);

    ast_step(&ast->imported);
    lgx_ast_parse(ast, &ast->imported, ast->root);

    ast->imported.finished = 1;

    return 0;
}

int lgx_ast_import(lgx_ast_t* ast, lgx_package_t *pkg, lgx_ast_node_t* parent, char *file) {
    /* 读取绝对路径 */
    char resolved_path[PATH_MAX];
    if (realpath(file, resolved_path) == NULL) {
        ast_error(ast, pkg, "can't get realpath of file `%s`\n", file);
        return 1;
    }

    // 不能 import 入口文件
    if (strcmp(resolved_path, ast->imported.lex.file) == 0) {
        ast_error(ast, pkg, "can't import entry file `%s`\n", file);
        return 1;
    }

    lgx_package_t *package;
    wbt_list_for_each_entry(package, lgx_package_t, &ast->imported.head, head) {
        if (strcmp(resolved_path, package->lex.file) == 0) {
            if (package->finished) {
                // 已经加载过了，不需要重复加载
                wbt_debug("file `%s` already imported", resolved_path);
                return 0;
            } else {
                // 循环 import 是被禁止的
                ast_error(ast, pkg, "import cycle not allowed for file `%s`\n", file);
                return 1;
            }
        }
    }

    wbt_debug("import file `%s`", resolved_path);

    // 新的 import package
    lgx_package_t *import_pkg = xcalloc(1, sizeof(lgx_package_t));
    if (import_pkg == NULL) {
        return 1;
    }
    wbt_list_init(&import_pkg->head);

    if (ast_lex_init(ast, import_pkg, file) != 0) {
        return 1;
    }

    wbt_list_add_tail(&import_pkg->head, &ast->imported.head);

    ast_step(import_pkg);
    lgx_ast_parse(ast, import_pkg, parent);

    import_pkg->finished = 1;

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
            if (node->child[2]) {
                lgx_ast_print(node->child[2], indent+2);
            }
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