#include <stdio.h>
#include <stdlib.h>

#include "../parser/scope.h"
#include "../common/bytecode.h"
#include "compiler.h"

static void reg_push(lgx_bc_t *bc, unsigned char i) {
    bc->regs[bc->reg_top] = i;
    bc->reg_top ++;
}

static unsigned char reg_pop(lgx_bc_t *bc) {
    bc->reg_top --;
    return bc->regs[bc->reg_top];
}

static void bc_append(lgx_bc_t *bc, unsigned i) {
    if (bc->bc_top >= bc->bc_size) {
        bc->bc_size *= 2;
        bc->bc = realloc(bc->bc, bc->bc_size);
    }

    bc->bc[bc->bc_top] = i;
    bc->bc_top ++;
}

static void bc_set(lgx_bc_t *bc, unsigned pos, unsigned i) {
    bc->bc[pos] = i;
}

static unsigned char bc_gen(lgx_bc_t *bc, lgx_ast_node_t *node) {
    switch(node->type) {
        // Statement
        case BLOCK_STATEMENT:{
            int i;
            // 为当前作用域的变量分配寄存器
            for(i = 0; i < node->u.symbols->table_offset; i++) {
                node->u.symbols->table[i].v.u.reg = reg_pop(bc);
            }

            for(i = 0; i < node->children; i++) {
                bc_gen(bc, node->child[i]);
            }

            // 释放局部变量的寄存器
            for(i = node->u.symbols->table_offset - 1; i >= 0; i--) {
                reg_push(bc, node->u.symbols->table[i].v.u.reg);
            }
            break;
        }
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
        case WHILE_STATEMENT:{
            unsigned pos1 = bc->bc_top;
            // todo 如果条件是立即数，则不需要递归调用
            unsigned char r = bc_gen(bc, node->child[0]);
            
            // 写入条件跳转
            unsigned pos2 = bc->bc_top;
            bc_append(bc, I2(OP_TEST, r, 0));
            reg_push(bc, r);

            bc_gen(bc, node->child[1]);

            // 写入无条件跳转
            bc_append(bc, I1(OP_JMPI, pos1));
            
            // 更新条件跳转
            bc_set(bc, pos2, I2(OP_TEST, r, bc->bc_top));
            break;
        }
        case DO_WHILE_STATEMENT:

            bc_gen(bc, node->child[0]);
            bc_gen(bc, node->child[1]);
            break;
        case CONTINUE_STATEMENT:

            break;
        case BREAK_STATEMENT: // break 只应该出现在块级作用域中
            //bc_scope_delete(bc);

            // 写入跳转指令
            // 保存指令位置以便未来更新跳转地址
            break;
        case SWITCH_CASE_STATEMENT:

            break;
        case RETURN_STATEMENT:{
            // 计算返回值
            if (node->child[0]) {
                unsigned char r = bc_gen(bc, node->child[0]);
                bc_append(bc, I1(OP_ECHO, r));
                reg_push(bc, r);
            }

            // 释放参数与局部变量

            // 更新返回值寄存器

            // 写入返回指令

            break;
        }
        case ASSIGNMENT_STATEMENT:{
            lgx_val_t *v;
            lgx_str_ref_t s;

            s.buffer = (unsigned char *)((lgx_ast_node_token_t *)node->child[0])->tk_start;
            s.length = ((lgx_ast_node_token_t *)node->child[0])->tk_length;
            // todo 非局部变量需要特殊处理
            v = lgx_scope_val_get(node, &s);
            // todo 非表达式优化
            unsigned char r = bc_gen(bc, node->child[1]);
            
            bc_append(bc, I2(OP_MOV, v->u.reg, r));
            
            reg_push(bc, r);
            break;
        }
        // Declaration
        case FUNCTION_DECLARATION:{
            // 执行一次赋值操作
               
            break;
        }
        case VARIABLE_DECLARATION:{
            // 如果声明中附带初始化，则执行一次赋值操作
            if (node->child[1]) {
                lgx_val_t *v;
                lgx_str_ref_t s;

                s.buffer = (unsigned char *)((lgx_ast_node_token_t *)(node->child[0]))->tk_start;
                s.length = ((lgx_ast_node_token_t *)(node->child[0]))->tk_length;
            
                v = lgx_scope_val_get(node, &s);

                // todo 如果右侧是立即数，则不需要递归调用
                unsigned char r = bc_gen(bc, node->child[1]);
                bc_append(bc, I2(OP_MOV, v->u.reg, r));
                reg_push(bc, r);
            }
            break;
        }
        // Expression
        case CONDITIONAL_EXPRESSION:

            break;
        case BINARY_EXPRESSION: {
            // todo 非表达式优化
            unsigned char r1 = bc_gen(bc, node->child[0]);
            unsigned char r2 = bc_gen(bc, node->child[1]);
            unsigned char r3;

            switch (node->u.op) {
                case '+':
                    bc_append(bc, I2(OP_ADD, r1, r2));
                    reg_push(bc, r2);
                    return r1;
                case '-':
                    bc_append(bc, I2(OP_SUB, r1, r2));
                    reg_push(bc, r2);
                    return r1;
                case '*':
                    bc_append(bc, I2(OP_MUL, r1, r2));
                    reg_push(bc, r2);
                    return r1;
                case '/':
                    bc_append(bc, I2(OP_DIV, r1, r2));
                    reg_push(bc, r2);
                    return r1;
                case '>':
                    r3 = reg_pop(bc);
                    bc_append(bc, I3(OP_LT, r3, r2, r1));
                    reg_push(bc, r1);
                    reg_push(bc, r2);
                    return r3;
                case '<':
                    r3 = reg_pop(bc);
                    bc_append(bc, I3(OP_LT, r3, r1, r2));
                    reg_push(bc, r1);
                    reg_push(bc, r2);
                    return r3;
                default:
                    // error
                    break;
            }
            break;
        }
        case UNARY_EXPRESSION:

            bc_gen(bc, node->child[0]);

            switch (node->u.op) {
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
        case IDENTIFIER_TOKEN:{
            lgx_val_t *v;
            lgx_str_ref_t s;

            s.buffer = (unsigned char *)((lgx_ast_node_token_t *)node)->tk_start;
            s.length = ((lgx_ast_node_token_t *)node)->tk_length;
            // todo 非局部变量需要特殊处理
            v = lgx_scope_val_get(node, &s);
            
            unsigned char r = reg_pop(bc);
            bc_append(bc, I2(OP_MOV, r, v->u.reg));
            
            return r;
        }
        case NUMBER_TOKEN:{
            lgx_ast_node_token_t *token = (lgx_ast_node_token_t *)node;
            long long num = atol(token->tk_start);
            unsigned char r = reg_pop(bc);

            // todo 大整数从常量表中读取
            //printf("%lld %lld\n",num, num & 0xFFFF);
            if ((num >> 16) & 0xFFFF) {
                bc_append(bc, I2(OP_MOVI, r, (unsigned)((num >> 16) & 0xFFFF)));
                bc_append(bc, I2(OP_SHLI, r, 16));
                bc_append(bc, I2(OP_ADDI, r, (unsigned)(num & 0xFFFF)));
            } else {
                bc_append(bc, I2(OP_MOVI, r, (unsigned)(num & 0xFFFF)));
            }
            

            return r;
        }
        case STRING_TOKEN:

            break;
        default:
            printf("%s %d\n", "ERROR!", node->type);
    }
    
    return 0;
}

int lgx_bc_compile(lgx_ast_t *ast, lgx_bc_t *bc) {
    bc->reg_top = 0;
    for(int i = 255; i >= 0; i--) {
        reg_push(bc, i);
    }
    
    bc->bc_size = 1024;
    bc->bc_top = 0;
    bc->bc = malloc(bc->bc_size);
    
    bc_gen(bc, ast->root);
    bc_append(bc, I0(OP_HLT));
    
    return 0;
}