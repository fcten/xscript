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

#define bc_mov(a, b)     bc_append(bc, I2(OP_MOV, a, b))
#define bc_add(a, b, c)  bc_append(bc, I3(OP_ADD, a, b, c))
#define bc_sub(a, b, c)  bc_append(bc, I3(OP_SUB, a, b, c))
#define bc_mul(a, b, c)  bc_append(bc, I3(OP_MUL, a, b, c))
#define bc_div(a, b, c)  bc_append(bc, I3(OP_DIV, a, b, c))
#define bc_addi(a, b, c) bc_append(bc, I3(OP_ADDI, a, b, c))
#define bc_subi(a, b, c) bc_append(bc, I3(OP_SUBI, a, b, c))
#define bc_muli(a, b, c) bc_append(bc, I3(OP_MULI, a, b, c))
#define bc_divi(a, b, c) bc_append(bc, I3(OP_DIVI, a, b, c))
#define bc_shli(a, b)    bc_append(bc, I2(OP_SHLI, a, b))
#define bc_eq(a, b, c)   bc_append(bc, I3(OP_EQ, a, b, c))
#define bc_le(a, b, c)   bc_append(bc, I3(OP_LE, a, b, c))
#define bc_lt(a, b, c)   bc_append(bc, I3(OP_LT, a, b, c))
#define bc_gti(a, b, c)  bc_append(bc, I3(OP_GTI, a, b, c))
#define bc_gei(a, b, c)  bc_append(bc, I3(OP_GEI, a, b, c))
#define bc_lti(a, b, c)  bc_append(bc, I3(OP_LTI, a, b, c))
#define bc_lei(a, b, c)  bc_append(bc, I3(OP_LEI, a, b, c))
#define bc_test(a, b)    bc_append(bc, I2(OP_TEST, a, b))
#define bc_jmpi(a)       bc_append(bc, I1(OP_JMPI, a))
#define bc_echo(a)       bc_append(bc, I1(OP_ECHO, a))
#define bc_hlt(a)        bc_append(bc, I0(OP_HLT))

enum {
    ET_INSTANT,
    ET_REG,
    ET_ID,
};

typedef struct {
    unsigned char type;
    union {
        unsigned char instant;
        unsigned char reg;
    } u;
} expr_t;

static void reg_free(expr_t *e) {
    if (e->type == ET_REG) {
        reg_push(r->u.reg);
    }
}

static void bc_identifier(lgx_ast_node_t *node, expr_t *expr) {
    lgx_val_t *v;
    lgx_str_ref_t s;

    s.buffer = (unsigned char *)((lgx_ast_node_token_t *)node)->tk_start;
    s.length = ((lgx_ast_node_token_t *)node)->tk_length;
    // todo 非局部变量需要特殊处理
    v = lgx_scope_val_get(node, &s);

    expr->type = ET_ID;
    expr->u.reg = v->u.reg;
}

static void bc_number8(lgx_ast_node_t *node, expr_t *expr) {
    // TODO 处理浮点数
    long long num = atol(((lgx_ast_node_token_t *)node)->tk_start);

    if (num >= 0 && num <= 255) {
        expr->type = ET_INSTANT;
        expr->u.instant = num;
    } else {
        // 超过范围的立即数无法写入到指令中
        // todo 从常量表中读取
        expr->type = ET_REG;
    }
}

static void bc_expr_gen(int op, expr_t *e0, expr_t *e1, expr_t *e2) {
    switch (op) {
        case '+':
            if (e2->type == ET_INSTANT) {
                bc_addi(e0->u.reg, e1->u.reg, e2->u.instant);
            } else {
                bc_add(e0->u.reg, e1->u.reg, e2->u.reg);
            }
            break;
        case '-':
            if (e2->type == ET_INSTANT) {
                bc_subi(e0->u.reg, e1->u.reg, e2->u.instant);
            } else {
                bc_sub(e0->u.reg, e1->u.reg, e2->u.reg);
            }
            break;
        case '*':

            break;
        case '/':

            break;
        case '>':{
            if (e2->type == ET_INSTANT) {
                bc_gti(e0->u.reg, e1->u.reg, e2->u.instant);
            } else {
                bc_lt(e0->u.reg, e2->u.reg, e1->u.reg);
            }
            break;
        }
        case '<':{
            if (e2->type == ET_INSTANT) {
                bc_lti(e0->u.reg, e1->u.reg, e2->u.instant);
            } else {
                bc_lt(e0->u.reg, e1->u.reg, e2->u.reg);
            }
            break;
        }
        default:
            // error
            break;
    }
}

