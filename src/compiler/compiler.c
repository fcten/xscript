#include "../common/common.h"
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
    lgx_str_t s;

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

static int bc_long(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *expr) {
    char *s = ((lgx_ast_node_token_t *)node)->tk_start;

    expr->type = T_LONG;

    // 这里不用担心 s[1] 越界，因为源文件不可能以数字结尾
    if (s[0] == '0') {
        if (s[1] == 'b' || s[1] == 'B') {
            expr->v.l = strtoll(s+2, NULL, 2);
        } else if (s[1] == 'x' || s[1] == 'X') {
            expr->v.l = strtoll(s+2, NULL, 16);
        } else {
            expr->v.l = strtoll(s, NULL, 10);
        }
    } else {
        expr->v.l = strtoll(s, NULL, 10);
    }

    expr->u.reg.type = 0;
    expr->u.reg.reg = 0;

    const_add(bc, expr);

    return 0;
}

static int bc_double(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *expr) {
    expr->type = T_DOUBLE;
    expr->v.d = strtod(((lgx_ast_node_token_t *)node)->tk_start, NULL);

    expr->u.reg.type = 0;
    expr->u.reg.reg = 0;

    const_add(bc, expr);

    return 0;
}

static int bc_true(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *expr) {
    expr->type = T_BOOL;
    expr->v.l = 1;

    expr->u.reg.type = 0;
    expr->u.reg.reg = 0;

    const_add(bc, expr);

    return 0;
}

static int bc_false(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *expr) {
    expr->type = T_BOOL;
    expr->v.l = 0;

    expr->u.reg.type = 0;
    expr->u.reg.reg = 0;

    const_add(bc, expr);

    return 0;
}

static int bc_undefined(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *expr) {
    expr->type = T_UNDEFINED;

    expr->u.reg.type = 0;
    expr->u.reg.reg = 0;

    const_add(bc, expr);

    return 0;
}

static int bc_string(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *expr) {
    expr->type = T_STRING;
    expr->v.str = lgx_str_new(((lgx_ast_node_token_t *)node)->tk_start+1, ((lgx_ast_node_token_t *)node)->tk_length-2);

    expr->u.reg.type = 0;
    expr->u.reg.reg = 0;

    const_add(bc, expr);

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
        case TK_INDEX: bc_array_get(bc, e, e1, e2); break;
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
        loop->u.jmps = xmalloc(sizeof(lgx_ast_node_list_t));
        if (loop->u.jmps) {
            lgx_list_init(&loop->u.jmps->head);
            loop->u.jmps->node = NULL;
        } else {
            bc_error(bc, "[Error] [Line:%d] out of memory\n", node->line);
            return 1;
        }
    }

    lgx_ast_node_list_t *n = xmalloc(sizeof(lgx_ast_node_list_t));
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

static int bc_expr_array(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    e->u.reg.type = R_TEMP;
    e->u.reg.reg = reg_pop(bc);

    bc_array_new(bc, e);

    int i;
    for(i = 0; i < node->children; i++) {
        lgx_val_t expr;
        lgx_val_init(&expr);
        bc_expr(bc, node->child[i], &expr);
        bc_array_add(bc, e, &expr);
        reg_free(bc, &expr);
    }

    return 0;
}

static int bc_expr_binary_logic(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    bc_error(bc, "[Error] [Line:%d] undo features\n", node->line);
    return 1;
}

