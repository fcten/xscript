#include <stdio.h>
#include <stdlib.h>

#include "../tokenizer/lex.h"
#include "val.h"
#include "list.h"
#include "bytecode.h"

int lgx_bc_init(lgx_bc_t *bc, lgx_ast_t* ast) {
    bc->bc_size = 1024;
    bc->bc = malloc(bc->bc_size * sizeof(unsigned));
    if (!bc->bc) {
        bc->bc_size = 0;
        return 1;
    }

    bc->st_size = 1024;
    bc->st = malloc(bc->st_size * sizeof(lgx_val_t));
    if (!bc->st) {
        free(bc->bc);
        bc->bc_size = 0;
        bc->st_size = 0;
        return 2;
    }
    
    bc->ast = ast;

    lgx_list_init(&bc->scope.head);
    
    lgx_hash_init(&bc->constants, 1024);

    return 0;
}

// 创建新的变量作用域
void bc_scope_new(lgx_bc_t *bc) {
    lgx_val_scope_t* s = malloc(sizeof(lgx_val_scope_t));
    lgx_list_init(&s->head);
    
    lgx_val_scope_t* parent = lgx_list_last_entry(&bc->scope.head, lgx_val_scope_t, head);
    
    s->vr = parent->vr + parent->vr_offset;
    s->vr_offset = 0;

    lgx_list_add_tail(&bc->scope.head, &s->head);
}

// 删除当前变量作用域
void bc_scope_delete(lgx_bc_t *bc) {
    // 释放局部变量
}

// 在当前作用域上分配变量
lgx_val_t* bc_val_new(lgx_bc_t *bc) {
    lgx_val_scope_t* s = lgx_list_last_entry(&bc->scope.head, lgx_val_scope_t, head);

    return &s->vr[s->vr_offset++];
}

int bc_gen(lgx_bc_t *bc, lgx_ast_node_t *node) {
    unsigned i;
    lgx_val_t k, v;
    lgx_str_t s;
    lgx_ast_node_token_t *token;
    switch(node->type) {
        // Statement
        case BLOCK_STATEMENT:
            bc_scope_new(bc);

            for(i = 0; i < node->children; i++) {
                bc_gen(bc, node->child[i]);
            }

            bc_scope_delete(bc);
            break;
        case IF_STATEMENT:
            bc_gen(bc, node->child[0]);

            // 写入条件跳转

            bc_gen(bc, node->child[1]);

            // 更新条件跳转
            break;
        case IF_ELSE_STATEMENT:
            bc_gen(bc, node->child[0]);

            // 写入条件跳转

            bc_gen(bc, node->child[1]);

            // 写入无条件跳转
            // 更新条件跳转

            bc_gen(bc, node->child[2]);

            // 更新无条件跳转
            break;
        case FOR_STATEMENT:

            break;
        case WHILE_STATEMENT:
            bc_gen(bc, node->child[0]);

            // 写入条件跳转

            bc_gen(bc, node->child[1]);

            // 写入无条件跳转

            // 更新条件跳转
            break;
        case DO_WHILE_STATEMENT:

            bc_gen(bc, node->child[0]);
            bc_gen(bc, node->child[1]);
            break;
        case CONTINUE_STATEMENT:

            break;
        case BREAK_STATEMENT: // break 只应该出现在块级作用域中
            bc_scope_delete(bc);

            // 写入跳转指令
            // 保存指令位置以便未来更新跳转地址
            break;
        case SWITCH_CASE_STATEMENT:

            break;
        case RETURN_STATEMENT:
            // 计算返回值
            if (node->child[0]) {
                bc_gen(bc, node->child[0]);
            }

            // 释放参数与局部变量

            // 更新返回值寄存器

            // 写入返回指令

            break;
        case ASSIGNMENT_STATEMENT:

            break;
        // Declaration
        case FUNCTION_DECLARATION:
            // 跳过函数体
            bc_gen(bc, node->child[0]);
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
            bc_gen(bc, node->child[0]);
            bc_gen(bc, node->child[1]);

            switch (node->op) {
                case '+':

                    break;
                case '-':

                    break;
                case '*':

                    break;
                case '/':

                    break;
                default:
                    // error
                    break;
            }

            break;
        case UNARY_EXPRESSION:

            bc_gen(bc, node->child[0]);

            switch (node->op) {
                case '!':

                    break;
                case '~':

                    break;
                case '-':

                    break;
                default:
                    // error
                    break;
            }

            break;
        // Other
        case IDENTIFIER_TOKEN:

            break;
        case NUMBER_TOKEN:
            //bc->bc[bc->bc_offset++] = OP_PUSH;
            token = (lgx_ast_node_token_t *)node;

            k.type = T_LONG;
            k.v.l = atoi(token->tk_start);
            
            i = lgx_hash_get(&bc->constants, &k);
            break;
        case STRING_TOKEN:

            break;
        default:
            printf("%s %d\n", "ERROR!", node->type);
    }
    
    return 0;
}

int lgx_bc_gen(lgx_bc_t *bc) {
    return bc_gen(bc, bc->ast->root);
}

char *R[] = {
    "AX", "BX", "CX", "DX"
};

int lgx_bc_print(lgx_bc_t *bc) {
    int i = 0;
    unsigned short op, a, b, c;
    while(i < bc->bc_offset) {
        op = bc->bc[i] >> 10;
        a = (bc->bc[i] & 0b0000001111100000) >> 5;
        b = bc->bc[i] & 0b0000000000011111;
        c = bc->bc[i] & 0b0000001111111111;

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
    
    return 0;
}