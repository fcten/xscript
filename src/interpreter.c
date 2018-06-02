#include <stdio.h>
#include <stdlib.h>

#include "./tokenizer/lex.h"
#include "./parser/ast.h"
#include "./parser/scope.h"
#include "./common/bytecode.h"
#include "./interpreter/vm.h"

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

long long execute(lgx_ast_node_t* node) {
    switch(node->type) {
        // Statement
        case BLOCK_STATEMENT: {
            for(int i = 0; i < node->children; i++) {
                int ret = execute(node->child[i]);
                if (ret != 0) {
                    return ret;
                }
            }
            break;
        }
        case IF_STATEMENT:
            if (execute(node->child[0])) {
                return execute(node->child[1]);
            }
            break;
        case IF_ELSE_STATEMENT:
            if (execute(node->child[0])) {
                return execute(node->child[1]);
            } else {
                return execute(node->child[2]);
            }
        case FOR_STATEMENT:

            break;
        case WHILE_STATEMENT:
            while (execute(node->child[0])) {
                int ret = execute(node->child[1]);
                if (ret == 1) {
                    break;
                }
            }
            break;
        case DO_WHILE_STATEMENT:
            do {
                int ret = execute(node->child[0]);
                if (ret == 1) {
                    break;
                }
            } while(execute(node->child[1]));
            break;
        case CONTINUE_STATEMENT:
            return 2;
        case BREAK_STATEMENT:
            return 1;
        case SWITCH_CASE_STATEMENT:

            break;
        case RETURN_STATEMENT:
            // 计算返回值
            if (node->child[0]) {
                return execute(node->child[0]);
            } else {
                return 0;
            }
        case ASSIGNMENT_STATEMENT: {
            lgx_val_t *v;
            lgx_str_ref_t s;

            s.buffer = (unsigned char *)((lgx_ast_node_token_t *)(node->child[0]))->tk_start;
            s.length = ((lgx_ast_node_token_t *)(node->child[0]))->tk_length;
            v = lgx_scope_val_get(node, &s);
            v->type = T_LONG;
            v->v.l = execute(node->child[1]);
            break;
        }
        // Declaration
        case FUNCTION_DECLARATION:
            // 执行一次赋值操作

            break;
        case VARIABLE_DECLARATION:
            // 如果声明中附带初始化，则执行一次赋值操作
            if (node->child[1]) {
                lgx_val_t *v;
                lgx_str_ref_t s;

                s.buffer = (unsigned char *)((lgx_ast_node_token_t *)(node->child[0]))->tk_start;
                s.length = ((lgx_ast_node_token_t *)(node->child[0]))->tk_length;
            
                v = lgx_scope_val_get(node, &s);
                v->type = T_LONG;
                v->v.l = execute(node->child[1]);
            }
            break;
        // Expression
        case CONDITIONAL_EXPRESSION:

            break;
        case BINARY_EXPRESSION: {
            long long a = execute(node->child[0]), b = execute(node->child[1]);
            switch (node->u.op) {
                case '+':
                    return a + b;
                case '-':
                    return a - b;
                case '*':
                    return a * b;
                case '/':
                    return a / b;
                case '%':
                    return a % b;
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
                case TK_CALL:

                case TK_INDEX:
                case TK_ATTR:
                default:
                    // error
                    return 0;
            }
            break;
        }
        case UNARY_EXPRESSION:
            switch (node->u.op) {
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
            v = lgx_scope_val_get(node, &s);
            if (v) {
                return v->v.l;
            } else {
                printf("[Error] use undefined variable `%.*s`\n", s.length, s.buffer);
                return 0;
            }
        }
        case NUMBER_TOKEN:
            return atoi(((lgx_ast_node_token_t *)node)->tk_start);
        case STRING_TOKEN:

            break;
        case FUNCTION_CALL_PARAMETER:
        case FUNCTION_DECL_PARAMETER:
            break;
        default:
            printf("%s %d\n", "ERROR!", node->type);
    }
    
    return 0;
}

#define I0(op)          (op)
#define I1(op, e)       (op + (e << 8))
#define I2(op, a, d)    (op + (a << 8) + (d << 16))
#define I3(op, a, b, c) (op + (a << 8) + (b << 16) + (c << 24))

unsigned opcode[] = {
    I2(OP_MOVI, 0, 0),
    I2(OP_MOVI, 1, 10000),
    I2(OP_MULI, 1, 1000),
    I3(OP_GTI,  2, 1, 0),
    I1(OP_TEST, 2),
    I1(OP_JMPI, 9),
    I2(OP_ADD,  0, 1),
    I2(OP_SUBI, 1, 1),
    I1(OP_JMPI, 3),
    I1(OP_ECHO, 0),
    I0(OP_HLT)
};

int main(int argc, char* argv[]) {
    /*
    lgx_lex_init();

    lgx_ast_t ast;
    lgx_ast_init(&ast);
    ast.lex.length = read_file(argv[1], &ast.lex.source);

    lgx_ast_parser(&ast);
    if (ast.errno) {
        printf("%.*s\n", ast.err_len, ast.err_info);
        return 1;
    }

    lgx_ast_print(ast.root, 0);

    printf("%lld\n", execute(ast.root));
    */
    
    lgx_vm_t vm;
    lgx_vm_init(&vm, (unsigned *)opcode, sizeof(opcode)/4);
    lgx_vm_start(&vm);

    return 0;
}