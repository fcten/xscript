#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../parser/scope.h"
#include "../common/val.h"
#include "../common/operator.h"
#include "register.h"
#include "compiler.h"
#include "code.h"
#include "constant.h"

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

static int bc_identifier(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *expr) {
    lgx_val_t *v;
    lgx_str_ref_t s;

    s.buffer = ((lgx_ast_node_token_t *)node)->tk_start;
    s.length = ((lgx_ast_node_token_t *)node)->tk_length;

    v = lgx_scope_local_val_get(node, &s);
    if (v) {
        *expr = *v;

        return 0;
    }

    v = lgx_scope_global_val_get(node, &s);
    if (v) {
        *expr = *v;

        return 0;
    }

    bc_error(bc, "[Error] [Line:%d] `%.*s` is not defined\n", node->line, s.length, s.buffer);
    return 1;
}

static int bc_number(lgx_ast_node_t *node, lgx_val_t *expr) {
    // TODO 处理浮点数
    expr->type = T_LONG;
    expr->v.l = atol(((lgx_ast_node_token_t *)node)->tk_start);

    expr->u.reg.type = 0;
    expr->u.reg.reg = 0;

    return 0;
}

static int bc_true(lgx_ast_node_t *node, lgx_val_t *expr) {
    // TODO 处理浮点数
    expr->type = T_BOOL;
    expr->v.l = 1;

    expr->u.reg.type = 0;
    expr->u.reg.reg = 0;

    return 0;
}

static int bc_false(lgx_ast_node_t *node, lgx_val_t *expr) {
    // TODO 处理浮点数
    expr->type = T_BOOL;
    expr->v.l = 0;

    expr->u.reg.type = 0;
    expr->u.reg.reg = 0;

    return 0;
}

static int bc_string(lgx_ast_node_t *node, lgx_val_t *expr) {
    expr->type = T_STRING;
    expr->v.str = lgx_str_new(((lgx_ast_node_token_t *)node)->tk_start+1, ((lgx_ast_node_token_t *)node)->tk_length-2);

    expr->u.reg.type = 0;
    expr->u.reg.reg = 0;

    return 0;
}

static int bc_expr_unary(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e, lgx_val_t *e1) {
    e->u.reg.type = R_TEMP;
    e->u.reg.reg = reg_pop(bc);

    switch (node->u.op) {
        case '!': bc_lnot(bc, e, e1); break;
        case '~': bc_not(bc, e, e1); break;
        case '-': bc_neg(bc, e, e1); break;
        default:
            bc_error(bc, "[Error] [Line:%d] unknown unary operation\n", node->line);
            return 1;
    }

    return 0;
}

static int bc_expr_binary(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e, lgx_val_t *e1, lgx_val_t *e2) {
    e->u.reg.type = R_TEMP;
    e->u.reg.reg = reg_pop(bc);

    switch (node->u.op) {
        case '+': bc_add(bc, e, e1, e2); break;
        case '-': bc_sub(bc, e, e1, e2); break;
        case '*': bc_mul(bc, e, e1, e2); break;
        case '/': bc_div(bc, e, e1, e2); break;
        case '%': return 1;
        case TK_SHL: bc_shl(bc, e, e1, e2); break;
        case TK_SHR: bc_shr(bc, e, e1, e2); break;
        case '>': bc_gt(bc, e, e1, e2); break;
        case '<': bc_lt(bc, e, e1, e2); break;
        case TK_GE: bc_ge(bc, e, e1, e2); break;
        case TK_LE: bc_le(bc, e, e1, e2); break;
        case TK_EQ: bc_eq(bc, e, e1, e2); break;
        case TK_NE: bc_ne(bc, e, e1, e2); break;
        case '&': return 1;
        case '^': return 1;
        case '|': return 1;
        case TK_AND: return 1;
        case TK_OR: return 1;
        case '=': bc_mov(bc, e1, e2); break;
        case TK_INDEX:
        case TK_ATTR:
        default:
            // error
            bc_error(bc, "[Error] [Line:%d] unknown binary operation\n", node->line);
            return 1;
    }

    return 0;
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
            bc_set_pe(bc, n->node->u.pos, end);
        } else if (n->node->type == CONTINUE_STATEMENT) {
            bc_set_pe(bc, n->node->u.pos, start);
        } else {
            // error
            bc_error(bc, "[Error] [Line:%d] break or continue statement expected\n", n->node->line);
            return 1;
        }
    }

    return 0;
}

static int bc_expr(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e);

static int bc_expr_binary_logic(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    bc_error(bc, "[Error] [Line:%d] undo features\n", node->line);
    return 1;
}

