#include <stdio.h>
#include <stdlib.h>

#include "./tokenizer/lex.h"
#include "./parser/ast.h"
#include "./common/bytecode.h"

int read_file(const char* file, char** p) {
    FILE* fp;
    fp = fopen(file, "rb");  // localfile文件名

    fseek(fp, 0L, SEEK_END);  /* 定位到文件末尾 */
    int flen = ftell(fp);     /* 得到文件大小 */
    *p = (char*)malloc(flen); /* 根据文件大小动态分配内存空间 */
    if (*p == NULL) {
        fclose(fp);
        return 0;
    }
    fseek(fp, 0L, SEEK_SET);       /* 定位到文件开头 */
    return fread(*p, 1, flen, fp); /* 一次性读取全部文件内容 */
}

typedef struct {
    lgx_scope_t scope_chain;
} lgx_interpreter_t;

lgx_interpreter_t itp;

long long execute(lgx_ast_node_t* node) {
    switch(node->type) {
        // Statement
        case BLOCK_STATEMENT: {
            lgx_scope_new(&itp.scope_chain);
            for(int i = 0; i < node->children; i++) {
                execute(node->child[i]);
            }
            lgx_scope_delete(&itp.scope_chain);
            break;
        }
        case IF_STATEMENT:
            if (execute(node->child[0])) {
                execute(node->child[1]);
            }
            break;
        case IF_ELSE_STATEMENT:
            if (execute(node->child[0])) {
                execute(node->child[1]);
            } else {
                execute(node->child[2]);
            }
            break;
        case FOR_STATEMENT:

            break;
        case WHILE_STATEMENT:
            while (execute(node->child[0])) {
                execute(node->child[1]);
            }
            break;
        case DO_WHILE_STATEMENT:
            do {
                execute(node->child[0]);
            } while(execute(node->child[1]));
            break;
        case CONTINUE_STATEMENT:

            break;
        case BREAK_STATEMENT: // break 只应该出现在块级作用域中

            break;
        case SWITCH_CASE_STATEMENT:

            break;
        case RETURN_STATEMENT:
            // 计算返回值
            if (node->child[0]) {
                printf("%lld\n", execute(node->child[0]));
            } else {
                printf("undefined\n");
            }
            break;
        case ASSIGNMENT_STATEMENT: {
            lgx_val_t *v;
            lgx_str_ref_t s;

            s.buffer = (unsigned char *)((lgx_ast_node_token_t *)(node->child[0]))->tk_start;
            s.length = ((lgx_ast_node_token_t *)(node->child[0]))->tk_length;
            v = lgx_scope_val_get(&itp.scope_chain, &s);
            v->type = T_LONG;
            v->v.l = execute(node->child[1]);
            break;
        }
        // Declaration
        case FUNCTION_DECLARATION:
            if (node->child[0]) {

            } else {
                // 匿名函数暂时不被支持
            }
            break;
        case VARIABLE_DECLARATION: {
            lgx_val_t *v;
            lgx_str_ref_t s;

            s.buffer = (unsigned char *)((lgx_ast_node_token_t *)(node->child[0]))->tk_start;
            s.length = ((lgx_ast_node_token_t *)(node->child[0]))->tk_length;
            lgx_scope_val_set(&itp.scope_chain, &s);
            if (node->child[1]) {
                v = lgx_scope_val_get(&itp.scope_chain, &s);
                v->type = T_LONG;
                v->v.l = execute(node->child[1]);
            }
            break;
        }
        // Expression
        case CALL_EXPRESSION:

            break;
        case ARRAY_EXPRESSION:

            break;
        case CONDITIONAL_EXPRESSION:

            break;
        case BINARY_EXPRESSION:
            switch (node->op) {
                case '+':
                    return execute(node->child[0]) + execute(node->child[1]);
                case '-':
                    return execute(node->child[0]) - execute(node->child[1]);
                case '*':
                    return execute(node->child[0]) * execute(node->child[1]);
                case '/':
                    return execute(node->child[0]) / execute(node->child[1]);
                case '%':
                    return execute(node->child[0]) % execute(node->child[1]);
                case TK_SHL:
                    return execute(node->child[0]) << execute(node->child[1]);
                case TK_SHR:
                    return execute(node->child[0]) >> execute(node->child[1]);
                case '>':
                    return execute(node->child[0]) > execute(node->child[1]);
                case '<':
                    return execute(node->child[0]) < execute(node->child[1]);
                case TK_GE:
                    return execute(node->child[0]) >= execute(node->child[1]);
                case TK_LE:
                    return execute(node->child[0]) <= execute(node->child[1]);
                case TK_EQ:
                    return execute(node->child[0]) == execute(node->child[1]);
                case TK_NE:
                    return execute(node->child[0]) != execute(node->child[1]);
                case '&':
                    return execute(node->child[0]) & execute(node->child[1]);
                case '^':
                    return execute(node->child[0]) ^ execute(node->child[1]);
                case '|':
                    return execute(node->child[0]) | execute(node->child[1]);
                case TK_AND:
                    return execute(node->child[0]) && execute(node->child[1]);
                case TK_OR:
                    return execute(node->child[0]) || execute(node->child[1]);
                default:
                    // error
                    return 0;
            }
            break;
        case UNARY_EXPRESSION:
            switch (node->op) {
                case '!':
                    return !execute(node->child[0]);
                case '~':
                    return ~execute(node->child[0]);
                case '-':
                    return -execute(node->child[0]);
                default:
                    // error
                    break;
            }

            break;
        // Other
        case IDENTIFIER_TOKEN: {
            lgx_val_t *v;
            lgx_str_ref_t s;

            s.buffer = (unsigned char *)((lgx_ast_node_token_t *)node)->tk_start;
            s.length = ((lgx_ast_node_token_t *)node)->tk_length;
            v = lgx_scope_val_get(&itp.scope_chain, &s);
            return v->v.l;
        }
        case NUMBER_TOKEN:
            return atoi(((lgx_ast_node_token_t *)node)->tk_start);
        case STRING_TOKEN:

            break;
        default:
            printf("%s %d\n", "ERROR!", node->type);
    }
    
    return 0;
}

int main(int argc, char* argv[]) {
    lgx_lex_init();

    lgx_ast_t ast;
    lgx_ast_init(&ast);
    ast.lex.length = read_file("./test/function.x", &ast.lex.source);

    lgx_ast_parser(&ast);
    if (ast.errno) {
        printf("%.*s\n", ast.err_len, ast.err_info);
        return 1;
    }

    lgx_ast_print(ast.root, 0);
    
    lgx_scope_init(&itp.scope_chain);
    execute(ast.root);
    
    return 0;
}