static int bc_expr_binary_math(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    lgx_val_t e1, e2;
    lgx_val_init(&e1);
    lgx_val_init(&e2);

    if (bc_expr(bc, node->child[0], &e1)) {
        return 1;
    }

    if (node->u.op != TK_EQ && !is_register(&e1) && !is_number(&e1)) {
        bc_error(bc, "[Error] [Line:%d] makes number from %s without a cast\n", node->line, lgx_val_typeof(&e1));
        return 1;
    }

    if (bc_expr(bc, node->child[1], &e2)) {
        return 1;
    }

    if (node->u.op != TK_EQ && !is_register(&e2) && !is_number(&e2) ) {
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

    reg_free(bc, &e1);
    reg_free(bc, &e2);

    return 0;
}

static int bc_expr_binary_relation(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    return bc_expr_binary_math(bc, node, e);
}

static int bc_expr_binary_bitwise(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    lgx_val_t e1, e2;
    lgx_val_init(&e1);
    lgx_val_init(&e2);

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

    reg_free(bc, &e1);
    reg_free(bc, &e2);

    return 0;
}

static int bc_expr_binary_assignment(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    if (node->child[0]->type == IDENTIFIER_TOKEN) {
        lgx_val_t e1, e2;
        lgx_val_init(&e1);
        lgx_val_init(&e2);

        if (bc_expr(bc, node->child[0], &e1)) {
            return 1;
        }

        if (e1.u.reg.type == R_LOCAL) {
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

        reg_free(bc, &e1);
        reg_free(bc, &e2);
    } else if (node->child[0]->type == BINARY_EXPRESSION && node->child[0]->u.op == TK_INDEX) {
        lgx_val_t e1, e2, e3;
        lgx_val_init(&e1);
        lgx_val_init(&e2);
        lgx_val_init(&e3);

        if (bc_expr(bc, node->child[0]->child[0], &e1)) {
            return 1;
        }

        if ( !is_register(&e1) ) {
            bc_error(bc, "[Error] [Line:%d] variable expected\n", node->line);
            return 1;
        }

        if (node->child[0]->child[1]) {
            if (bc_expr(bc, node->child[0]->child[1], &e2)) {
                return 1;
            }

            if ( !is_register(&e2) && e2.type != T_LONG ) {
                bc_error(bc, "[Error] [Line:%d] makes integer from %s without a cast\n", node->line, lgx_val_typeof(&e2));
                return 1;
            }

            if (bc_expr(bc, node->child[1], &e3)) {
                return 1;
            }

            bc_array_set(bc, &e1, &e2, &e3);
        } else {
            if (bc_expr(bc, node->child[1], &e3)) {
                return 1;
            }

            bc_array_add(bc, &e1, &e3);
        }

        reg_free(bc, &e1);
        reg_free(bc, &e2);
        reg_free(bc, &e3);
    } else if (node->child[0]->type == BINARY_EXPRESSION && node->child[0]->u.op == TK_ATTR) {
        // TODO 对象属性赋值
    } else {
        bc_error(bc, "[Error] [Line:%d] invalid left variable for assignment\n", node->line);
        return 1;
    }

    return 0;
}

static int bc_expr_binary_call(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    lgx_val_t e1;
    lgx_val_init(&e1);

    if (bc_expr(bc, node->child[0], &e1)) {
        return 1;
    }

    if ( !is_register(&e1) && e1.type != T_FUNCTION ) {
        bc_error(bc, "[Error] [Line:%d] makes function from %s without a cast\n", node->line, lgx_val_typeof(&e1));
        return 1;
    }

    bc_call_new(bc, &e1);

    // 写入参数
    // TODO 检查参数数量是否匹配
    int i;
    for(i = 0; i < node->child[1]->children; i++) {
        lgx_val_t expr;
        lgx_val_init(&expr);
        bc_expr(bc, node->child[1]->child[i], &expr);
        bc_call_set(bc, &e1, i+4, &expr);
        reg_free(bc, &expr);
    }

    bc_call(bc, &e1);

    e->u.reg.type = R_TEMP;
    e->u.reg.reg = reg_pop(bc);
    bc_call_end(bc, &e1, e);

    reg_free(bc, &e1);

    return 0;
}

static int bc_expr_binary_index(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    lgx_val_t e1, e2;
    lgx_val_init(&e1);
    lgx_val_init(&e2);

    if (bc_expr(bc, node->child[0], &e1)) {
        return 1;
    }

    if ( !is_register(&e1) && e1.type != T_ARRAY ) {
        bc_error(bc, "[Error] [Line:%d] makes array from %s without a cast\n", node->line, lgx_val_typeof(&e1));
        return 1;
    }

    if (bc_expr(bc, node->child[1], &e2)) {
        return 1;
    }

    if ( !is_register(&e2) && e2.type != T_LONG ) {
        bc_error(bc, "[Error] [Line:%d] makes integer from %s without a cast\n", node->line, lgx_val_typeof(&e2));
        return 1;
    }

    if ( e1.type == T_ARRAY && e2.type == T_LONG ) {
        if (lgx_op_binary(node->u.op, e, &e1, &e2)) {
            return 1;
        }
    } else {
        if (bc_expr_binary(bc, node, e, &e1, &e2)) {
            return 1;
        }
    }

    reg_free(bc, &e1);
    reg_free(bc, &e2);

    return 0;
}

static int bc_expr(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    switch (node->type) {
        case STRING_TOKEN:
            return bc_string(bc, node, e);
        case LONG_TOKEN:
            return bc_long(bc, node, e);
        case DOUBLE_TOKEN:
            return bc_double(bc, node, e);
        case IDENTIFIER_TOKEN:
            return bc_identifier(bc, node, e);
        case TRUE_TOKEN:
            return bc_true(bc, node, e);
        case FALSE_TOKEN:
            return bc_false(bc, node, e);
        case UNDEFINED_TOKEN:
            return bc_undefined(bc, node, e);
        case ARRAY_TOKEN:
            return bc_expr_array(bc, node, e);
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
                    if (bc_expr_binary_index(bc, node, e)) {
                        return 1;
                    }
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
            lgx_val_init(&e1);
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

            reg_free(bc, &e1);
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
            for(i = 0; i < node->u.symbols->size; i++) {
                lgx_hash_node_t *head = &node->u.symbols->table[i];
                if (head->k.type != T_UNDEFINED) {
                    head->v.u.reg.type = R_LOCAL;
                    head->v.u.reg.reg = reg_pop(bc);
                }
                lgx_hash_node_t *next = head->next;
                while (next) {
                    next->v.u.reg.type = R_LOCAL;
                    next->v.u.reg.reg = reg_pop(bc);
                }
            }

            for(i = 0; i < node->children; i++) {
                if (bc_stat(bc, node->child[i])) {
                    return 1;
                }
            }

            // 释放局部变量的寄存器
            // TODO 寄存器释放顺序
            for(i = 0; i < node->u.symbols->size; i++) {
                lgx_hash_node_t *head = &node->u.symbols->table[i];
                if (head->k.type != T_UNDEFINED) {
                    reg_push(bc, head->v.u.reg.reg);
                }
                lgx_hash_node_t *next = head->next;
                while (next) {
                    reg_push(bc, next->v.u.reg.reg);
                }
            }
            break;
        }
        case IF_STATEMENT:{
            lgx_val_t e;
            lgx_val_init(&e);
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
                bc_error(bc, "[Error] [Line:%d] makes boolean from %s without a cast\n", node->line, lgx_val_typeof(&e));
            }
            break;
        }
        case IF_ELSE_STATEMENT:{
            lgx_val_t e;
            lgx_val_init(&e);
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
                bc_error(bc, "[Error] [Line:%d] makes boolean from %s without a cast\n", node->line, lgx_val_typeof(&e));
            }
            break;
        }
        case FOR_STATEMENT:{
            // exp1: 循环开始前执行一次
            if (node->child[0]) {
                lgx_val_t e;
                lgx_val_init(&e);
                if (bc_expr(bc, node->child[0], &e)) {
                    return 1;
                }
            }

            unsigned pos1 = bc->bc_top; // 跳转指令位置
            unsigned pos2 = 0; // 跳出指令位置

            // exp2: 每次循环开始前执行一次，如果值为假，则跳出循环
            if (node->child[1]) {
                lgx_val_t e;
                lgx_val_init(&e);
                if (bc_expr(bc, node->child[1], &e)) {
                    return 1;
                }

                if (e.type == T_BOOL) {
                    if (e.v.l == 0) {
                        break;
                    }
                } else if (is_register(&e)) {
                    pos2 = bc->bc_top; 
                    bc_test(bc, &e, 0);
                    reg_free(bc, &e);
                } else {
                    bc_error(bc, "[Error] [Line:%d] makes boolean from %s without a cast\n", node->line, lgx_val_typeof(&e));
                }
            }

            // 循环主体
            if (bc_stat(bc, node->child[3])) {
                return 1;
            }

            unsigned pos3 = bc->bc_top; // 跳转指令位置

            // exp3: 每次循环结束后执行一次
            if (node->child[2]) {
                lgx_val_t e;
                lgx_val_init(&e);
                if (bc_expr(bc, node->child[2], &e)) {
                    return 1;
                }
            }

            bc_jmp(bc, pos1);

            if (pos2) {
                bc_set_pd(bc, pos2, bc->bc_top);
            }

            jmp_fix(bc, node, pos3, bc->bc_top);

            break;
        }
        case WHILE_STATEMENT:{
            unsigned start = bc->bc_top; // 循环起始位置
            lgx_val_t e;
            lgx_val_init(&e);
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
                bc_error(bc, "[Error] [Line:%d] makes boolean from %s without a cast\n", node->line, lgx_val_typeof(&e));
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
            lgx_val_init(&e);
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
            // TODO
            break;
        case RETURN_STATEMENT:{
            lgx_val_t r;
            lgx_val_init(&r);
            r.u.reg.type = R_LOCAL;

            // 计算返回值
            if (node->child[0]) {
                if (bc_expr(bc, node->child[0], &r)) {
                    return 1;
                }
            }

            // 写入返回指令
            bc_ret(bc, &r);
            reg_free(bc, &r);
            break;
        }
        case ECHO_STATEMENT:{
            lgx_val_t e;
            lgx_val_init(&e);
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
                lgx_val_init(&e);
                if (bc_expr_binary_assignment(bc, node, &e)) {
                    return 1;
                }
            }
            break;
        }
        case EXPRESSION_STATEMENT: {
            lgx_val_t e;
            lgx_val_init(&e);
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
            lgx_str_t s;
            s.buffer = ((lgx_ast_node_token_t *)(node->child[0]))->tk_start;
            s.length = ((lgx_ast_node_token_t *)(node->child[0]))->tk_length;
            e = lgx_scope_global_val_get(node, &s);

            e->type = T_FUNCTION;
            e->v.fun = lgx_fun_new();
            e->v.fun->addr = bc->bc_top;
            // TODO 计算该函数所需的堆栈空间
            e->v.fun->stack_size = 256;

            // 重置寄存器分配
            unsigned char *regs = bc->regs;
            unsigned char reg_top = bc->reg_top;

            bc->regs = xcalloc(256, sizeof(unsigned char));
            bc->reg_top = 0;
            for(int i = 255; i >= 4; i--) {
                reg_push(bc, i);
            }

            // 编译函数体
            bc_stat(bc, node->child[2]);

            // 恢复寄存器分配
            bc->reg_top = reg_top;
            bc->regs = regs;

            // 始终写入一条返回语句，确保函数调用正常返回
            lgx_val_t r;
            lgx_val_init(&r);
            r.u.reg.type = R_LOCAL;
            bc_ret(bc, &r);

            bc_set_pe(bc, start, bc->bc_top);

            // 执行一次赋值操作
            const_add(bc, e);
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
    bc->ast = ast;

    bc->regs = xcalloc(256, sizeof(unsigned char));
    bc->reg_top = 0;
    for(int i = 255; i >= 4; i--) {
        reg_push(bc, i);
    }
    
    bc->bc_size = 1024;
    bc->bc_top = 0;
    bc->bc = xmalloc(bc->bc_size);

    bc->err_info = xmalloc(256);
    bc->err_len = 0;
    bc->errno = 0;

    bc->constant = lgx_hash_new(32);

    if (bc_stat(bc, ast->root)) {
        bc_error(bc, "[Error] unknown error\n");
        return 1;
    }

    bc_hlt(bc);    
    return 0;
}

int lgx_bc_cleanup(lgx_bc_t *bc) {
    xfree(bc->regs);
    xfree(bc->bc);
    xfree(bc->err_info);
    lgx_hash_delete(bc->constant);

    lgx_ast_cleanup(bc->ast);

    return 0;
}