static int bc_expr_binary_math(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    lgx_val_t e1, e2;

    if (bc_expr(bc, node->child[0], &e1)) {
        return 1;
    }

    if ( !is_register(&e1) && !is_number(&e1) ) {
        bc_error(bc, "[Error] [Line:%d] makes number from %s without a cast\n", node->line, lgx_val_typeof(&e1));
        return 1;
    }

    if (bc_expr(bc, node->child[1], &e2)) {
        return 1;
    }

    if ( !is_register(&e2) && !is_number(&e2) ) {
        bc_error(bc, "[Error] [Line:%d] makes number from %s without a cast\n", node->line, lgx_val_typeof(&e2));
        return 1;
    }

    if ( !is_register(&e1) && !is_register(&e2) ) {
        if (lgx_op_binary(node->u.op, e, &e1, &e2)) {
            return 1;
        }
    } else {
        if (bc_expr_binary(bc, node, e, &e1, &e2)) {
            return 1;
        }
    }

    return 0;
}

static int bc_expr_binary_relation(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    return bc_expr_binary_math(bc, node, e);
}

static int bc_expr_binary_bitwise(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    lgx_val_t e1, e2;

    if (bc_expr(bc, node->child[0], &e1)) {
        return 1;
    }

    if ( !is_register(&e1) && e1.type != T_LONG ) {
        bc_error(bc, "[Error] [Line:%d] makes integer from %s without a cast\n", node->line, lgx_val_typeof(&e1));
        return 1;
    }

    if (bc_expr(bc, node->child[1], &e2)) {
        return 1;
    }

    if ( !is_register(&e2) && e2.type != T_LONG ) {
        bc_error(bc, "[Error] [Line:%d] makes integer from %s without a cast\n", node->line, lgx_val_typeof(&e2));
        return 1;
    }

    if ( e1.type == T_LONG && e2.type == T_LONG ) {
        if (lgx_op_binary(node->u.op, e, &e1, &e2)) {
            return 1;
        }
    } else {
        if (bc_expr_binary(bc, node, e, &e1, &e2)) {
            return 1;
        }
    }

    return 0;
}

static int bc_expr_binary_assignment(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    lgx_val_t e1, e2;

    if (bc_expr(bc, node->child[0], &e1)) {
        return 1;
    }

    if ( e1.u.reg.type == R_LOCAL ) {
        if (bc_expr(bc, node->child[1], &e2)) {
            return 1;
        }

        if (bc_expr_binary(bc, node, e, &e1, &e2)) {
            return 1;
        }
    } else if (e1.u.reg.type == R_GLOBAL) {
        // TODO 全局变量赋值
    } else {
        bc_error(bc, "[Error] [Line:%d] variable expected\n", node->line);
        return 1;
    }

    return 0;
}

static int bc_expr_binary_call(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    // TODO 传入参数
    lgx_val_t e1;

    if (bc_expr(bc, node->child[0], &e1)) {
        return 1;
    }

    bc_call(bc, &e1);

    // TODO 返回值

    return 0;
}

