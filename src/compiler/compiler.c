#include "../common/common.h"
#include "../parser/scope.h"
#include "../common/val.h"
#include "../common/cast.h"
#include "../common/operator.h"
#include "register.h"
#include "compiler.h"
#include "code.h"
#include "constant.h"

#define check_constant(v, t) ( !is_register((v)) && (v)->type == t )
#define check_variable(v, t) (  is_register((v)) && ( (v)->type == t || (v)->type == T_UNDEFINED ) )
#define check_type(v, t)     ( check_constant(v, t) || check_variable(v, t) )
#define is_auto(v)           (  is_register((v)) && (v)->type == T_UNDEFINED )

void bc_error(lgx_bc_t *bc, const char *fmt, ...) {
    va_list   args;

    if (bc->err_no) {
        return;
    }

    va_start(args, fmt);
    bc->err_len = vsnprintf(bc->err_info, 256, fmt, args);
    va_end(args);
    
    bc->err_no = 1;
}

static int bc_identifier(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *expr, int global) {
    lgx_val_t *v;
    lgx_str_t s;

    s.buffer = ((lgx_ast_node_token_t *)node)->tk_start;
    s.length = ((lgx_ast_node_token_t *)node)->tk_length;

    // 局部变量
    v = lgx_scope_local_val_get(node, &s);
    if (v) {
        v->u.c.used = 1;

        *expr = *v;

        return 0;
    }

    // 全局变量
    v = lgx_scope_global_val_get(node, &s);
    if (v) {
        v->u.c.used = 1;

        *expr = *v;

        if (v->type == T_FUNCTION) {
            expr->u.c.type = R_TEMP;
            expr->u.c.reg = reg_pop(bc);
            bc_load(bc, expr, v);
        } else if (global || v->u.c.type == R_LOCAL) {
            //*expr = *v;
        } else {
            expr->u.c.type = R_TEMP;
            expr->u.c.reg = reg_pop(bc);
            bc_global_get(bc, expr, v);
        }

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

    expr->u.c.type = 0;
    expr->u.c.reg = 0;

    return 0;
}

static int bc_double(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *expr) {
    expr->type = T_DOUBLE;
    expr->v.d = strtod(((lgx_ast_node_token_t *)node)->tk_start, NULL);

    expr->u.c.type = 0;
    expr->u.c.reg = 0;

    return 0;
}

static int bc_true(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *expr) {
    expr->type = T_BOOL;
    expr->v.l = 1;

    expr->u.c.type = 0;
    expr->u.c.reg = 0;

    return 0;
}

static int bc_false(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *expr) {
    expr->type = T_BOOL;
    expr->v.l = 0;

    expr->u.c.type = 0;
    expr->u.c.reg = 0;

    return 0;
}

static int bc_undefined(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *expr) {
    expr->type = T_UNDEFINED;

    expr->u.c.type = 0;
    expr->u.c.reg = 0;

    return 0;
}

static int bc_string(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *expr) {
    expr->type = T_STRING;
    expr->v.str = lgx_str_new_with_esc(((lgx_ast_node_token_t *)node)->tk_start+1, ((lgx_ast_node_token_t *)node)->tk_length-2);

    expr->u.c.type = 0;
    expr->u.c.reg = 0;

    return 0;
}

static int bc_expr_unary(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e, lgx_val_t *e1) {
    e->u.c.type = R_TEMP;
    e->u.c.reg = reg_pop(bc);

    switch (node->u.op) {
        case '!': bc_lnot(bc, e, e1); break;
        case '~': bc_not(bc, e, e1); break;
        case '-': bc_neg(bc, e, e1); break;
        case TK_TYPEOF: bc_typeof(bc, e, e1); break;
        default:
            bc_error(bc, "[Error] [Line:%d] unknown unary operation\n", node->line);
            return 1;
    }

    return 0;
}

static int bc_expr_binary(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e, lgx_val_t *e1, lgx_val_t *e2) {
    e->u.c.type = R_TEMP;
    e->u.c.reg = reg_pop(bc);

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
        case TK_INDEX: bc_array_get(bc, e, e1, e2); break;
        case TK_ATTR:
        case TK_ASSIGN_ADD:
        case TK_ASSIGN_SUB:
        case TK_ASSIGN_MUL:
        case TK_ASSIGN_DIV:
        case TK_ASSIGN_AND:
        case TK_ASSIGN_OR:
        case TK_ASSIGN_SHL:
        case TK_ASSIGN_SHR:
            break;
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
    while (node && node->type != FOR_STATEMENT && node->type != WHILE_STATEMENT && node->type != DO_WHILE_STATEMENT && node->type != SWITCH_STATEMENT) {
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
        loop->u.jmps = (lgx_ast_node_list_t *)xmalloc(sizeof(lgx_ast_node_list_t));
        if (loop->u.jmps) {
            lgx_list_init(&loop->u.jmps->head);
            loop->u.jmps->node = NULL;
        } else {
            bc_error(bc, "[Error] [Line:%d] out of memory\n", node->line);
            return 1;
        }
    }

    lgx_ast_node_list_t *n = (lgx_ast_node_list_t *)xmalloc(sizeof(lgx_ast_node_list_t));
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
        node->type != SWITCH_STATEMENT) {
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

// TODO 常量优化
static int bc_expr_array(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    e->type = T_ARRAY;

    e->u.c.type = R_TEMP;
    e->u.c.reg = reg_pop(bc);

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

static int bc_expr_binary_logic_and(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    // if (e1 == false) {
    //     e = false;
    // } else {
    //     e = e2;
    // }

    lgx_val_t e1, e2;
    lgx_val_init(&e1);
    lgx_val_init(&e2);

    if (bc_expr(bc, node->child[0], &e1)) {
        return 1;
    }

    if (check_constant(&e1, T_BOOL)) {
        if (e1.v.l == 0) {
            *e = e1;
            return 0;
        }

        if (bc_expr(bc, node->child[1], &e2)) {
            return 1;
        }

        if (check_constant(&e2, T_BOOL) || check_variable(&e2, T_BOOL)) {
            *e = e2;
            return 0;
        } else {
            bc_error(bc, "[Error] [Line:%d] makes boolean from %s without a cast\n", node->line, lgx_val_typeof(&e2));
            return 1;
        }
    } else if (check_variable(&e1, T_BOOL)) {
        e->u.c.type = R_TEMP;
        e->u.c.reg = reg_pop(bc);

        int pos1 = bc->bc_top;
        bc_test(bc, &e1, 0);
        reg_free(bc, &e1);

        // e1 == true
        if (bc_expr(bc, node->child[1], &e2)) {
            return 1;
        }

        if (check_constant(&e2, T_BOOL)) {
            bc_load(bc, e, &e2);
        } else if (check_variable(&e2, T_BOOL)) {
            bc_mov(bc, e, &e2);
        } else {
            bc_error(bc, "[Error] [Line:%d] makes boolean from %s without a cast\n", node->line, lgx_val_typeof(&e2));
            return 1;
        }

        int pos2 = bc->bc_top;
        bc_jmpi(bc, 0);

        bc_set_pd(bc, pos1, bc->bc_top - pos1 - 1);

        // e1 == false
        lgx_val_t tmp;
        tmp.type = T_BOOL;
        tmp.v.l = 0;
        bc_load(bc, e, &tmp);

        bc_set_pe(bc, pos2, bc->bc_top);

        return 0;
    } else {
        bc_error(bc, "[Error] [Line:%d] makes boolean from %s without a cast\n", node->line, lgx_val_typeof(&e1));
        return 1;
    }
}

static int bc_expr_binary_logic_or(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    // if (e1 == true) {
    //     e = true;
    // } else {
    //     e = e2;
    // }

    lgx_val_t e1, e2;
    lgx_val_init(&e1);
    lgx_val_init(&e2);

    if (bc_expr(bc, node->child[0], &e1)) {
        return 1;
    }

    if (check_constant(&e1, T_BOOL)) {
        if (e1.v.l) {
            *e = e1;
            return 0;
        }

        if (bc_expr(bc, node->child[1], &e2)) {
            return 1;
        }

        if (check_constant(&e2, T_BOOL) || check_variable(&e2, T_BOOL)) {
            *e = e2;
            return 0;
        } else {
            bc_error(bc, "[Error] [Line:%d] makes boolean from %s without a cast\n", node->line, lgx_val_typeof(&e2));
            return 1;
        }
    } else if (check_variable(&e1, T_BOOL)) {
        e->u.c.type = R_TEMP;
        e->u.c.reg = reg_pop(bc);

        int pos1 = bc->bc_top;
        bc_test(bc, &e1, 0);
        reg_free(bc, &e1);

        // e1 == true
        lgx_val_t tmp;
        tmp.type = T_BOOL;
        tmp.v.l = 1;
        bc_load(bc, e, &tmp);

        int pos2 = bc->bc_top;
        bc_jmpi(bc, 0);

        bc_set_pd(bc, pos1, bc->bc_top - pos1 - 1);

        // e1 == false
        if (bc_expr(bc, node->child[1], &e2)) {
            return 1;
        }

        if (check_constant(&e2, T_BOOL)) {
            bc_load(bc, e, &e2);
        } else if (check_variable(&e2, T_BOOL)) {
            bc_mov(bc, e, &e2);
        } else {
            bc_error(bc, "[Error] [Line:%d] makes boolean from %s without a cast\n", node->line, lgx_val_typeof(&e2));
            return 1;
        }

        bc_set_pe(bc, pos2, bc->bc_top);

        return 0;
    } else {
        bc_error(bc, "[Error] [Line:%d] makes boolean from %s without a cast\n", node->line, lgx_val_typeof(&e1));
        return 1;
    }
}

static int bc_expr_binary_math(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    lgx_val_t e1, e2;
    lgx_val_init(&e1);
    lgx_val_init(&e2);

    if (bc_expr(bc, node->child[0], &e1)) {
        return 1;
    }

    if (bc_expr(bc, node->child[1], &e2)) {
        return 1;
    }

    if (node->u.op == TK_EQ) {
        if (!is_auto(&e1) && !check_type(&e2, e1.type)) {
            bc_error(bc, "[Error] [Line:%d] makes %s from %s without a cast\n", node->line, lgx_val_typeof(&e1), lgx_val_typeof(&e2));
            return 1;
        }
    } else if (node->u.op == '+') {
        if (check_type(&e1, T_STRING) || check_type(&e2, T_STRING)) {
            // OK
        } else {
            if (!check_type(&e1, T_LONG) && !check_type(&e1, T_DOUBLE)) {
                bc_error(bc, "[Error] [Line:%d] makes number from %s without a cast\n", node->line, lgx_val_typeof(&e1));
                return 1;
            }
            if (!check_type(&e2, T_LONG) && !check_type(&e2, T_DOUBLE)) {
                bc_error(bc, "[Error] [Line:%d] makes number from %s without a cast\n", node->line, lgx_val_typeof(&e2));
                return 1;
            }
        }
    } else {
        if (!check_type(&e1, T_LONG) && !check_type(&e1, T_DOUBLE)) {
            bc_error(bc, "[Error] [Line:%d] makes number from %s without a cast\n", node->line, lgx_val_typeof(&e1));
            return 1;
        }
        if (!check_type(&e2, T_LONG) && !check_type(&e2, T_DOUBLE)) {
            bc_error(bc, "[Error] [Line:%d] makes number from %s without a cast\n", node->line, lgx_val_typeof(&e2));
            return 1;
        }
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

    if (!check_type(&e1, T_LONG)) {
        bc_error(bc, "[Error] [Line:%d] makes integer from %s without a cast\n", node->line, lgx_val_typeof(&e1));
        return 1;
    }

    if (bc_expr(bc, node->child[1], &e2)) {
        return 1;
    }

    if (!check_type(&e2, T_LONG)) {
        bc_error(bc, "[Error] [Line:%d] makes integer from %s without a cast\n", node->line, lgx_val_typeof(&e2));
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

static int bc_expr_binary_assignment(lgx_bc_t *bc, lgx_ast_node_t *node, lgx_val_t *e) {
    if (node->child[0]->type == IDENTIFIER_TOKEN) {
        lgx_val_t e1, e2;
        lgx_val_init(&e1);
        lgx_val_init(&e2);

        if (bc_identifier(bc, node->child[0], &e1, 1)) {
            return 1;
        }

        if (bc_expr(bc, node->child[1], &e2)) {
            return 1;
        }

        if (!is_auto(&e1) && !is_auto(&e2) && e1.type != e2.type) {
            // 允许从 int 到 float 的隐式转换
            if (e1.type == T_DOUBLE && check_constant(&e2, T_LONG)) {
                // 常量类型转换
                lgx_val_t tmp = e2;
                lgx_cast_double(&tmp, &e2);
                e2 = tmp;
                // TODO 变量类型转换
            } else {
                bc_error(bc, "[Error] [Line:%d] makes %s from %s without a cast\n", node->line, lgx_val_typeof(&e1), lgx_val_typeof(&e2));
                return 1;
            }
        }

        if (e1.u.c.type == R_LOCAL) {
            bc_mov(bc, &e1, &e2);
        } else {
            bc_global_set(bc, &e1, &e2);
        }

        // 写入表达式的值
        *e = e1;

        reg_free(bc, &e1);
    } else if (node->child[0]->type == BINARY_EXPRESSION && node->child[0]->u.op == TK_INDEX) {
        lgx_val_t e1, e2, e3;
        lgx_val_init(&e1);
        lgx_val_init(&e2);
        lgx_val_init(&e3);

        if (bc_expr(bc, node->child[0]->child[0], &e1)) {
            return 1;
        }

        if (!check_type(&e1, T_ARRAY)) {
            bc_error(bc, "[Error] [Line:%d] makes array from %s without a cast\n", node->line, lgx_val_typeof(&e1));
            return 1;
        }

        if (node->child[0]->child[1]) {
            if (bc_expr(bc, node->child[0]->child[1], &e2)) {
                return 1;
            }

            if (!check_type(&e2, T_LONG) && !check_type(&e2, T_STRING)) {
                bc_error(bc, "[Error] [Line:%d] attempt to index a %s key, integer or string expected\n", node->line, lgx_val_typeof(&e2));
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

        // 写入表达式的值
        *e = e3;

        reg_free(bc, &e1);
        reg_free(bc, &e2);
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

    if (!check_type(&e1, T_FUNCTION)) {
        bc_error(bc, "[Error] [Line:%d] makes function from %s without a cast\n", node->line, lgx_val_typeof(&e1));
        return 1;
    }

    lgx_fun_t *fun = e1.v.fun;

    // 实参数量必须小于等于形参
    if (node->child[1]->children > fun->args_num) {
        bc_error(bc, "[Error] [Line:%d] arguments length mismatch\n", node->line, lgx_val_typeof(&e1));
        return 1;
    }

    // 计算参数
    int i;
    lgx_val_t *expr = (lgx_val_t *)xcalloc(node->child[1]->children, sizeof(lgx_val_t));
    for(i = 0; i < node->child[1]->children; i++) {
        bc_expr(bc, node->child[1]->child[i], &expr[i]);
    }

    bc_call_new(bc, e1.u.c.reg);

    for(i = 0; i < fun->args_num; i++) {
        if (i < node->child[1]->children) {
            if (fun->args[i].type != T_UNDEFINED &&
                expr[i].type != T_UNDEFINED &&
                fun->args[i].type != expr[i].type) {
                bc_error(bc, "[Error] [Line:%d] makes %s from %s without a cast\n", node->child[1]->line, lgx_val_typeof(&fun->args[i]), lgx_val_typeof(&expr[i]));
                return 1;
            }

            bc_call_set(bc, i+4, &expr[i]);
            reg_free(bc, &expr[i]);
        } else {
            if (!fun->args[i].u.c.init) {
                bc_error(bc, "[Error] [Line:%d] arguments length mismatch\n", node->line);
                return 1;
            }
        }
    }

    xfree(expr);

    e->type = fun->ret.type;
    e->u.c.type = R_TEMP;
    e->u.c.reg = reg_pop(bc);
    bc_call(bc, e, e1.u.c.reg);

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

    if (!check_type(&e1, T_ARRAY)) {
        bc_error(bc, "[Error] [Line:%d] makes array from %s without a cast\n", node->line, lgx_val_typeof(&e1));
        return 1;
    }

    if (!node->child[1]) {
        bc_error(bc, "[Error] [Line:%d] index can not be empty\n", node->line);
        return 1;
    }

    if (bc_expr(bc, node->child[1], &e2)) {
        return 1;
    }

    if (!check_type(&e2, T_LONG) && !!check_type(&e1, T_STRING)) {
        bc_error(bc, "[Error] [Line:%d] attempt to index a %s key, integer or string expected\n", node->line, lgx_val_typeof(&e2));
        return 1;
    }

    if (!is_register(&e1) && !is_register(&e2)) {
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
            return bc_identifier(bc, node, e, 0);
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
                case TK_AND:
                    if (bc_expr_binary_logic_and(bc, node, e)) {
                        return 1;
                    }
                    break;
                case TK_OR:
                    if (bc_expr_binary_logic_or(bc, node, e)) {
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
                case TK_ASSIGN_ADD:
                case TK_ASSIGN_SUB:
                case TK_ASSIGN_MUL:
                case TK_ASSIGN_DIV:
                case TK_ASSIGN_AND:
                case TK_ASSIGN_OR:
                case TK_ASSIGN_SHL:
                case TK_ASSIGN_SHR:
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

            switch (node->u.op) {
                case '!':
                    if (!check_type(&e1, T_BOOL)) {
                        bc_error(bc, "[Error] [Line:%d] makes boolean from %s without a cast\n", node->line, lgx_val_typeof(&e1));
                        return 1;
                    }
                    break;
                case '~':
                    if (!check_type(&e1, T_LONG)) {
                        bc_error(bc, "[Error] [Line:%d] makes integer from %s without a cast\n", node->line, lgx_val_typeof(&e1));
                        return 1;
                    }
                    break;
                case '-':
                    if (!check_type(&e1, T_LONG) && !check_type(&e1, T_DOUBLE)) {
                        bc_error(bc, "[Error] [Line:%d] makes number from %s without a cast\n", node->line, lgx_val_typeof(&e1));
                        return 1;
                    }
                    break;
                case TK_TYPEOF:
                    break;
                default:
                    return 1;
            }

            if (!is_register(&e1)) {
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
            bc_error(bc, "[Error] [Line:%d] expression expected, ast-node type: %d\n", node->line, node->type);
            return 1;
    }

    return 0;
}

static int bc_stat(lgx_bc_t *bc, lgx_ast_node_t *node) {
    switch(node->type) {
        // Statement
        case BLOCK_STATEMENT:{
            int i;
            lgx_hash_node_t* next = node->u.symbols->head;
            // 为当前作用域的变量分配寄存器
            while (next) {
                if (next->v.type != T_FUNCTION) {
                    next->v.u.c.reg = reg_pop(bc);
                }
                next = next->order;
            }

            // 处理函数参数
            if (node->parent && node->parent->type == FUNCTION_DECLARATION) {
                lgx_val_t *e;
                lgx_str_t s;
                s.buffer = ((lgx_ast_node_token_t *)(node->parent->child[0]))->tk_start;
                s.length = ((lgx_ast_node_token_t *)(node->parent->child[0]))->tk_length;
                e = lgx_scope_global_val_get(node, &s);

                int i;
                for (i = 0; i < node->parent->child[1]->children; i++) {
                    lgx_ast_node_t *n = node->parent->child[1]->child[i];

                    // 参数是否有默认值
                    if (n->child[1]) {
                        lgx_val_t expr;
                        bc_expr(bc, n->child[1], &expr);
                        if (is_register(&expr)) {
                            bc_error(bc, "[Error] [Line:%d] only constant expression allowed in parameter declaration", n->line);
                            return 1;
                        }
                        if (e->v.fun->args[i].type != T_UNDEFINED &&
                            expr.type != T_UNDEFINED &&
                            e->v.fun->args[i].type != expr.type) {
                            bc_error(bc, "[Error] [Line:%d] makes %s from %s without a cast\n", n->line, lgx_val_typeof(&e->v.fun->args[i]), lgx_val_typeof(&expr));
                            return 1;
                        }
                        
                        // 如果参数值为 undefined，则赋值为初始值
                        lgx_val_t undef, ret, *v;
                        undef.type = T_UNDEFINED;
                        undef.u.c.type = R_TEMP;
                        undef.u.c.reg = reg_pop(bc);
                        ret.u.c.type = R_TEMP;
                        ret.u.c.reg = reg_pop(bc);

                        lgx_str_t s;
                        s.buffer = ((lgx_ast_node_token_t *)n->child[0])->tk_start;
                        s.length = ((lgx_ast_node_token_t *)n->child[0])->tk_length;

                        v = lgx_scope_local_val_get(node, &s);

                        bc_load(bc, &undef, &undef);
                        bc_eq(bc, &ret, v, &undef);
                        bc_test(bc, &ret, 1);
                        bc_mov(bc, v, &expr);

                        reg_free(bc, &ret);
                        reg_free(bc, &undef);
                    }
                }
            }

            for(i = 0; i < node->children; i++) {
                if (bc_stat(bc, node->child[i])) {
                    return 1;
                }
            }

            // 释放局部变量的寄存器
            // TODO 寄存器释放顺序？
            next = node->u.symbols->head;
            while (next) {
                // 检查未使用的变量
                if (!next->v.u.c.used) {
                    bc_error(bc, "[Error] [Line:%d] unused variable `%.*s`\n", node->line, next->k.v.str->length, next->k.v.str->buffer);
                    return 1;
                }
                if (next->v.type != T_FUNCTION) {
                    reg_push(bc, next->v.u.c.reg);
                }
                next = next->order;
            }
            break;
        }
        case IF_STATEMENT:{
            lgx_val_t e;
            lgx_val_init(&e);
            if (bc_expr(bc, node->child[0], &e)) {
                return 1;
            }

            if (check_constant(&e, T_BOOL)) {
                if (e.v.l == 0) {
                    break;
                } else {
                    if (bc_stat(bc, node->child[1])) {
                        return 1;
                    }
                }
            } else if (check_variable(&e, T_BOOL)) {
                unsigned pos = bc->bc_top; // 跳出指令位置
                bc_test(bc, &e, 0);
                reg_free(bc, &e);

                if (bc_stat(bc, node->child[1])) {
                    return 1;
                }

                bc_set_pd(bc, pos, bc->bc_top - pos - 1);
            } else {
                bc_error(bc, "[Error] [Line:%d] makes boolean from %s without a cast\n", node->line, lgx_val_typeof(&e));
                return 1;
            }
            break;
        }
        case IF_ELSE_STATEMENT:{
            lgx_val_t e;
            lgx_val_init(&e);
            if (bc_expr(bc, node->child[0], &e)) {
                return 1;
            }

            if (check_constant(&e, T_BOOL)) {
                if (e.v.l == 0) {
                    if( bc_stat(bc, node->child[2])) {
                        return 1;
                    }
                } else {
                    if (bc_stat(bc, node->child[1])) {
                        return 1;
                    }
                }
            } else if (check_variable(&e, T_BOOL)) {
                unsigned pos1 = bc->bc_top; // 跳转指令位置
                bc_test(bc, &e, 0);
                reg_free(bc, &e);

                if (bc_stat(bc, node->child[1])) {
                    return 1;
                }

                unsigned pos2 = bc->bc_top; // 跳转指令位置
                bc_jmpi(bc, 0);
                bc_set_pd(bc, pos1, bc->bc_top - pos1 - 1);

                if (bc_stat(bc, node->child[2])) {
                    return 1;
                }

                bc_set_pe(bc, pos2, bc->bc_top);
            } else {
                bc_error(bc, "[Error] [Line:%d] makes boolean from %s without a cast\n", node->line, lgx_val_typeof(&e));
                return 1;
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

                if (check_constant(&e, T_BOOL)) {
                    if (e.v.l == 0) {
                        break;
                    }
                } else if (check_variable(&e, T_BOOL)) {
                    pos2 = bc->bc_top; 
                    bc_test(bc, &e, 0);
                    reg_free(bc, &e);
                } else {
                    bc_error(bc, "[Error] [Line:%d] makes boolean from %s without a cast\n", node->line, lgx_val_typeof(&e));
                    return 1;
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

            bc_jmpi(bc, pos1);

            if (pos2) {
                bc_set_pd(bc, pos2, bc->bc_top - pos2 - 1);
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

            if (check_constant(&e, T_BOOL)) {
                if (e.v.l == 0) {
                    break;
                } else {
                    if (bc_stat(bc, node->child[1])) {
                        return 1;
                    }
                    // 写入无条件跳转
                    bc_jmpi(bc, start);
                }
            } else if (check_variable(&e, T_BOOL)) {
                unsigned pos = bc->bc_top; // 循环跳出指令位置
                bc_test(bc, &e, 0);
                reg_free(bc, &e);

                if (bc_stat(bc, node->child[1])) {
                    return 1;
                }
                // 写入无条件跳转
                bc_jmpi(bc, start);
                // 更新条件跳转
                bc_set_pd(bc, pos, bc->bc_top - pos - 1);
            } else {
                bc_error(bc, "[Error] [Line:%d] makes boolean from %s without a cast\n", node->line, lgx_val_typeof(&e));
                return 1;
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

            if (check_constant(&e, T_BOOL)) {
                if (e.v.l == 0) {
                    break;
                } else {
                    // 写入无条件跳转
                    bc_jmpi(bc, start);
                }
            } else if (check_variable(&e, T_BOOL)) {
                unsigned pos = bc->bc_top;
                bc_test(bc, &e, 0);
                bc_jmpi(bc, start);
                reg_free(bc, &e);

                bc_set_pd(bc, pos, bc->bc_top - pos - 1);
            } else {
                bc_error(bc, "[Error] [Line:%d] makes boolean from %s without a cast\n", node->line, lgx_val_typeof(&e));
                return 1;
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
            bc_jmpi(bc, 0);
            break;
        case BREAK_STATEMENT:
            // 只能出现在循环语句和 switch 语句中
            if (jmp_add(bc, node)) {
                // error
                break;
            }
            node->u.pos = bc->bc_top;
            bc_jmpi(bc, 0);
            break;
        case SWITCH_STATEMENT:{
            // 执行表达式
            lgx_val_t e;
            lgx_val_init(&e);
            if (bc_expr(bc, node->child[0], &e)) {
                return 1;
            }

            if (!check_type(&e, T_LONG) && check_type(&e, T_STRING)) {
                bc_error(bc, "[Error] [Line:%d] makes integer or string from %s without a cast\n", node->line, lgx_val_typeof(&e));
                return 1;
            }

            if (node->children == 1) {
                // 没有 case 和 default，什么都不用做
                reg_free(bc, &e);
            } else {
                lgx_val_t condition;
                condition.type = T_ARRAY;
                condition.v.arr = lgx_hash_new(node->children - 1);

                int i, has_default = 0;
                for (i = 1; i < node->children; i ++) {
                    lgx_ast_node_t *child = node->child[i];
                    if (child->type == CASE_STATEMENT) {
                        lgx_hash_node_t case_node;
                        lgx_val_init(&case_node.k);

                        if (bc_expr(bc, child->child[0], &case_node.k)) {
                            return 1;
                        }

                        if (!check_constant(&case_node.k, T_LONG) && !check_constant(&case_node.k, T_STRING)) {
                            bc_error(bc, "[Error] [Line:%d] only constant expression allowed in case label", child->line);
                            return 1;
                        }

                        if (lgx_hash_get(condition.v.arr, &case_node.k)) {
                            bc_error(bc, "[Error] [Line:%d] duplicate case value", child->line);
                            return 1;
                        }

                        case_node.v.type = T_LONG;
                        case_node.v.v.l = 0;

                        lgx_hash_set(condition.v.arr, &case_node);
                    } else {
                        has_default = 1;
                    }
                }

                lgx_val_t undef;
                undef.type = T_UNDEFINED;
                undef.u.c.type = R_TEMP;
                undef.u.c.reg = reg_pop(bc);

                lgx_val_t tmp, result;
                tmp.u.c.type = R_TEMP;
                tmp.u.c.reg = reg_pop(bc);
                result.u.c.type = R_TEMP;
                result.u.c.reg = reg_pop(bc);

                bc_load(bc, &tmp, &condition);
                bc_array_get(bc, &tmp, &tmp, &e);
                bc_load(bc, &undef, &undef);
                bc_eq(bc, &result, &tmp, &undef);
                bc_test(bc, &result, 1);

                unsigned pos = bc->bc_top;
                bc_jmpi(bc, 0);
                bc_jmp(bc, &tmp);

                reg_free(bc, &result);
                reg_free(bc, &tmp);
                reg_free(bc, &undef);
                reg_free(bc, &e);

                for (i = 1; i < node->children; i ++) {
                    lgx_ast_node_t *child = node->child[i];

                    if (child->type == CASE_STATEMENT) {
                        lgx_val_t k;
                        if (bc_expr(bc, child->child[0], &k)) {
                            return 1;
                        }

                        lgx_hash_node_t *n = lgx_hash_get(condition.v.arr, &k);
                        n->v.v.l = bc->bc_top;

                        if (bc_stat(bc, child->child[1])) {
                            return 1;
                        }
                    } else { // child->type == DEFAULT_STATEMENT
                        bc_set_pe(bc, pos, bc->bc_top);

                        if (bc_stat(bc, child->child[0])) {
                            return 1;
                        }
                    }
                }

                if (!has_default) {
                    bc_set_pe(bc, pos, bc->bc_top);
                }
            }

            jmp_fix(bc, node, 0, bc->bc_top);
            break;
        }
        case TRY_STATEMENT:{
            break;
        }
        case THROW_STATEMENT:{
            break;
        }
        case RETURN_STATEMENT:{
            lgx_val_t r;
            lgx_val_init(&r);

            // 获取返回值类型
            lgx_ast_node_t *n = node->parent;
            while (n && n->type != FUNCTION_DECLARATION) {
                if (n->parent) {
                    n = n->parent;
                } else {
                    n = NULL;
                }
            }

            lgx_val_t ret;
            if (n) {
                lgx_str_t s;
                s.buffer = ((lgx_ast_node_token_t *)(n->child[0]))->tk_start;
                s.length = ((lgx_ast_node_token_t *)(n->child[0]))->tk_length;
                lgx_val_t *f = lgx_scope_global_val_get(n, &s);
                ret.type = f->v.fun->ret.type;
            } else {
                ret.type = T_UNDEFINED;
            }

            // 计算返回值
            if (node->child[0]) {
                if (bc_expr(bc, node->child[0], &r)) {
                    return 1;
                }

                if (ret.type != T_UNDEFINED && !is_auto(&r) && ret.type != r.type) {
                    bc_error(bc, "[Error] [Line:%d] makes %s from %s without a cast\n", node->line, lgx_val_typeof(&ret), lgx_val_typeof(&r));
                    return 1;
                }
            } else {
                if (ret.type == T_UNDEFINED) {
                    r.u.c.type = R_LOCAL;
                } else {
                    bc_error(bc, "[Error] [Line:%d] makes %s from undefined without a cast\n", node->line, lgx_val_typeof(&ret));
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
                reg_free(bc, &e);
            }
            break;
        }
        case EXPRESSION_STATEMENT: {
            lgx_val_t e;
            lgx_val_init(&e);
            if (bc_expr(bc, node->child[0], &e)) {
                return 1;
            }
            reg_free(bc, &e);
            break;
        }
        // Declaration
        case FUNCTION_DECLARATION:{
            unsigned start = bc->bc_top;
            bc_jmpi(bc, 0);

            lgx_val_t *e;
            lgx_str_t s;
            s.buffer = ((lgx_ast_node_token_t *)(node->child[0]))->tk_start;
            s.length = ((lgx_ast_node_token_t *)(node->child[0]))->tk_length;
            e = lgx_scope_global_val_get(node, &s);
            e->v.fun->addr = bc->bc_top;

            // 重置寄存器分配
            lgx_reg_alloc_t *reg = bc->reg;
            bc->reg = reg_allocator_new();

            // 编译函数体
            bc_stat(bc, node->child[2]);

            // 保存该函数所需的堆栈空间
            e->v.fun->stack_size = bc->reg->max + 1;

            // 恢复寄存器分配
            reg_allocator_delete(bc->reg);
            bc->reg = reg;

            // 始终写入一条返回语句，确保函数调用正常返回
            lgx_val_t r;
            lgx_val_init(&r);
            r.u.c.type = R_LOCAL;
            bc_ret(bc, &r);

            bc_set_pe(bc, start, bc->bc_top);

            // 执行一次赋值操作
            //bc_load(bc, e, e);
            
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

    bc->reg = reg_allocator_new();
    
    bc->bc_size = 1024;
    bc->bc_top = 0;
    bc->bc = (unsigned *)xmalloc(bc->bc_size * sizeof(unsigned));

    bc->err_info = (char *)xmalloc(256);
    bc->err_len = 0;
    bc->err_no = 0;

    bc->constant = lgx_hash_new(32);

    if (bc_stat(bc, ast->root)) {
        bc_error(bc, "[Error] unknown error\n");
        return 1;
    }

    lgx_val_t ret;
    lgx_val_init(&ret);
    ret.type = T_UNDEFINED;
    ret.u.c.type = R_LOCAL;
    bc_ret(bc, &ret);
    return 0;
}

int lgx_bc_cleanup(lgx_bc_t *bc) {
    reg_allocator_delete(bc->reg);
    xfree(bc->bc);
    xfree(bc->err_info);
    // TODO 需要处理内存泄漏和重复释放问题
    //lgx_hash_delete(bc->constant);

    lgx_ast_cleanup(bc->ast);

    return 0;
}