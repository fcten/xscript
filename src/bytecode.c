#include <stdio.h>

#include "lex.h"
#include "bytecode.h"

#define op_create1(op, a)    ((op << 10) + a)
#define op_create2(op, a, b) ((op << 10) + (a << 5) + b)

#define AX 0
#define BX 1
#define CX 2
#define DX 3

unsigned short buf[10240];
unsigned offset;

void lgx_bc_gen(lgx_ast_node_t* node) {
    int i, p;
    switch(node->type) {
        // Statement
        case BLOCK_STATEMENT:
            // TODO 创建新的变量作用域 ？
            for(i = 0; i < node->children; i++) {
                lgx_bc_gen(node->child[i]);
            }
            break;
        case IF_STATEMENT:
            lgx_bc_gen(node->child[0]);

            // 写入条件跳转
            buf[offset++] = op_create1(OP_TEST, AX);
            p = offset;
            buf[offset++] = op_create1(OP_JMP, 0);

            lgx_bc_gen(node->child[1]);

            // 更新条件跳转
            buf[p] = op_create1(OP_JMP, offset);
            break;
        case IF_ELSE_STATEMENT:
            lgx_bc_gen(node->child[0]);

            // 写入条件跳转
            buf[offset++] = op_create1(OP_TEST, AX);
            p = offset;
            buf[offset++] = op_create1(OP_JMP, 0);

            lgx_bc_gen(node->child[1]);

            // 写入无条件跳转
            buf[offset++] = op_create1(OP_JMP, 0);
            // 更新条件跳转
            buf[p] = op_create1(OP_JMP, offset);
            p = offset - 1;

            lgx_bc_gen(node->child[2]);

            // 更新无条件跳转
            buf[p] = op_create1(OP_JMP, offset);
            break;
        case FOR_STATEMENT:

            break;
        case WHILE_STATEMENT:
            p = offset;

            lgx_bc_gen(node->child[0]);

            lgx_bc_gen(node->child[1]);
            break;
        case DO_WHILE_STATEMENT:

            lgx_bc_gen(node->child[0]);
            lgx_bc_gen(node->child[1]);
            break;
        case CONTINUE_STATEMENT:

            break;
        case BREAK_STATEMENT:
            // 写入跳转指令
            // 保存指令位置以便未来更新跳转地址
            break;
        case SWITCH_CASE_STATEMENT:

            break;
        case RETURN_STATEMENT:
            // 计算返回值
            if (node->child[0]) {
                lgx_bc_gen(node->child[0]);
            }

            // 释放参数与局部变量

            // 返回值入栈
            if (node->child[0]) {
                buf[offset++] = op_create1(OP_PUSH, AX);
            } else {
                // 如果没有指定返回值，则返回 undefined
                buf[offset++] = op_create1(OP_PUSH, AX);
            }

            // 写入返回指令
            buf[offset++] = op_create1(OP_RET, 0);

            break;
        case ASSIGNMENT_STATEMENT:

            break;
        // Declaration
        case FUNCTION_DECLARATION:
            // 跳过函数体
            lgx_bc_gen(node->child[0]);
            break;
        case VARIABLE_DECLARATION:

            break;
        // Expression
        case CALL_EXPRESSION:

            break;
        case ARRAY_EXPRESSION:

            break;
        case CONDITIONAL_EXPRESSION:

            break;
        case BINARY_EXPRESSION:

            lgx_bc_gen(node->child[0]);

            buf[offset++] = op_create2(OP_MOV, BX, AX);

            lgx_bc_gen(node->child[1]);

            buf[offset++] = op_create2(OP_ADD, AX, BX);

            break;
        case UNARY_EXPRESSION:

            lgx_bc_gen(node->child[0]);

            break;
        // Other
        case IDENTIFIER_TOKEN:
            buf[offset++] = op_create2(OP_LOAD, AX, 0);

            break;
        case NUMBER_TOKEN:
            buf[offset++] = op_create2(OP_MOV, AX, 0);

            break;
        case STRING_TOKEN:

            break;
        default:
            printf("%s %d\n", "ERROR!", node->type);
    }
}

char *R[] = {
    "AX", "BX", "CX", "DX"
};

void lgx_bc_print() {
    int i = 0;
    unsigned short op, a, b, c;
    while(i < offset) {
        op = buf[i] >> 10;
        a = (buf[i] & 0b0000001111100000) >> 5;
        b = buf[i] & 0b0000000000011111;
        c = buf[i] & 0b0000001111111111;

        switch(op) {
            case OP_MOV:
                printf("%4d MOV %s %s\n", i, R[a], R[b]);
                break;
            case OP_LOAD:
                printf("%4d LOAD %s %d\n", i, R[a], b);
                break;
            case OP_PUSH:
                printf("%4d PUSH %s\n", i, R[c]);
                break;
            case OP_POP:
                printf("%4d POP %s\n", i, R[c]);
                break;
            case OP_CMP:
                printf("%4d CMP\n", i);
                break;
            case OP_ADD:
                printf("%4d ADD %s %s\n", i, R[a], R[b]);
                break;
            case OP_TEST:
                printf("%4d TEST %s\n", i, R[c]);
                break;
            case OP_JMP:
                printf("%4d JMP %d\n", i, c);
                break;
            case OP_CALL:
                printf("%4d CALL\n", i);
                break;
            case OP_RET:
                printf("%4d RET\n", i);
                break;
            case OP_NOP:
                printf("%4d NOP\n", i);
                break;
            default:
                // error
                break;
        }

        i ++;
    }
}