static int bc_expr(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    switch (node->type) {
        case STRING_TOKEN:
            return bc_string(node, e);
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
            switch (node->u.op) {
                case TK_AND: case TK_OR:
                    // 逻辑运算符 && 和 || 存在熔断特性
                    if (bc_expr_binary_logic(bc, node, e)) {
                        return 1;
                    }
                    break;
                case '+': case '-': case '*': case '/': case '%':
                    if (bc_expr_binary_math(bc, node, e)) {
                        return 1;
                    }
                    break;
                case TK_EQ: case TK_NE: case '>': case '<': case TK_GE: case TK_LE:
                    if (bc_expr_binary_relation(bc, node, e)) {
                        return 1;
                    }
                    break;
                case TK_SHL: case TK_SHR: case '&': case '|': case '^':
                    if (bc_expr_binary_bitwise(bc, node, e)) {
                        return 1;
                    }
                    break;
                case '=':
                    if (bc_expr_binary_assignment(bc, node, e)) {
                        return 1;
                    }
                    break;
                case TK_CALL:
                    if (bc_expr_binary_call(bc, node, e)) {
                        return 1;
                    }
                    break;
                case TK_INDEX:
                    // TODO
                    break;
                case TK_ATTR:
                    // TODO
                    break;
                default:
                    bc_error(bc, "[Error] [Line:%d] unknown operator `%c`\n", node->line, node->u.op);
                    return 1;
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
                node->u.symbols->table[i].v.u.reg.type = R_LOCAL;
                node->u.symbols->table[i].v.u.reg.reg = reg_pop(bc);
            }

            for(i = 0; i < node->children; i++) {
                if (bc_stat(bc, node->child[i])) {
                    return 1;
                }
            }

            // 释放局部变量的寄存器
            for(i = node->u.symbols->table_offset - 1; i >= 0; i--) {
                reg_push(bc, node->u.symbols->table[i].v.u.reg.reg);
            }
            break;
        }
        case IF_STATEMENT:{
            lgx_val_t e;
            memset(&e, 0, sizeof(e));
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
            } else if (is_register(&e)) {
                unsigned pos = bc->bc_top; // 跳出指令位置
                bc_test(bc, &e, 0);
                reg_free(bc, &e);

                if (bc_stat(bc, node->child[1])) {
                    return 1;
                }

                bc_set_pd(bc, pos, bc->bc_top);
            } else {
                // error
            }
            break;
        }
        case IF_ELSE_STATEMENT:{
            lgx_val_t e;
            memset(&e, 0, sizeof(e));
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
            } else if (is_register(&e)) {
                unsigned pos1 = bc->bc_top; // 跳转指令位置
                bc_test(bc, &e, 0);
                reg_free(bc, &e);

                if (bc_stat(bc, node->child[1])) {
                    return 1;
                }

                unsigned pos2 = bc->bc_top; // 跳转指令位置
                bc_jmp(bc, 0);
                bc_set_pd(bc, pos1, bc->bc_top);

                if (bc_stat(bc, node->child[2])) {
                    return 1;
                }

                bc_set_pe(bc, pos2, bc->bc_top);
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
            memset(&e, 0, sizeof(e));
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
                    bc_jmp(bc, start);
                }
            } else if (is_register(&e)) {
                unsigned pos = bc->bc_top; // 循环跳出指令位置
                bc_test(bc, &e, 0);
                reg_free(bc, &e);

                if (bc_stat(bc, node->child[1])) {
                    return 1;
                }
                // 写入无条件跳转
                bc_jmp(bc, start);
                // 更新条件跳转
                bc_set_pd(bc, pos, bc->bc_top);
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
            memset(&e, 0, sizeof(e));
            if (bc_expr(bc, node->child[1], &e)) {
                return 1;
            }

            if (e.type == T_BOOL) {
                if (e.v.l == 0) {
                    break;
                } else {
                    // 写入无条件跳转
                    bc_jmp(bc, start);
                }
            } else if (is_register(&e)) {
                unsigned pos = bc->bc_top;
                bc_test(bc, &e, 0);
                bc_jmp(bc, start);
                reg_free(bc, &e);

                bc_set_pd(bc, pos, bc->bc_top);
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
            bc_jmp(bc, 0);
            break;
        case BREAK_STATEMENT:
            // 只能出现在循环语句和 switch 语句中
            if (jmp_add(bc, node)) {
                // error
                break;
            }
            node->u.pos = bc->bc_top;
            bc_jmp(bc, 0);
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
            memset(&e, 0, sizeof(e));
            if (bc_expr(bc, node->child[0], &e)) {
                return 1;
            }

            bc_echo(bc, &e);
            reg_free(bc, &e);
            break;
        }
        case VARIABLE_DECLARATION:{
            if (node->child[1]) {
                // 如果声明中附带初始化，则执行一次赋值操作
                lgx_val_t e;
                memset(&e, 0, sizeof(e));
                if (bc_expr_binary_assignment(bc, node, &e)) {
                    return 1;
                }
            }
            break;
        }
        case EXPRESSION_STATEMENT: {
            lgx_val_t e;
            memset(&e, 0, sizeof(e));
            if (bc_expr(bc, node->child[0], &e)) {
                return 1;
            }
            break;
        }
        // Declaration
        case FUNCTION_DECLARATION:{
            unsigned start = bc->bc_top;
            bc_jmp(bc, 0);

            lgx_val_t *e;
            lgx_str_ref_t s;
            s.buffer = ((lgx_ast_node_token_t *)(node->child[0]))->tk_start;
            s.length = ((lgx_ast_node_token_t *)(node->child[0]))->tk_length;
            e = lgx_scope_global_val_get(node, &s);

            e->type = T_FUNCTION;
            e->v.fun = lgx_fun_new();
            e->v.fun->addr = bc->bc_top;

            // TODO 重置寄存器分配
            bc_stat(bc, node->child[2]);
            // TODO 恢复寄存器分配

            bc_ret(bc);

            bc_set_pe(bc, start, bc->bc_top);

            // 执行一次赋值操作
            bc_load(bc, e, const_get(bc, e));
            
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
    bc->errno = 0;

    lgx_hash_init(&bc->constant, 32);

    if (bc_stat(bc, ast->root)) {
        bc_error(bc, "[Error] unknown error\n");
        return 1;
    }

    bc_hlt(bc);    
    return 0;
}