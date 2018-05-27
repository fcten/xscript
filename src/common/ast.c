#include <stdio.h>
#include <stdlib.h>

#include "val.h"
#include "lex.h"
#include "ast.h"

void ast_parse_statement(lgx_ast_t* ast, lgx_ast_node_t* parent);

void ast_parse_bsc_expression(lgx_ast_t* ast, lgx_ast_node_t* parent);
void ast_parse_pri_expression(lgx_ast_t* ast, lgx_ast_node_t* parent);
void ast_parse_suf_expression(lgx_ast_t* ast, lgx_ast_node_t* parent);
void ast_parse_sub_expression(lgx_ast_t* ast, lgx_ast_node_t* parent, int precedence);
void ast_parse_expression(lgx_ast_t* ast, lgx_ast_node_t* parent);

void ast_parse_variable_declaration(lgx_ast_t* ast, lgx_ast_node_t* parent);
void ast_parse_function_declaration(lgx_ast_t* ast, lgx_ast_node_t* parent);

lgx_ast_node_t* ast_node_new(int n) {
    lgx_ast_node_t* node = calloc(1, sizeof(lgx_ast_node_t) + n * sizeof(lgx_ast_node_t*));
    // TODO ERROR CHECK
    node->children = 0;
    node->size = n;
    return node;
}

lgx_ast_node_token_t* ast_node_token_new(lgx_ast_t* ast) {
    lgx_ast_node_token_t* node = calloc(1, sizeof(lgx_ast_node_token_t));

    node->tk_start = ast->cur_start;
    node->tk_length = ast->cur_length;
    
    return node;
}

lgx_ast_node_t* ast_node_append_child(lgx_ast_node_t* parent, lgx_ast_node_t* child) {
    if (parent->children >= parent->size) {
        parent = realloc(parent, sizeof(lgx_ast_node_t) + 2 * parent->size * sizeof(lgx_ast_node_t*));
        // TODO ERROR CHECK
        parent->size *= 2;
    }
    
    parent->child[parent->children++] = child;
    
    return parent;
}

void ast_error() {

}

int ast_next(lgx_ast_t* ast) {
    unsigned int token;
    while(1) {
        token = lgx_lex(&ast->lex);
        switch (token) {
            case TK_SPACE:
            case TK_COMMENT:
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

// pri_expr -> ID | '(' sub_expr ')'
void ast_parse_pri_expression(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_token_t* id;
    switch (ast->cur_token) {
        case '(':
            ast_step(ast);

            ast_parse_expression(ast, parent);

            if (ast->cur_token != ')') {
                printf("[Error] [Line:%d] ')' expected\n", ast->cur_line);
                return;
            }
            ast_step(ast);
            break;
        case TK_ID:
            // todo
            id = ast_node_token_new(ast);
            id->type = IDENTIFIER_TOKEN;
            ast_node_append_child(parent, (lgx_ast_node_t*)id);

            ast_step(ast);
            break;
        default:
            return;
    }
}

// suf_expr -> pri_expr { '[' sub_expr ']' | '(' sub_expr ')' }
void ast_parse_suf_expression(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    ast_parse_pri_expression(ast, parent);

    switch (ast->cur_token) {
        case '[':
        case '(':
            break;
    }
}

// bsc_expr -> NUMBER | STRING | suf_expr
void ast_parse_bsc_expression(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_token_t* id;
    switch (ast->cur_token) {
        case TK_NUMBER:
            id = ast_node_token_new(ast);
            id->type = NUMBER_TOKEN;
            ast_node_append_child(parent, (lgx_ast_node_t*)id);

            ast_step(ast);
            break;
        case TK_STRING:
            id = ast_node_token_new(ast);
            id->type = STRING_TOKEN;
            ast_node_append_child(parent, (lgx_ast_node_t*)id);

            ast_step(ast);
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
            unary_expression = ast_node_new(1);
            unary_expression->type = UNARY_EXPRESSION;
            unary_expression->op = ast->cur_token;
            ast_node_append_child(parent, unary_expression);

            ast_step(ast);

            ast_parse_sub_expression(ast, unary_expression, 2);
            break;
        default:
            ast_parse_bsc_expression(ast, parent);
    }

    int p = ast_operator_precedence(ast->cur_token);
    lgx_ast_node_t* binary_expression;
    while (p >= 0 && p < precedence) {
        binary_expression = ast_node_new(2);
        binary_expression->type = BINARY_EXPRESSION;
        binary_expression->op = ast->cur_token;
        ast_node_append_child(binary_expression, parent->child[parent->children-1]);
        parent->child[parent->children-1] = binary_expression;

        ast_step(ast);

        ast_parse_sub_expression(ast, binary_expression, p);

        p = ast_operator_precedence(ast->cur_token);
    }
}

void ast_parse_expression(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    ast_parse_sub_expression(ast, parent, 15);
}

void ast_parse_block_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* block_statement = ast_node_new(128);
    block_statement->type = BLOCK_STATEMENT;
    ast_node_append_child(parent, block_statement);

    if (ast->cur_token != '{') {
        printf("[Error] [Line:%d] '{' expected\n", ast->cur_line);
        return;
    }
    ast_step(ast);
    
    ast_parse_statement(ast, block_statement);

    if (ast->cur_token != '}') {
        printf("[Error] [Line:%d] '}' expected\n", ast->cur_line);
        return;
    }
    ast_step(ast);
}

void ast_parse_if_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* if_statement = ast_node_new(3);
    if_statement->type = IF_STATEMENT;
    ast_node_append_child(parent, if_statement);

    // ast->cur_token == TK_IF
    ast_step(ast);
    
    if (ast->cur_token != '(') {
        printf("[Error] [Line:%d] '(' expected\n", ast->cur_line);
        return;
    }
    ast_step(ast);
    
    ast_parse_expression(ast, if_statement);

    if (ast->cur_token != ')') {
        printf("[Error] [Line:%d] ')' expected\n", ast->cur_line);
        return;
    }
    ast_step(ast);
    
    ast_parse_block_statement(ast, if_statement);
    
    if (ast->cur_token != TK_ELSE) {
        return;
    }
    ast_step(ast);
    
    if_statement->type = IF_ELSE_STATEMENT;
    ast_parse_block_statement(ast, if_statement);
}

void ast_parse_for_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    ast_step(ast);
}

