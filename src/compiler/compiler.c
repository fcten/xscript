#include <stdio.h>
#include <stdlib.h>

#include "../parser/scope.h"
#include "../common/bytecode.h"
#include "../common/val.h"
#include "compiler.h"

static void reg_push(lgx_bc_t *bc, unsigned char i) {
    bc->regs[bc->reg_top] = i;
    bc->reg_top ++;
}

static unsigned char reg_pop(lgx_bc_t *bc) {
    bc->reg_top --;
    return bc->regs[bc->reg_top];
}

static void reg_free(lgx_bc_t *bc, lgx_val_t *e) {
    if (e->type == T_REGISTER) {
        reg_push(bc, e->u.l);
    }
}

static int bc_append(lgx_bc_t *bc, unsigned i) {
    if (bc->bc_top >= bc->bc_size) {
        bc->bc_size *= 2;
        bc->bc = realloc(bc->bc, bc->bc_size);
    }

    bc->bc[bc->bc_top] = i;
    bc->bc_top ++;

    return 0;
}

static void bc_set(lgx_bc_t *bc, unsigned pos, unsigned i) {
    bc->bc[pos] = i;
}

#define bc_nop()         bc_append(bc, I0(OP_NOP))

#define bc_mov(a, b)     bc_append(bc, I2(OP_MOV, a, b))
#define bc_movi(a, b)    bc_append(bc, I2(OP_MOVI, a, b))

#define bc_add(a, b, c)  bc_append(bc, I3(OP_ADD, a, b, c))
#define bc_sub(a, b, c)  bc_append(bc, I3(OP_SUB, a, b, c))
#define bc_mul(a, b, c)  bc_append(bc, I3(OP_MUL, a, b, c))
#define bc_div(a, b, c)  bc_append(bc, I3(OP_DIV, a, b, c))
#define bc_addi(a, b, c) bc_append(bc, I3(OP_ADDI, a, b, c))
#define bc_subi(a, b, c) bc_append(bc, I3(OP_SUBI, a, b, c))
#define bc_muli(a, b, c) bc_append(bc, I3(OP_MULI, a, b, c))
#define bc_divi(a, b, c) bc_append(bc, I3(OP_DIVI, a, b, c))
#define bc_neg(a, b)     bc_append(bc, I2(OP_NEG, a, b))

#define bc_shl(a, b)     bc_append(bc, I2(OP_SHL, a, b))
#define bc_shli(a, b)    bc_append(bc, I2(OP_SHLI, a, b))
#define bc_shr(a, b)     bc_append(bc, I2(OP_SHR, a, b))
#define bc_shri(a, b)    bc_append(bc, I2(OP_SHRI, a, b))
#define bc_and(a, b)     bc_append(bc, I2(OP_AND, a, b))
#define bc_or(a, b)      bc_append(bc, I2(OP_OR, a, b))
#define bc_xor(a, b)     bc_append(bc, I2(OP_XOR, a, b))
#define bc_not(a, b)     bc_append(bc, I2(OP_NOT, a, b))

#define bc_eq(a, b, c)   bc_append(bc, I3(OP_EQ, a, b, c))
#define bc_le(a, b, c)   bc_append(bc, I3(OP_LE, a, b, c))
#define bc_lt(a, b, c)   bc_append(bc, I3(OP_LT, a, b, c))
#define bc_eqi(a, b, c)  bc_append(bc, I3(OP_EQI, a, b, c))
#define bc_gti(a, b, c)  bc_append(bc, I3(OP_GTI, a, b, c))
#define bc_gei(a, b, c)  bc_append(bc, I3(OP_GEI, a, b, c))
#define bc_lti(a, b, c)  bc_append(bc, I3(OP_LTI, a, b, c))
#define bc_lei(a, b, c)  bc_append(bc, I3(OP_LEI, a, b, c))
#define bc_land(a, b, c) bc_append(bc, I3(OP_LAND, a, b, c))
#define bc_lor(a, b, c)  bc_append(bc, I3(OP_LOR, a, b, c))
#define bc_lnot(a, b, c) bc_append(bc, I3(OP_LNOT, a, b, c))

#define bc_test(a, b)    bc_append(bc, I2(OP_TEST, a, b))
#define bc_jmpi(a)       bc_append(bc, I1(OP_JMPI, a))

#define bc_echo(a)       bc_append(bc, I1(OP_ECHO, a))
#define bc_hlt(a)        bc_append(bc, I0(OP_HLT))

#define is_instant8(e) (e->type == T_LONG && e->v.l >= 0 && e->v.l <= 255)
#define is_instant16(e) (e->type == T_LONG && e->v.l >= 0 && e->v.l <= 65535)

static int bc_identifier(lgx_ast_node_t *node, lgx_val_t *expr) {
    lgx_val_t *v;
    lgx_str_ref_t s;

    s.buffer = (unsigned char *)((lgx_ast_node_token_t *)node)->tk_start;
    s.length = ((lgx_ast_node_token_t *)node)->tk_length;
    // todo 非局部变量需要特殊处理
    v = lgx_scope_val_get(node, &s);

    expr->type = T_IDENTIFIER;
    expr->u.reg = v->u.reg;

    return 0;
}

static int bc_number(lgx_ast_node_t *node, lgx_val_t *expr) {
    // TODO 处理浮点数
    expr->type = T_LONG;
    expr->u.l = atol(((lgx_ast_node_token_t *)node)->tk_start);

    return 0;
}