static void bc_expr(lgx_bc_t *bc, lgx_ast_node_t *node, expr_t *e1, expr_t *e2) {
    // todo 折叠常量表达式 constant-fold


    // 如果是立即数
    if (node->type == NUMBER_TOKEN) {
        bc_number8(node, e1);
        bc_movi(e0->u.reg, e1->u.instant);
    }
    // 如果是变量
    else if (node->type == IDENTIFIER_TOKEN) {
        bc_identifier(node, e1);
        if (e0->u.reg != e1->u.reg) {
            bc_mov(e0->u.reg, e1->u.reg);
        }
    }
    // 如果是单目表达式
    else if (node->type == UNARY_EXPRESSION) {
        bc_expr(node->child[1], &e1, &e2);
    }
    // 如果是双目表达式
    else if (node->type == BINARY_EXPRESSION) {
        bc_expr(node->child[1], &e1, &e2);
    }
}

static void bc_stat_assign(lgx_bc_t *bc, lgx_ast_node_t *node) {
    expr_t e0, e1, e2;

    bc_identifier(node->child[0], &e0);

    // 如果右侧是立即数
    if (node->child[0]->type == NUMBER_TOKEN) {
        bc_number8(node, e1);
        bc_movi(e0->u.reg, e1->u.instant);
    }
    // 如果右侧是变量
    else if (node->child[0]->type == IDENTIFIER_TOKEN) {
        bc_identifier(node, e1);
        if (e0->u.reg != e1->u.reg) {
            bc_mov(e0->u.reg, e1->u.reg);
        }
    }
    // 如果右侧是单目表达式
    else if (node->child[0]->type == UNARY_EXPRESSION) {
        bc_expr(node->child[1], &e1, &e2);
    }
    // 如果右侧是双目表达式
    else if (node->child[0]->type == BINARY_EXPRESSION) {
        bc_expr(node->child[1], &e1, &e2);
    }

    bc_expr_gen(node->child[1]->u.op, &e0, &e1, &e2);

    reg_free(&e1);
    reg_free(&e2);
}

static void bc_gen(lgx_bc_t *bc, lgx_ast_node_t *node) {
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
            //bc_gen(bc, node->child[0]);

            // 写入条件跳转

            //bc_gen(bc, node->child[1]);

            // 更新条件跳转
            break;
        case IF_ELSE_STATEMENT:
            //bc_gen(bc, node->child[0]);

            // 写入条件跳转

            //bc_gen(bc, node->child[1]);

            // 写入无条件跳转
            // 更新条件跳转

            //bc_gen(bc, node->child[2]);

            // 更新无条件跳转
            break;
        case FOR_STATEMENT:

            break;
        case WHILE_STATEMENT:{
            unsigned pos1 = bc->bc_top;
            // todo 如果条件是立即数，则不需要递归调用
            unsigned char r = reg_pop(bc);
            bc_gen(bc, r, node->child[0]);
            
            // 写入条件跳转
            unsigned pos2 = bc->bc_top;
            bc_test(r, 0);
            reg_push(bc, r);

            bc_gen(bc, 0, node->child[1]);

            // 写入无条件跳转
            bc_jmpi(pos1);
            
            // 更新条件跳转
            bc_set(bc, pos2, I2(OP_TEST, r, bc->bc_top));
            break;
        }
        case DO_WHILE_STATEMENT:

            //bc_gen(bc, node->child[0]);
            //bc_gen(bc, node->child[1]);
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
                unsigned char r = reg_pop(bc);
                bc_gen(bc, r, node->child[0]);
                bc_echo(r);
                reg_push(bc, r);
            }

            // 释放参数与局部变量

            // 更新返回值寄存器

            // 写入返回指令

            break;
        }
        case ASSIGNMENT_STATEMENT:
            bc_stat_assign(bc, node);
            break;
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

                bc_gen(bc, v->u.reg, node->child[1]);
            }
            break;
        }
        // Expression
        case CONDITIONAL_EXPRESSION:

            break;
        case UNARY_EXPRESSION:

            //bc_gen(bc, node->child[0]);

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
        case STRING_TOKEN:

            break;
        default:
            printf("%s %d\n", "ERROR!", node->type);
    }
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
    bc_hlt();
    
    return 0;
}