void ast_parse_while_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* while_statement = ast_node_new(2);
    while_statement->type = WHILE_STATEMENT;
    ast_node_append_child(parent, while_statement);

    // ast->cur_token == TK_WHILE
    ast_step(ast);

    if (ast->cur_token != '(') {
        printf("[Error] [Line:%d] '(' expected, not `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);
    
    ast_parse_expression(ast, while_statement);

    if (ast->cur_token != ')') {
        printf("[Error] [Line:%d] ')' expected, not `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);
    
    ast_parse_block_statement(ast, while_statement);
}

void ast_parse_do_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* do_statement = ast_node_new(2);
    do_statement->type = DO_WHILE_STATEMENT;
    ast_node_append_child(parent, do_statement);

    // ast->cur_token == TK_DO
    ast_step(ast);
    
    ast_parse_block_statement(ast, do_statement);

    if (ast->cur_token != TK_WHILE) {
        printf("[Error] [Line:%d] 'while' expected, not `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);
    
    if (ast->cur_token != '(') {
        printf("[Error] [Line:%d] '(' expected, not `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);
    
    ast_parse_expression(ast, do_statement);

    if (ast->cur_token != ')') {
        printf("[Error] [Line:%d] ')' expected, not `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);
}

void ast_parse_break_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* break_statement = ast_node_new(0);
    break_statement->type = BREAK_STATEMENT;
    ast_node_append_child(parent, break_statement);

    // ast->cur_token == TK_BREAK
    ast_step(ast);
}

void ast_parse_continue_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* continue_statement = ast_node_new(0);
    continue_statement->type = CONTINUE_STATEMENT;
    ast_node_append_child(parent, continue_statement);

    // ast->cur_token == TK_CONTINUE
    ast_step(ast);
}

void ast_parse_switch_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    ast_step(ast);
}

void ast_parse_return_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* return_statement = ast_node_new(1);
    return_statement->type = RETURN_STATEMENT;;
    ast_node_append_child(parent, return_statement);

    // ast->cur_token == TK_RETURN
    ast_step(ast);
    
    switch (ast->cur_token) {
        case ';':
        case TK_EOL:
            ast_step(ast);
        default:
            ast_parse_expression(ast, return_statement);
    }
}

void ast_parse_assign_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* assign_statement = ast_node_new(2);
    assign_statement->type = ASSIGNMENT_STATEMENT;
    ast_node_append_child(parent, assign_statement);
    
    // ast->cur_token == TK_ID
    lgx_ast_node_token_t* id = ast_node_token_new(ast);
    id->type = IDENTIFIER_TOKEN;
    ast_node_append_child(assign_statement, (lgx_ast_node_t*)id);
    ast_step(ast);
    
    if (ast->cur_token != '=') {
        printf("[Error] [Line:%d] '=' expected, not `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
        return;
    }
    ast_step(ast);
    
    ast_parse_expression(ast, assign_statement);

    if (ast->cur_token != ';' && ast->cur_token != TK_EOL) {
        printf("[Error] [Line:%d] ';' expected\n", ast->cur_line);
        return;
    }
    ast_step(ast);
}

void ast_parse_statement(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    while(1) {
        switch (ast->cur_token) {
            case ';':
            case TK_EOL:
                // null statement
                ast_step(ast);
                break;
            case TK_IF:
                ast_parse_if_statement(ast, parent);
                break;
            case TK_FOR:
                ast_parse_for_statement(ast, parent);
                break;
            case TK_WHILE:
                ast_parse_while_statement(ast, parent);
                break;
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
                break;
            case TK_RETURN:
                ast_parse_return_statement(ast, parent);
                break;
            case TK_ID:
                ast_parse_assign_statement(ast, parent);
                break;
            case TK_AUTO:
                ast_parse_variable_declaration(ast, parent);
                break;
            case TK_FUNCTION:
                ast_parse_function_declaration(ast, parent);
                break;
            default:
                return;
        }
    }
}

void ast_parse_variable_declaration(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* variable_declaration;

    // ast->cur_token == TK_VAR
    ast_step(ast);

    while (1) {
        variable_declaration = ast_node_new(2);
        variable_declaration->type = VARIABLE_DECLARATION;
        ast_node_append_child(parent, variable_declaration);

        if (ast->cur_token != TK_ID) {
            printf("[Error] [Line:%d] `%.*s` is not a <identifier>\n", ast->cur_line, ast->cur_length, ast->cur_start);
            return;
        } else {
            printf("[Info] [Line:%d] variable `%.*s` declared\n", ast->cur_line, ast->cur_length, ast->cur_start);
        }
        ast_step(ast);
        
        if (ast->cur_token == '=') {
            ast_step(ast);
            
            printf("[Info] [Line:%d] variable initialized\n", ast->cur_line);
            ast_parse_expression(ast, variable_declaration);
        }

        switch (ast->cur_token) {
            case ',':
                ast_step(ast);
                break;
            case ';':
            case TK_EOL:
                ast_step(ast);
                return;
            default:
                printf("[Error] [Line:%d] ,' or ';' expected\n", ast->cur_line);
                return;
        }
    }
}

void ast_parse_function_declaration(lgx_ast_t* ast, lgx_ast_node_t* parent) {
    lgx_ast_node_t* function_declaration = ast_node_new(3);
    function_declaration->type = FUNCTION_DECLARATION;
    ast_node_append_child(parent, function_declaration);

    // ast->cur_token == TK_FUNCTION
    ast_step(ast);

    if (ast->cur_token == TK_ID) {
        printf("[Info] [Line:%d] function `%.*s` declared\n", ast->cur_line, ast->cur_length, ast->cur_start);
        ast_step(ast);
    } else if (ast->cur_token == '(') {
        printf("[Info] [Line:%d] anonymous function declared\n", ast->cur_line);
    }
    
    if (ast->cur_token != '(') {
        printf("[Error] [Line:%d] '(' expected\n", ast->cur_line);
        return;
    }
    ast_step(ast);

    while(ast->cur_token != ')') {      
        switch (ast->cur_token) {
            case TK_ID:
                printf("[Info] [Line:%d] param `%.*s` declared\n", ast->cur_line, ast->cur_length, ast->cur_start);
                ast_step(ast);
                break;
            default:
                printf("[Error] [Line:%d] `%.*s` is not a <identifier>\n", ast->cur_line, ast->cur_length, ast->cur_start);
                return;
        }
        
        switch (ast->cur_token) {
            case ',':
                ast_step(ast);
                break;
            case '=':
                ast_step(ast);

                ast_parse_expression(ast, function_declaration);

                printf("[Info] [Line:%d] param initialized\n", ast->cur_line);
                break;
            case ')':
                break;
            default:
                printf("[Error] [Line:%d] '=', ',' or ')' expected\n", ast->cur_line);
                return;
        }
    }
    ast_step(ast);
    
    ast_parse_block_statement(ast, function_declaration);
}

int lgx_ast_parser(lgx_ast_t* ast) {
    ast->root = ast_node_new(128);
    ast->root->type = BLOCK_STATEMENT;
    
    ast_step(ast);

    ast_parse_statement(ast, ast->root);

    switch (ast->cur_token) {
        case TK_EOF:
            break;
        default:
            printf("[Error] [Line:%d] unexpected `%.*s`\n", ast->cur_line, ast->cur_length, ast->cur_start);
    }

    return 0;
}

int ast_calculate(lgx_ast_node_t* node) {
    int a, b;
    switch(node->type) {
        // Expression
        case BINARY_EXPRESSION:
            a = ast_calculate(node->child[0]);
            b = ast_calculate(node->child[1]);
            switch (node->op) {
                case '*':
                    return a * b;
                case '/':
                    return a / b;
                case '%':
                    return a % b;
                case '+':
                    return a + b;
                case '-':
                    return a - b;
                case TK_SHL:
                    return a << b;
                case TK_SHR:
                    return a >> b;
                case '>':
                    return a > b;
                case '<':
                    return a < b;
                case TK_GE:
                    return a >= b;
                case TK_LE:
                    return a <= b;
                case TK_EQ:
                    return a == b;
                case TK_NE:
                    return a != b;
                case '&':
                    return a & b;
                case '^':
                    return a ^ b;
                case '|':
                    return a | b;
                case TK_AND:
                    return a && b;
                case TK_OR:
                    return a || b;
                default:
                    // error
                    return 0;
            }
            break;
        case UNARY_EXPRESSION:
            a = ast_calculate(node->child[0]);
            switch (node->op) {
                case '!': // 逻辑非运算符
                    return !a;
                case '~': // 按位取反运算符
                    return ~a;
                case '-': // 负号运算符
                    return -a;
                default:
                    // error
                    return 0;
            }
            break;
        case IDENTIFIER_TOKEN:
            return 0;
        case NUMBER_TOKEN:
            return atoi(((lgx_ast_node_token_t*)node)->tk_start);
        case STRING_TOKEN:
            return 0;
        default:
            return 0;
    }
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
            printf("%*s%s(%d)\n", indent, "", "IF_STATEMENT", ast_calculate(node->child[0]));
            lgx_ast_print(node->child[0], indent+2);
            lgx_ast_print(node->child[1], indent+2);
            break;
        case IF_ELSE_STATEMENT:
            printf("%*s%s(%d)\n", indent, "", "IF_ELSE_STATEMENT", ast_calculate(node->child[0]));
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
        case SWITCH_CASE_STATEMENT:
            printf("%*s%s\n", indent, "", "SWITCH_CASE_STATEMENT");
            break;
        case RETURN_STATEMENT:
            printf("%*s%s\n", indent, "", "RETURN_STATEMENT");
            break;
        case ASSIGNMENT_STATEMENT:
            printf("%*s%s\n", indent, "", "ASSIGNMENT_STATEMENT");
            break;
        // Declaration
        case FUNCTION_DECLARATION:
            printf("%*s%s\n", indent, "", "FUNCTION_DECLARATION");
            lgx_ast_print(node->child[0], indent+2);
            break;
        case VARIABLE_DECLARATION:
            printf("%*s%s\n", indent, "", "VARIABLE_DECLARATION");
            break;
        // Expression
        case CALL_EXPRESSION:
            printf("%*s%s\n", indent, "", "CALL_EXPRESSION");
            break;
        case ARRAY_EXPRESSION:
            printf("%*s%s\n", indent, "", "ARRAY_EXPRESSION");
            break;
        case CONDITIONAL_EXPRESSION:
            printf("%*s%s\n", indent, "", "CONDITIONAL_EXPRESSION");
            break;
        case BINARY_EXPRESSION:
            printf("%*s%s\n", indent, "", "(");

            if (node->child[0])
                lgx_ast_print(node->child[0], indent+2);
            else
                printf("%*s%s\n", indent+2, "", "null");

            printf("%*s%c\n", indent, "", node->op);

            if (node->child[1])
                lgx_ast_print(node->child[1], indent+2);
            else
                printf("%*s%s\n", indent+2, "", "null");

            printf("%*s%s\n", indent, "", ")");
            break;
        case UNARY_EXPRESSION:
            printf("%*s%s\n", indent, "", "(");

            printf("%*s%c\n", indent, "", node->op);

            if (node->child[0])
                lgx_ast_print(node->child[0], indent+2);
            else
                printf("%*s%s\n", indent+2, "", "null");

            printf("%*s%s\n", indent, "", ")");
            break;
        // Other
        case IDENTIFIER_TOKEN:
            printf("%*s%s\n", indent, "", "IDENTIFIER_TOKEN");
            break;
        case NUMBER_TOKEN:
            printf("%*s%s\n", indent, "", "NUMBER_TOKEN");
            break;
        case STRING_TOKEN:
            printf("%*s%s\n", indent, "", "STRING_TOKEN");
            break;
        default:
            printf("%*s%s %d\n", indent, "", "ERROR!", node->type);
    }
}