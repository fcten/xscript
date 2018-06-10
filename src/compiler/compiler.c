#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../parser/scope.h"
#include "../common/bytecode.h"
#include "../common/val.h"
#include "../common/operator.h"
#include "compiler.h"

void bc_error(lgx_bc_t *bc, const char *fmt, ...) {
    va_list   args;

    if (bc->errno) {
        return;
    }

    va_start(args, fmt);
    bc->err_len = vsnprintf(bc->err_info, 256, fmt, args);
    va_end(args);
    
    bc->errno = 1;
}


static void reg_push(lgx_bc_t *bc, unsigned char i) {
    bc->regs[bc->reg_top] = i;
    bc->reg_top ++;
}

static unsigned char reg_pop(lgx_bc_t *bc) {
    // TODO 限制变量数量
    bc->reg_top --;
    return bc->regs[bc->reg_top];
}

static void reg_free(lgx_bc_t *bc, lgx_val_t *e) {
    if (e->type == T_REGISTER) {
        reg_push(bc, e->v.l);
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

static void bc_set_pa(lgx_bc_t *bc, unsigned pos, unsigned pa) {
    bc->bc[pos] &= 0xFFFF00FF;
    bc->bc[pos] |= pa << 8;
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
#define bc_lnot(a, b)    bc_append(bc, I2(OP_LNOT, a, b))

#define bc_test(a, b)    bc_append(bc, I2(OP_TEST, a, b))
#define bc_jmpi(a)       bc_append(bc, I1(OP_JMPI, a))

#define bc_echo(a)       bc_append(bc, I1(OP_ECHO, a))
#define bc_hlt(a)        bc_append(bc, I0(OP_HLT))

#define is_instant8(e) ((e)->type == T_LONG && (e)->v.l >= 0 && (e)->v.l <= 255)
#define is_instant16(e) ((e)->type == T_LONG && (e)->v.l >= 0 && (e)->v.l <= 65535)

static int bc_identifier(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *expr) {
    lgx_val_t *v;
    lgx_str_ref_t s;

    s.buffer = (unsigned char *)((lgx_ast_node_token_t *)node)->tk_start;
    s.length = ((lgx_ast_node_token_t *)node)->tk_length;
    // todo 非局部变量需要特殊处理
    v = lgx_scope_val_get(node, &s);
    if (v) {
        expr->type = T_IDENTIFIER;
        expr->v.l = v->u.reg;

        return 0;
    } else {
        bc_error(bc, "[Error] [Line:%d] `%.*s` is not defined\n", node->line, s.length, s.buffer);
        return 1;
    }
}

static int bc_number(lgx_ast_node_t *node, lgx_val_t *expr) {
    // TODO 处理浮点数
    expr->type = T_LONG;
    expr->v.l = atol(((lgx_ast_node_token_t *)node)->tk_start);

    return 0;
}

static int bc_true(lgx_ast_node_t *node, lgx_val_t *expr) {
    // TODO 处理浮点数
    expr->type = T_BOOL;
    expr->v.l = 1;

    return 0;
}

static int bc_false(lgx_ast_node_t *node, lgx_val_t *expr) {
    // TODO 处理浮点数
    expr->type = T_BOOL;
    expr->v.l = 0;

    return 0;
}

static int bc_expr_unary(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e, lgx_val_t *e1) {
    e->type = T_REGISTER;
    e->v.l = reg_pop(bc);

    switch (node->u.op) {
        case '!': return bc_lnot(e->v.l, e1->v.l);
        case '~': return bc_not(e->v.l, e1->v.l);
        case '-': return bc_neg(e->v.l, e1->v.l);
        default:
            bc_error(bc, "[Error] [Line:%d] unknown unary operation\n", node->line);
            return 1;
    }
}

static int bc_expr_binary(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e, lgx_val_t *e1, lgx_val_t *e2) {
    e->type = T_REGISTER;
    e->v.l = reg_pop(bc);

    switch (node->u.op) {
        case '+':
            if (is_instant8(e2)) {
                return bc_addi(e->v.l, e1->v.l, e2->v.l);
            } else {
                if (e2->type == T_REGISTER || e2->type == T_IDENTIFIER) {
                    return bc_add(e->v.l, e1->v.l, e2->v.l);
                } else {
                    // todo 从常量表中读取
                    return 1;
                }
            }
        case '-':
            if (is_instant8(e2)) {
                return bc_subi(e->v.l, e1->v.l, e2->v.l);
            } else {
                if (e2->type == T_REGISTER || e2->type == T_IDENTIFIER) {
                    return bc_sub(e->v.l, e1->v.l, e2->v.l);
                } else {
                    // todo 从常量表中读取
                    return 1;
                }
            }
        case '*':
            if (is_instant8(e2)) {
                return bc_muli(e->v.l, e1->v.l, e2->v.l);
            } else {
                if (e2->type == T_REGISTER || e2->type == T_IDENTIFIER) {
                    return bc_mul(e->v.l, e1->v.l, e2->v.l);
                } else {
                    // todo 从常量表中读取
                    return 1;
                }
            }
        case '/':
            if (is_instant8(e2)) {
                return bc_divi(e->v.l, e1->v.l, e2->v.l);
            } else {
                if (e2->type == T_REGISTER || e2->type == T_IDENTIFIER) {
                    return bc_div(e->v.l, e1->v.l, e2->v.l);
                } else {
                    // todo 从常量表中读取
                    return 1;
                }
            }
        case '%':
            return 1;
        case TK_SHL:
            return 1;
        case TK_SHR:
            return 1;
        case '>':
            if (is_instant8(e2)) {
                return bc_gti(e->v.l, e1->v.l, e2->v.l);
            } else {
                if (e2->type == T_REGISTER || e2->type == T_IDENTIFIER) {
                    return bc_lt(e->v.l, e2->v.l, e1->v.l);
                } else {
                    // todo 从常量表中读取
                    return 1;
                }
            }
        case '<':
            if (is_instant8(e2)) {
                return bc_lti(e->v.l, e1->v.l, e2->v.l);
            } else {
                if (e2->type == T_REGISTER || e2->type == T_IDENTIFIER) {
                    return bc_lt(e->v.l, e1->v.l, e2->v.l);
                } else {
                    // todo 从常量表中读取
                    return 1;
                }
            }
        case TK_GE:
            if (is_instant8(e2)) {
                return bc_gei(e->v.l, e1->v.l, e2->v.l);
            } else {
                if (e2->type == T_REGISTER || e2->type == T_IDENTIFIER) {
                    return bc_le(e->v.l, e2->v.l, e1->v.l);
                } else {
                    // todo 从常量表中读取
                    return 1;
                }
            }
        case TK_LE:
            if (is_instant8(e2)) {
                return bc_lei(e->v.l, e1->v.l, e2->v.l);
            } else {
                if (e2->type == T_REGISTER || e2->type == T_IDENTIFIER) {
                    return bc_le(e->v.l, e1->v.l, e2->v.l);
                } else {
                    // todo 从常量表中读取
                    return 1;
                }
            }
        case TK_EQ:
            if (is_instant8(e2)) {
                return bc_eqi(e->v.l, e1->v.l, e2->v.l);
            } else {
                if (e2->type == T_REGISTER || e2->type == T_IDENTIFIER) {
                    return bc_eq(e->v.l, e1->v.l, e2->v.l);
                } else {
                    // todo 从常量表中读取
                    return 1;
                }
            }
        case TK_NE: return 1;
        case '&': return 1;
        case '^': return 1;
        case '|': return 1;
        case TK_AND: return 1;
        case TK_OR: return 1;
        case TK_INDEX:
        case TK_ATTR:
        default:
            // error
            bc_error(bc, "[Error] [Line:%d] unknown binary operation\n", node->line);
            return 1;
    }
}

static lgx_ast_node_t *jmp_find_loop(lgx_ast_node_t *node) {
    while (node && node->type != FOR_STATEMENT && node->type != WHILE_STATEMENT && node->type != DO_WHILE_STATEMENT) {
        node = node->parent;
    }
    return node;
}

static lgx_ast_node_t *jmp_find_loop_or_switch(lgx_ast_node_t *node) {
    while (node && node->type != FOR_STATEMENT && node->type != WHILE_STATEMENT && node->type != DO_WHILE_STATEMENT && node->type != SWITCH_CASE_STATEMENT) {
        node = node->parent;
    }
    return node;
}

static int jmp_add(lgx_bc_t *bc, lgx_ast_node_t *node) {
    lgx_ast_node_t *loop = NULL;
    if (node->type == CONTINUE_STATEMENT) {
        loop = jmp_find_loop(node);
    } else if (node->type == BREAK_STATEMENT) {
        loop = jmp_find_loop_or_switch(node);
    } else {
        bc_error(bc, "[Error] [Line:%d] break or continue statement expected\n", node->line);
        return 1;
    }
    if (!loop) {
        bc_error(bc, "[Error] [Line:%d] illegal break or continue statement\n", node->line);
        return 1;
    }

    // 初始化
    if (!loop->u.jmps) {
        loop->u.jmps = malloc(sizeof(lgx_ast_node_list_t));
        if (loop->u.jmps) {
            lgx_list_init(&loop->u.jmps->head);
            loop->u.jmps->node = NULL;
        } else {
            bc_error(bc, "[Error] [Line:%d] out of memory\n", node->line);
            return 1;
        }
    }

    lgx_ast_node_list_t *n = malloc(sizeof(lgx_ast_node_list_t));
    if (n) {
        n->node = node;
        lgx_list_init(&n->head);
        lgx_list_add_tail(&n->head, &loop->u.jmps->head);

        return 0;
    } else {
        bc_error(bc, "[Error] [Line:%d] out of memory\n", node->line);
        return 1;
    }
}

static int jmp_fix(lgx_bc_t *bc, lgx_ast_node_t *node, unsigned start, unsigned end) {
    if (node->type != FOR_STATEMENT &&
        node->type != WHILE_STATEMENT && 
        node->type != DO_WHILE_STATEMENT &&
        node->type != SWITCH_CASE_STATEMENT) {
        bc_error(bc, "[Error] [Line:%d] switch or loop statement expected\n", node->line);
        return 1;
    }

    if (!node->u.jmps) {
        return 0;
    }

    lgx_ast_node_list_t *n;
    lgx_list_for_each_entry(n, lgx_ast_node_list_t, &node->u.jmps->head, head) {
        if (n->node->type == BREAK_STATEMENT) {
            bc_set(bc, n->node->u.pos, I1(OP_JMPI, end));
        } else if (n->node->type == CONTINUE_STATEMENT) {
            bc_set(bc, n->node->u.pos, I1(OP_JMPI, start));
        } else {
            // error
            bc_error(bc, "[Error] [Line:%d] break or continue statement expected\n", n->node->line);
            return 1;
        }
    }

    return 0;
}

static int bc_expr(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    switch (node->type) {
        case STRING_TOKEN:
            break;
        case NUMBER_TOKEN:
            return bc_number(node, e);
        case IDENTIFIER_TOKEN:
            return bc_identifier(bc, node, e);
        case TRUE_TOKEN:
            return bc_true(node, e);
        case FALSE_TOKEN:
            return bc_false(node, e);
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
                if (bc_expr_binary(bc, node, e, &e1, &e2)) {
                    return 1;
                }
            } else {
                if (bc_expr_binary(bc, node, e, &e2, &e1)) {
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
                if (bc_expr_unary(bc, node, e, &e1)) {
                    return 1;
                }
            }
            break;
        }
        case CALL_EXPRESSION:{
            break;
        }
        default:
            bc_error(bc, "[Error] [Line:%d] expression expected\n", node->line);
            return 1;
    }

    return 0;
}

static int bc_stat(lgx_bc_t *bc, lgx_ast_node_t *node) {
    switch(node->type) {
        // Statement
        case BLOCK_STATEMENT:{
            int i;
            // 为当前作用域的变量分配寄存器
            for(i = 0; i < node->u.symbols->table_offset; i++) {
                node->u.symbols->table[i].v.u.reg = reg_pop(bc);
            }

            for(i = 0; i < node->children; i++) {
                if (bc_stat(bc, node->child[i])) {
                    return 1;
                }
            }

            // 释放局部变量的寄存器
            for(i = node->u.symbols->table_offset - 1; i >= 0; i--) {
                reg_push(bc, node->u.symbols->table[i].v.u.reg);
            }
            break;
        }
        case IF_STATEMENT:{
            lgx_val_t e;
            if (bc_expr(bc, node->child[0], &e)) {
                return 1;
            }

            if (e.type == T_BOOL) {
                if (e.v.l == 0) {
                    break;
                } else {
                    if (bc_stat(bc, node->child[1])) {
                        return 1;
                    }
                }
            } else if (e.type == T_IDENTIFIER || e.type == T_REGISTER) {
                unsigned pos = bc->bc_top; // 跳出指令位置
                bc_test(e.v.l, 0);
                reg_free(bc, &e);

                if (bc_stat(bc, node->child[1])) {
                    return 1;
                }

                bc_set(bc, pos, I2(OP_TEST, e.v.l, bc->bc_top));
            } else {
                // error
            }
            break;
        }
        case IF_ELSE_STATEMENT:{
            lgx_val_t e;
            if (bc_expr(bc, node->child[0], &e)) {
                return 1;
            }

            if (e.type == T_BOOL) {
                if (e.v.l == 0) {
                    if( bc_stat(bc, node->child[2])) {
                        return 1;
                    }
                } else {
                    if (bc_stat(bc, node->child[1])) {
                        return 1;
                    }
                }
            } else if (e.type == T_IDENTIFIER || e.type == T_REGISTER) {
                unsigned pos1 = bc->bc_top; // 跳转指令位置
                bc_test(e.v.l, 0);
                reg_free(bc, &e);

                if (bc_stat(bc, node->child[1])) {
                    return 1;
                }

                unsigned pos2 = bc->bc_top; // 跳转指令位置
                bc_jmpi(0);
                bc_set(bc, pos1, I2(OP_TEST, e.v.l, bc->bc_top));

                if (bc_stat(bc, node->child[2])) {
                    return 1;
                }

                bc_set(bc, pos2, I1(OP_JMPI, bc->bc_top));
            } else {
                // error
            }
            break;
        }
        case FOR_STATEMENT:

            break;
        case WHILE_STATEMENT:{
            unsigned start = bc->bc_top; // 循环起始位置
            lgx_val_t e;
            if (bc_expr(bc, node->child[0], &e)) {
                return 1;
            }

            if (e.type == T_BOOL) {
                if (e.v.l == 0) {
                    break;
                } else {
                    if (bc_stat(bc, node->child[1])) {
                        return 1;
                    }
                    // 写入无条件跳转
                    bc_jmpi(start);
                }
            } else if (e.type == T_IDENTIFIER || e.type == T_REGISTER) {
                unsigned pos = bc->bc_top; // 循环跳出指令位置
                bc_test(e.v.l, 0);
                reg_free(bc, &e);

                if (bc_stat(bc, node->child[1])) {
                    return 1;
                }
                // 写入无条件跳转
                bc_jmpi(start);
                // 更新条件跳转
                bc_set(bc, pos, I2(OP_TEST, e.v.l, bc->bc_top));
            } else {
                // error
            }

            jmp_fix(bc, node, start, bc->bc_top);
            break;
        }
        case DO_WHILE_STATEMENT:{
            unsigned start = bc->bc_top; // 循环起始位置

            if (bc_stat(bc, node->child[0])) {
                return 1;
            }

            lgx_val_t e;
            if (bc_expr(bc, node->child[1], &e)) {
                return 1;
            }

            if (e.type == T_BOOL) {
                if (e.v.l == 0) {
                    break;
                } else {
                    // 写入无条件跳转
                    bc_jmpi(start);
                }
            } else if (e.type == T_IDENTIFIER || e.type == T_REGISTER) {
                unsigned pos = bc->bc_top;
                bc_test(e.v.l, 0);
                bc_jmpi(start);
                reg_free(bc, &e);

                bc_set(bc, pos, I2(OP_TEST, e.v.l, bc->bc_top));
            } else {
                // error
            }

            jmp_fix(bc, node, start, bc->bc_top);
            break;
        }
        case CONTINUE_STATEMENT:
            // 只能出现在循环语句中
            if (jmp_add(bc, node)) {
                // error
                break;
            }
            node->u.pos = bc->bc_top;
            bc_jmpi(0);
            break;
        case BREAK_STATEMENT:
            // 只能出现在循环语句和 switch 语句中
            if (jmp_add(bc, node)) {
                // error
                break;
            }
            node->u.pos = bc->bc_top;
            bc_jmpi(0);
            break;
        case SWITCH_CASE_STATEMENT:

            break;
        case RETURN_STATEMENT:
            // 计算返回值
        
            // 释放参数与局部变量

            // 更新返回值寄存器

            // 写入返回指令
            break;
        case ECHO_STATEMENT:{
            lgx_val_t e;
            if (bc_expr(bc, node->child[0], &e)) {
                return 1;
            }

            bc_echo(e.v.l);
            reg_free(bc, &e);
            break;
        }
        case VARIABLE_DECLARATION:{
            if (!node->child[1]) {
                break;
            }
            // 如果声明中附带初始化，则执行一次赋值操作
        }
        case ASSIGNMENT_STATEMENT: {
            lgx_val_t e0, e1;

            if (bc_identifier(bc, node->child[0], &e0)) {
                return 1;
            }
            if (bc_expr(bc, node->child[1], &e1)) {
                return 1;
            }
            
            if (e1.type == T_LONG || e1.type == T_DOUBLE) {
                if (is_instant16(&e1)){
                    bc_movi(e0.v.l, e1.v.l);
                } else {
                    // todo 常量表读取
                }
            } else if (e1.type == T_IDENTIFIER) {
                bc_mov(e0.v.l, e1.v.l);
            } else if (e1.type == T_REGISTER) {
                reg_free(bc, &e1);
                bc_set_pa(bc, bc->bc_top-1, e0.v.l);
            } else {
                return 1;
            }
            break;
        }
        // Declaration
        case FUNCTION_DECLARATION:{
            unsigned start = bc->bc_top; // 函数起始位置
            bc_jmpi(0);


            bc_set(bc, start, I1(OP_JMPI, bc->bc_top));

            // 执行一次赋值操作
            
            break;
        }
        default:
            bc_error(bc, "[Error] [Line:%d] unknown ast-node type\n", node->line);
            return 1;
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

    bc->err_info = malloc(256);
    bc->err_len = 0;

    if (bc_stat(bc, ast->root)) {
        return 1;
    }

    bc_hlt();    
    return 0;
}