static int bc_expr_unary(lgx_bc_t *bc, int op, lgx_val_t *e, lgx_val_t *e1) {
    e->type = T_REGISTER;
    e->v.l = reg_pop(bc);

    switch (op) {
        case '!': return bc_lnot(e->v.l, e1->->v.l);
        case '~': return bc_not(e->v.l, e1->->v.l);
        case '-': return bc_neg(e->v.l, e1->->v.l);
        default:
            // error
            return 1;
    }
}

static int bc_expr_binary(lgx_bc_t *bc, int op, lgx_val_t *e, lgx_val_t *e1, lgx_val_t *e2) {
    e->type = T_REGISTER;
    e->v.l = reg_pop(bc);

    switch (op) {
        case '+':
            if (is_instant8(e2)) {
                return bc_addi(e->v.l, e1->->v.l, e2->v.l);
            } else {
                if (e2->type == T_REGISTER || e2->type == T_IDENTIFIER) {
                    return bc_add(e->v.l, e1->->v.l, e2->v.l);
                } else {
                    // todo 从常量表中读取
                    return 1;
                }
            }
        case '-':
            if (is_instant8(e2)) {
                return bc_subi(e->v.l, e1->->v.l, e2->v.l);
            } else {
                if (e2->type == T_REGISTER || e2->type == T_IDENTIFIER) {
                    return bc_sub(e->v.l, e1->->v.l, e2->v.l);
                } else {
                    // todo 从常量表中读取
                    return 1;
                }
            }
        case '*':
            if (is_instant8(e2)) {
                return bc_muli(e->v.l, e1->->v.l, e2->v.l);
            } else {
                if (e2->type == T_REGISTER || e2->type == T_IDENTIFIER) {
                    return bc_mul(e->v.l, e1->->v.l, e2->v.l);
                } else {
                    // todo 从常量表中读取
                    return 1;
                }
            }
        case '/':
            if (is_instant8(e2)) {
                return bc_divi(e->v.l, e1->->v.l, e2->v.l);
            } else {
                if (e2->type == T_REGISTER || e2->type == T_IDENTIFIER) {
                    return bc_div(e->v.l, e1->->v.l, e2->v.l);
                } else {
                    // todo 从常量表中读取
                    return 1;
                }
            }
        case '%':
            return 1;
        case '>':
            if (is_instant8(e2)) {
                return bc_gti(e->v.l, e1->->v.l, e2->v.l);
            } else {
                if (e2->type == T_REGISTER || e2->type == T_IDENTIFIER) {
                    return bc_lt(e->v.l, e2->v.l, e1->->v.l);
                } else {
                    // todo 从常量表中读取
                    return 1;
                }
            }
        case '<':
            if (is_instant8(e2)) {
                return bc_lti(e->v.l, e1->->v.l, e2->v.l);
            } else {
                if (e2->type == T_REGISTER || e2->type == T_IDENTIFIER) {
                    return bc_lt(e->v.l, e1->v.l, e2->->v.l);
                } else {
                    // todo 从常量表中读取
                    return 1;
                }
            }
        default:
            // error
            return 1;
    }
}

static int bc_expr(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    switch (node->type) {
        case STRING_TOKEN:
            break;
        case NUMBER_TOKEN:
            return bc_number(node, e);
        case IDENTIFIER_TOKEN:
            return bc_identifier(node, e);
        case CONDITIONAL_EXPRESSION:
            break;
        case BINARY_EXPRESSION:{
            lgx_val_t e1, e2;
            if (bc_expr(bc, node->child[0], &e1)) {
                return 1;
            }
            if (bc_expr(bc, node->child[1], &e2)) {
                return 1;
            }

            if ( (e1.type == T_LONG || e1.type == T_DOUBLE || e1.type == T_BOOL) &&
                (e2.type == T_LONG || e2.type == T_DOUBLE || e2.type == T_BOOL) ) {
                if (lgx_op_binary(node->u.op, e, &e1, &e2)) {
                    return 1;
                }
            } else if (e1.type == T_REGISTER || e1.type == T_IDENTIFIER) {
                if (bc_expr_binary(bc, node->u.op, e, &e1, &e2)) {
                    return 1;
                }
            } else {
                if (bc_expr_binary(bc, node->u.op, e, &e2, &e1)) {
                    return 1;
                }
            }
            break;
        }
        case UNARY_EXPRESSION:{
            lgx_val_t e1;
            if (bc_expr(bc, node->child[0], &e1)) {
                return 1;
            }

            if (e1.type == T_LONG || e1.type == T_DOUBLE || e1.type == T_BOOL) {
                if (lgx_op_unary(node->u.op, e, &e1)) {
                    return 1;
                }
            } else {
                if (bc_expr_unary(bc, node->u.op, e, &e1)) {
                    return 1;
                }
            }
            break;
        }
        default:
            // error
            return 1;
    }

    return 0;
}

static void bc_stat(lgx_bc_t *bc, lgx_ast_node_t *node) {
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
            bc_expr(bc, r, node->child[0]);
            
            // 写入条件跳转
            unsigned pos2 = bc->bc_top;
            bc_test(r, 0);
            reg_push(bc, r);

            bc_gen(bc, node->child[1]);

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
    expr_t e0, e1;

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
        bc_expr_unary(bc, node->child[1], &e0);
    }
    // 如果右侧是双目表达式
    else if (node->child[0]->type == BINARY_EXPRESSION) {
        bc_expr_binary(bc, node->child[1], &e0);
    }
            break;
        // Declaration
        case FUNCTION_DECLARATION:{
            // 执行一次赋值操作
               
            break;
        }
        case VARIABLE_DECLARATION:{
            // 如果声明中附带初始化，则执行一次赋值操作
            if (node->child[1]) {
                bc_stat_assign(bc, node);
            }
            break;
        }
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