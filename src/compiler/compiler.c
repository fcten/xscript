#include "../common/common.h"
#include "../common/escape.h"
#include "../parser/symbol.h"
#include "register.h"
#include "compiler.h"
#include "bytecode.h"
#include "constant.h"
#include "exception.h"
#include "expr_result.h"

// 追加一条错误信息
void compiler_error(lgx_compiler_t* c, lgx_ast_node_t* node, const char *fmt, ...) {
    va_list   args;

    assert(c->ast);
    lgx_ast_t* ast = c->ast;

    lgx_ast_error_list_t* err = xcalloc(1, sizeof(lgx_ast_error_list_t));
    if (!err) {
        return;
    }

    lgx_list_init(&err->head);
    err->err_no = 1;

    if (lgx_str_init(&err->err_msg, 256)) {
        xfree(err);
        return;
    }

    if (ast->lex.source.path) {
        err->err_msg.length = snprintf(err->err_msg.buffer, err->err_msg.size,
            "[COMPILER ERROR] [%s:%d:%d] ", ast->lex.source.path, node->line + 1, node->row);
    } else {
        err->err_msg.length = snprintf(err->err_msg.buffer, err->err_msg.size,
            "[COMPILER ERROR] ");
    }

    va_start(args, fmt);
    err->err_msg.length += vsnprintf(err->err_msg.buffer + err->err_msg.length,
        256 - err->err_msg.length, fmt, args);
    va_end(args);
    
    lgx_list_add_tail(&err->head, &ast->errors);
}

static lgx_ast_node_t *find_function(lgx_ast_node_t *node) {
    while (node && node->type != FUNCTION_DECLARATION) {
        node = node->parent;
    }
    return node;
}

static int reg_pop(lgx_compiler_t* c, lgx_ast_node_t *node) {
    node = find_function(node);
    assert(node);

    int r = lgx_reg_pop(node->u.regs);
    if (r < 0) {
        compiler_error(c, node, "too many local variables\n");
        return -1;
    }

    return r;
}

static void reg_push(lgx_compiler_t* c, lgx_ast_node_t *node, unsigned char reg) {
    node = find_function(node);
    assert(node);

    lgx_reg_push(node->u.regs, reg);
}

static int load_to_reg(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    int r = reg_pop(c, node);

    if (r >= 0) {
        if (is_literal(e)) {
            int reg = lgx_const_get(&c->constant, e);
            assert(reg >= 0);
            bc_load(c, r, reg);
        } else if (is_const(e)) {
            bc_load(c, r, e->u.constant);
        } else if (is_global(e)) {
            bc_global_get(c, r, e->u.global);
        }
    } else {
        compiler_error(c, node, "too many local variables\n");
    }

    return r;
}

int lgx_expr_result_init(lgx_expr_result_t* e) {
    memset(e, 0, sizeof(lgx_expr_result_t));
    return 0;
}

int lgx_expr_result_cleanup(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    // 释放寄存器
    if (is_temp(e)) {
        reg_push(c, node, e->u.local);
    }

    if (is_constant(e)) {
        switch (e->v_type.type) {
            case T_STRING:
                lgx_str_cleanup(&e->v.str);
                break;
            case T_ARRAY:
                lgx_array_value_cleanup(&e->v.arr);
                lgx_ht_cleanup(&e->v.arr);
                break;
            default:
                break;
        }
    }

    lgx_type_cleanup(&e->v_type);

    return 0;
}

static lgx_ast_node_t *jmp_find_loop(lgx_ast_node_t *node) {
    while (node && node->type != FOR_STATEMENT && node->type != WHILE_STATEMENT && node->type != DO_STATEMENT) {
        node = node->parent;
    }
    return node;
}

static lgx_ast_node_t *jmp_find_loop_or_switch(lgx_ast_node_t *node) {
    while (node && node->type != FOR_STATEMENT && node->type != WHILE_STATEMENT && node->type != DO_STATEMENT && node->type != SWITCH_STATEMENT) {
        node = node->parent;
    }
    return node;
}

static int jmp_add(lgx_compiler_t* c, lgx_ast_node_t *node) {
    lgx_ast_node_t *loop = NULL;
    if (node->type == CONTINUE_STATEMENT) {
        loop = jmp_find_loop(node);
    } else if (node->type == BREAK_STATEMENT) {
        loop = jmp_find_loop_or_switch(node);
    } else {
        compiler_error(c, node, "break or continue statement expected\n");
        return 1;
    }
    if (!loop) {
        compiler_error(c, node, "illegal break or continue statement\n");
        return 1;
    }

    // 初始化
    if (!loop->u.jmps) {
        loop->u.jmps = (lgx_list_t *)xmalloc(sizeof(lgx_list_t));
        if (loop->u.jmps) {
            lgx_list_init(loop->u.jmps);
        } else {
            compiler_error(c, node, "out of memory\n");
            return 1;
        }
    }

    lgx_ast_node_list_t *n = (lgx_ast_node_list_t *)xmalloc(sizeof(lgx_ast_node_list_t));
    if (n) {
        n->node = node;
        lgx_list_init(&n->head);
        lgx_list_add_tail(&n->head, loop->u.jmps);

        return 0;
    } else {
        compiler_error(c, node, "out of memory\n");
        return 1;
    }
}

static int jmp_fix(lgx_compiler_t* c, lgx_ast_node_t *node, unsigned start, unsigned end) {
    if (node->type != FOR_STATEMENT &&
        node->type != WHILE_STATEMENT && 
        node->type != DO_STATEMENT &&
        node->type != SWITCH_STATEMENT) {
        compiler_error(c, node, "switch or loop statement expected\n");
        return 1;
    }

    if (!node->u.jmps) {
        return 0;
    }

    lgx_ast_node_list_t *n;
    lgx_list_for_each_entry(n, lgx_ast_node_list_t, node->u.jmps, head) {
        if (n->node->type == BREAK_STATEMENT) {
            bc_set_param_e(c, n->node->u.pos, end);
        } else if (n->node->type == CONTINUE_STATEMENT) {
            bc_set_param_e(c, n->node->u.pos, start);
        } else {
            // error
            compiler_error(c, node, "break or continue statement expected\n");
            return 1;
        }
    }

    return 0;
}

// 禁止在逻辑操作中使用赋值表达式，无论它的返回值是否为布尔值
// TODO 递归判断子节点
static int compiler_check_assignment(lgx_compiler_t* c, lgx_ast_node_t *node) {
    if (node->type == BINARY_EXPRESSION && node->u.op == TK_ASSIGN) {
        compiler_error(c, node, "assignment expression can't be used as boolean value\n");
        return 1;
    }

    return 0;
}

static int compiler_string_token(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    assert(node->type == STRING_TOKEN);

    e->type = EXPR_LITERAL;
    e->v_type.type = T_STRING;

    lgx_str_t src, dst;
    src.buffer = c->ast->lex.source.content + node->offset + 1;
    src.length = src.size = node->length - 2;
    if (lgx_str_init(&dst, src.length)) {
        compiler_error(c, node, "out of memory\n");
        return 1;
    }
    lgx_escape_decode(&src, &dst);

    e->v.str = dst;

    return 0;
}

static int compiler_char_token(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    assert(node->type == CHAR_TOKEN);

    e->type = EXPR_LITERAL;
    e->v_type.type = T_LONG;
    //e->v.l = 

    lgx_str_t src, dst;
    src.buffer = c->ast->lex.source.content + node->offset + 1;
    src.length = src.size = node->length - 2;
    if (lgx_str_init(&dst, src.length)) {
        return 1;
    }
    lgx_escape_decode(&src, &dst);

    e->v.l = (unsigned char)dst.buffer[0];

    lgx_str_cleanup(&dst);

    return 0;
}

static int compiler_long_token(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    assert(node->type == LONG_TOKEN);

    e->type = EXPR_LITERAL;
    e->v_type.type = T_LONG;

    char *s = c->ast->lex.source.content + node->offset;
    if (s[0] == '0') {
        if (s[1] == 'b' || s[1] == 'B') {
            e->v.l = strtoll(s+2, NULL, 2);
        } else if (s[1] == 'x' || s[1] == 'X') {
            e->v.l = strtoll(s+2, NULL, 16);
        } else {
            e->v.l = strtoll(s, NULL, 10);
        }
    } else {
        e->v.l = strtoll(s, NULL, 10);
    }

    return 0;
}

static int compiler_double_token(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    assert(node->type == DOUBLE_TOKEN);

    e->type = EXPR_LITERAL;
    e->v_type.type = T_DOUBLE;

    char *s = c->ast->lex.source.content + node->offset;
    e->v.d = strtod(s, NULL);

    return 0;
}

static int compiler_identifier_token(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    assert(node->type == IDENTIFIER_TOKEN);

    lgx_str_t name;
    name.buffer = c->ast->lex.source.content + node->offset;
    name.length = node->length;
    name.size = 0;
    lgx_symbol_t* symbol = lgx_symbol_get(node, &name, node->offset);

    if (!symbol) {
        compiler_error(c, node, "use undeclared identifier `%.*s`\n", name.length, name.buffer);
        return 1;
    }

    switch (symbol->s_type) {
        case S_VARIABLE:
            // 判断是全局变量还是局部变量
            if (symbol->is_global) {
                e->type = EXPR_GLOBAL;
                e->u.global = symbol->r;
                e->symbol = symbol;
                if (lgx_type_dup(&symbol->type, &e->v_type)) {
                    return 1;
                }
            } else {
                e->type = EXPR_LOCAL;
                e->u.local = symbol->r;
                e->symbol = symbol;
                if (lgx_type_dup(&symbol->type, &e->v_type)) {
                    return 1;
                }
            }
            break;
        case S_CONSTANT:
            e->type = EXPR_CONSTANT;
            e->u.constant = symbol->r;
            e->symbol = symbol;
            if (lgx_type_dup(&symbol->type, &e->v_type)) {
                return 1;
            }
            break;
        default:
            compiler_error(c, node, "use invalid identifier `%.*s`\n", name.length, name.buffer);
            return 1;
    }

    return 0;
}

static int compiler_true_token(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    assert(node->type == TRUE_TOKEN);

    e->type = EXPR_LITERAL;
    e->v_type.type = T_BOOL;
    e->v.l = 1;

    return 0;
}

static int compiler_false_token(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    assert(node->type == FALSE_TOKEN);

    e->type = EXPR_LITERAL;
    e->v_type.type = T_BOOL;
    e->v.l = 0;

    return 0;
}

static void cast_double(lgx_expr_result_t* e) {
    if (check_type(e, T_LONG)) {
        e->v_type.type = T_DOUBLE;
        e->v.d = e->v.l;
    }
}

static int op_add(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, lgx_expr_result_t* l, lgx_expr_result_t* r) {
    assert(check_constant(l, T_LONG) || check_constant(l, T_DOUBLE));
    assert(check_constant(r, T_LONG) || check_constant(r, T_DOUBLE));

    if (check_constant(l, T_LONG) && check_constant(r, T_LONG)) {
        e->type = EXPR_LITERAL;
        e->v_type.type = T_LONG;
        e->v.l = l->v.l + r->v.l;
    } else {
        cast_double(l);
        cast_double(r);
        e->type = EXPR_LITERAL;
        e->v_type.type = T_DOUBLE;
        e->v.d = l->v.d + r->v.d;
    }

    return 0;
}

static int op_sub(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, lgx_expr_result_t* l, lgx_expr_result_t* r) {
    assert(check_constant(l, T_LONG) || check_constant(l, T_DOUBLE));
    assert(check_constant(r, T_LONG) || check_constant(r, T_DOUBLE));

    if (check_constant(l, T_LONG) && check_constant(r, T_LONG)) {
        e->type = EXPR_LITERAL;
        e->v_type.type = T_LONG;
        e->v.l = l->v.l - r->v.l;
    } else {
        cast_double(l);
        cast_double(r);
        e->type = EXPR_LITERAL;
        e->v_type.type = T_DOUBLE;
        e->v.d = l->v.d - r->v.d;
    }

    return 0;
}

static int op_mul(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, lgx_expr_result_t* l, lgx_expr_result_t* r) {
    assert(check_constant(l, T_LONG) || check_constant(l, T_DOUBLE));
    assert(check_constant(r, T_LONG) || check_constant(r, T_DOUBLE));

    if (check_constant(l, T_LONG) && check_constant(r, T_LONG)) {
        e->type = EXPR_LITERAL;
        e->v_type.type = T_LONG;
        e->v.l = l->v.l * r->v.l;
    } else {
        cast_double(l);
        cast_double(r);
        e->type = EXPR_LITERAL;
        e->v_type.type = T_DOUBLE;
        e->v.d = l->v.d * r->v.d;
    }

    return 0;
}

static int op_div(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, lgx_expr_result_t* l, lgx_expr_result_t* r) {
    assert(check_constant(l, T_LONG) || check_constant(l, T_DOUBLE));
    assert(check_constant(r, T_LONG) || check_constant(r, T_DOUBLE));

    if (check_constant(l, T_LONG) && check_constant(r, T_LONG)) {
        e->type = EXPR_LITERAL;
        e->v_type.type = T_LONG;
        if (r->v.l == 0) {
            compiler_error(c, node, "divide by zero\n");
            return 1;
        }
        e->v.l = l->v.l / r->v.l;
    } else {
        cast_double(l);
        cast_double(r);
        e->type = EXPR_LITERAL;
        e->v_type.type = T_DOUBLE;
        if (r->v.d == 0) {
            compiler_error(c, node, "divide by zero\n");
            return 1;
        }
        e->v.d = l->v.d / r->v.d;
    }

    return 0;
}

static int op_mod(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, lgx_expr_result_t* l, lgx_expr_result_t* r) {
    assert(check_constant(l, T_LONG) || check_constant(l, T_DOUBLE));
    assert(check_constant(r, T_LONG) || check_constant(r, T_DOUBLE));

    if (check_constant(l, T_LONG) && check_constant(r, T_LONG)) {
        e->type = EXPR_LITERAL;
        e->v_type.type = T_LONG;
        e->v.l = l->v.l % r->v.l;
    } else {
        compiler_error(c, node, "invalid operand type for operator %%\n");
        return 1;
    }

    return 0;
}

static int op_eq(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, lgx_expr_result_t* l, lgx_expr_result_t* r) {
    assert(check_constant(l, T_LONG) || check_constant(l, T_DOUBLE));
    assert(check_constant(r, T_LONG) || check_constant(r, T_DOUBLE));

    if (check_constant(l, T_LONG) && check_constant(r, T_LONG)) {
        e->type = EXPR_LITERAL;
        e->v_type.type = T_BOOL;
        e->v.l = l->v.l == r->v.l;
    } else {
        cast_double(l);
        cast_double(r);
        e->type = EXPR_LITERAL;
        e->v_type.type = T_BOOL;
        e->v.l = l->v.d == r->v.d;
    }

    return 0;
}

static int op_ne(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, lgx_expr_result_t* l, lgx_expr_result_t* r) {
    assert(check_constant(l, T_LONG) || check_constant(l, T_DOUBLE));
    assert(check_constant(r, T_LONG) || check_constant(r, T_DOUBLE));

    if (check_constant(l, T_LONG) && check_constant(r, T_LONG)) {
        e->type = EXPR_LITERAL;
        e->v_type.type = T_BOOL;
        e->v.l = l->v.l != r->v.l;
    } else {
        cast_double(l);
        cast_double(r);
        e->type = EXPR_LITERAL;
        e->v_type.type = T_BOOL;
        e->v.l = l->v.d != r->v.d;
    }

    return 0;
}

static int op_gt(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, lgx_expr_result_t* l, lgx_expr_result_t* r) {
    assert(check_constant(l, T_LONG) || check_constant(l, T_DOUBLE));
    assert(check_constant(r, T_LONG) || check_constant(r, T_DOUBLE));

    if (check_constant(l, T_LONG) && check_constant(r, T_LONG)) {
        e->type = EXPR_LITERAL;
        e->v_type.type = T_BOOL;
        e->v.l = l->v.l > r->v.l;
    } else {
        cast_double(l);
        cast_double(r);
        e->type = EXPR_LITERAL;
        e->v_type.type = T_BOOL;
        e->v.l = l->v.d > r->v.d;
    }

    return 0;
}

static int op_ge(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, lgx_expr_result_t* l, lgx_expr_result_t* r) {
    assert(check_constant(l, T_LONG) || check_constant(l, T_DOUBLE));
    assert(check_constant(r, T_LONG) || check_constant(r, T_DOUBLE));

    if (check_constant(l, T_LONG) && check_constant(r, T_LONG)) {
        e->type = EXPR_LITERAL;
        e->v_type.type = T_BOOL;
        e->v.l = l->v.l >= r->v.l;
    } else {
        cast_double(l);
        cast_double(r);
        e->type = EXPR_LITERAL;
        e->v_type.type = T_BOOL;
        e->v.l = l->v.d >= r->v.d;
    }

    return 0;
}

static int op_lt(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, lgx_expr_result_t* l, lgx_expr_result_t* r) {
    assert(check_constant(l, T_LONG) || check_constant(l, T_DOUBLE));
    assert(check_constant(r, T_LONG) || check_constant(r, T_DOUBLE));

    if (check_constant(l, T_LONG) && check_constant(r, T_LONG)) {
        e->type = EXPR_LITERAL;
        e->v_type.type = T_BOOL;
        e->v.l = l->v.l < r->v.l;
    } else {
        cast_double(l);
        cast_double(r);
        e->type = EXPR_LITERAL;
        e->v_type.type = T_BOOL;
        e->v.l = l->v.d < r->v.d;
    }

    return 0;
}

static int op_le(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, lgx_expr_result_t* l, lgx_expr_result_t* r) {
    assert(check_constant(l, T_LONG) || check_constant(l, T_DOUBLE));
    assert(check_constant(r, T_LONG) || check_constant(r, T_DOUBLE));

    if (check_constant(l, T_LONG) && check_constant(r, T_LONG)) {
        e->type = EXPR_LITERAL;
        e->v_type.type = T_BOOL;
        e->v.l = l->v.l <= r->v.l;
    } else {
        cast_double(l);
        cast_double(r);
        e->type = EXPR_LITERAL;
        e->v_type.type = T_BOOL;
        e->v.l = l->v.d <= r->v.d;
    }

    return 0;
}

static int op_index(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, lgx_expr_result_t* l, lgx_expr_result_t* r) {
    assert(check_constant(l, T_ARRAY));
    // TODO 允许更多类型作为键
    assert(check_constant(r, T_LONG) || check_constant(r, T_STRING));

    lgx_ht_node_t *n = NULL;
    if (check_type(r, T_STRING)) {
        n = lgx_ht_get(&l->v.arr, &r->v.str);
    } else {
        lgx_str_t key;
        key.buffer = (char *)&r->v.l;
        key.length = sizeof(r->v.l);
        key.size = 0;
        n = lgx_ht_get(&l->v.arr, &key);
    }

    if (!n) {
        e->type = EXPR_LITERAL;
        e->v_type.type = T_NULL;
        return 0;
    }

    // 读取具体的值
    lgx_value_t* v = (lgx_value_t*)n->v;

    return lgx_value_to_expr(v, e);
}

static int op_concat(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, lgx_expr_result_t* l, lgx_expr_result_t* r) {
    assert(check_constant(l, T_STRING));
    assert(check_constant(r, T_STRING));

    e->type = EXPR_LITERAL;
    e->v_type.type = T_STRING;
    if (lgx_str_init(&e->v.str, l->v.str.length + r->v.str.length)) {
        compiler_error(c, node, "out of memory\n");
        return 1;
    }
    lgx_str_concat(&l->v.str, &e->v.str);
    lgx_str_concat(&r->v.str, &e->v.str);

    return 0;
}

static int op_shl(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, lgx_expr_result_t* l, lgx_expr_result_t* r) {
    assert(check_constant(l, T_LONG));
    assert(check_constant(r, T_LONG));

    e->type = EXPR_LITERAL;
    e->v_type.type = T_LONG;
    e->v.l = l->v.l << r->v.l;

    return 0;
}

static int op_shr(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, lgx_expr_result_t* l, lgx_expr_result_t* r) {
    assert(check_constant(l, T_LONG));
    assert(check_constant(r, T_LONG));

    e->type = EXPR_LITERAL;
    e->v_type.type = T_LONG;
    e->v.l = l->v.l >> r->v.l;

    return 0;
}

static int binary_operator(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, lgx_expr_result_t* l, lgx_expr_result_t* r) {
    if (is_constant(l) && is_constant(r)) {
        switch (node->u.op) {
            case TK_ADD: return op_add(c, node, e, l, r);
            case TK_SUB: return op_sub(c, node, e, l, r);
            case TK_MUL: return op_mul(c, node, e, l, r);
            case TK_DIV: return op_div(c, node, e, l, r);
            case TK_MOD: return op_mod(c, node, e, l, r);
            case TK_EQUAL: return op_eq(c, node, e, l, r);
            case TK_NOT_EQUAL: return op_ne(c, node, e, l, r);
            case TK_GREATER: return op_gt(c, node, e, l, r);
            case TK_GREATER_EQUAL: return op_ge(c, node, e, l, r);
            case TK_LESS: return op_lt(c, node, e, l, r);
            case TK_LESS_EQUAL: return op_le(c, node, e, l, r);
            case TK_LEFT_BRACK: return op_index(c, node, e, l, r);
            case TK_CONCAT: return op_concat(c, node, e, l, r);
            case TK_SHL: return op_shl(c, node, e, l, r);
            case TK_SHR: return op_shr(c, node, e, l, r);
            default:
                compiler_error(c, node, "unknown binary operator %d\n", node->u.op);
                return 1;
        }
    }
    
    if (0) {
        // TODO 如果一个操作数为立即数并且范围在 0 - 255 之内，使用立即数指令
    }

    // 编译为普通指令
    int r1 = -1, r2 = -1, ret = 0;;

    do {
        if (is_local(l) || is_temp(l)) {
            r1 = l->u.local;
        } else {
            r1 = load_to_reg(c, node, l);
            if (r1 < 0) {
                ret = 1;
                break;
            }
        }

        if (is_local(r) || is_temp(r)) {
            r2 = r->u.local;
        } else {
            r2 = load_to_reg(c, node, r);
            if (r2 < 0) {
                ret = 1;
                break;
            }
        }
        
        e->type = EXPR_TEMP;
        e->u.local = reg_pop(c, node);
        switch (node->u.op) {
            case TK_ADD: ret = bc_add(c, e->u.local, r1, r2); break;
            case TK_SUB: ret = bc_sub(c, e->u.local, r1, r2); break;
            case TK_MUL: ret = bc_mul(c, e->u.local, r1, r2); break;
            case TK_DIV: ret = bc_div(c, e->u.local, r1, r2); break;
            //case TK_MOD: ret = bc_mod(c, e->u.local, r1, r2); break;
            case TK_EQUAL: ret = bc_eq(c, e->u.local, r1, r2); break;
            case TK_NOT_EQUAL: ret = bc_ne(c, e->u.local, r1, r2); break;
            case TK_GREATER: ret = bc_gt(c, e->u.local, r1, r2); break;
            case TK_GREATER_EQUAL: ret = bc_ge(c, e->u.local, r1, r2); break;
            case TK_LESS: ret = bc_lt(c, e->u.local, r1, r2); break;
            case TK_LESS_EQUAL: ret = bc_le(c, e->u.local, r1, r2); break;
            case TK_LEFT_BRACK: ret = bc_array_get(c, e->u.local, r1, r2); break;
            case TK_CONCAT: bc_concat(c, e->u.local, r1, r2); break;
            case TK_SHL: bc_shl(c, e->u.local, r1, r2); break;
            case TK_SHR: bc_shr(c, e->u.local, r1, r2); break;
            default:
                compiler_error(c, node, "unknown binary operator %d\n", node->u.op);
                return 1;
        }
    } while (0);

    if (!is_local(l) && r1 >= 0) {
        reg_push(c, node, r1);
    }

    if (!is_local(r) && r2 >= 0) {
        reg_push(c, node, r2);
    }

    return ret;
}

static int op_lnot(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, lgx_expr_result_t* r) {
    if (check_type(r, T_BOOL)) {
        e->type = EXPR_LITERAL;
        e->v_type.type = T_BOOL;
        e->v.l = !r->v.l;
    } else {
        compiler_error(c, node, "invalid operand type for operator %%\n");
        return 1;
    }

    return 0;
}

static int op_not(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, lgx_expr_result_t* r) {
    if (check_type(r, T_LONG)) {
        e->type = EXPR_LITERAL;
        e->v_type.type = T_LONG;
        e->v.l = ~r->v.l;
    } else {
        compiler_error(c, node, "invalid operand type for operator %%\n");
        return 1;
    }

    return 0;
}

static int op_neg(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, lgx_expr_result_t* r) {
    if (check_type(r, T_LONG)) {
        e->type = EXPR_LITERAL;
        e->v_type.type = T_LONG;
        e->v.l = -r->v.l;
    } else if (check_type(r, T_DOUBLE)) {
        e->type = EXPR_LITERAL;
        e->v_type.type = T_DOUBLE;
        e->v.d = -r->v.d;
    } else {
        compiler_error(c, node, "invalid operand type for operator %%\n");
        return 1;
    }

    return 0;
}

static int unary_operator(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, lgx_expr_result_t* r) {
    if (is_constant(r)) {
        switch (node->u.op) {
            case TK_LOGIC_NOT: return op_lnot(c, node, e, r);
            case TK_NOT: return op_not(c, node, e, r);
            case TK_SUB: return op_neg(c, node, e, r);
            default:
                compiler_error(c, node, "unknown unary operator %d\n", node->u.op);
                return 1;
        }
    } else {
        int r1 = -1, ret = 0;;

        do {
            if (is_local(r) || is_temp(r)) {
                r1 = r->u.local;
            } else {
                r1 = load_to_reg(c, node, r);
                if (r1 < 0) {
                    ret = 1;
                    break;
                }
            }
            
            e->type = EXPR_TEMP;
            e->u.local = reg_pop(c, node);
            switch (node->u.op) {
                case TK_LOGIC_NOT: return bc_lnot(c, e->u.local, r1);
                case TK_NOT: return bc_not(c, e->u.local, r1);
                case TK_SUB: return bc_neg(c, e->u.local, r1);
                default:
                    compiler_error(c, node, "unknown unary operator %d\n", node->u.op);
                    return 1;
            }
        } while (0);

        if (!is_local(r) && r1 >= 0) {
            reg_push(c, node, r1);
        }

        return ret;
    }
}

static void compiler_type_error(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_type_t* type1, lgx_type_t* type2) {
    lgx_str_t t1, t2;
    lgx_str_set_null(t1);
    lgx_str_set_null(t2);

    lgx_type_to_string(type1, &t1);
    lgx_type_to_string(type2, &t2);

    compiler_error(c, node, "makes %.*s from %.*s without a cast\n", t1.length, t1.buffer, t2.length, t2.buffer);

    lgx_str_cleanup(&t1);
    lgx_str_cleanup(&t2);
}

static int compiler_expression(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e);

static int compiler_binary_expression_logic_and(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    assert(node->type == BINARY_EXPRESSION);
    assert(node->u.op == TK_LOGIC_AND);

    // if (e1 == false) {
    //     e = false;
    // } else {
    //     e = e2;
    // }

    // 禁止在逻辑操作中使用赋值表达式，无论它的返回值是否为布尔值
    if (compiler_check_assignment(c, node->child[0])) {
        return 1;
    }
    if (compiler_check_assignment(c, node->child[1])) {
        return 1;
    }

    int ret = 0;

    lgx_expr_result_t e1;
    lgx_expr_result_init(&e1);

    if (compiler_expression(c, node->child[0], &e1)) {
        ret = 1;
    }

    if (check_constant(&e1, T_BOOL)) {
        if (e1.v.l == 0) {
            e->type = EXPR_LITERAL;
            e->v_type.type = T_BOOL;
            e->v.l = 0;
        } else {
            lgx_expr_result_t e2;
            lgx_expr_result_init(&e2);

            if (compiler_expression(c, node->child[1], &e2)) {
                ret = 1;
            }

            if (!check_type(&e2, T_BOOL)) {
                lgx_type_t t;
                lgx_type_init(&t, T_BOOL);
                compiler_type_error(c, node, &t, &e2.v_type);
                ret = 1;
            }

            *e = e2;
        }
    } else if (check_variable(&e1, T_BOOL)) {
        e->type = EXPR_TEMP;
        e->v_type.type = T_BOOL;
        e->u.local = reg_pop(c, node);
        if (e->u.local < 0) {
            ret = 1;
        }

        int pos1 = c->bc.length;

        if (is_local(&e1) || is_temp(&e1)) {
            bc_test(c, e1.u.local, 0);
        } else {
            int r = load_to_reg(c, node, &e1);
            if (r < 0) {
                ret = 1;
            }
            bc_test(c, r, 0);
            reg_push(c, node, r);
        }

        // e1 == true
        lgx_expr_result_t e2;
        lgx_expr_result_init(&e2);

        if (compiler_expression(c, node->child[1], &e2)) {
            ret = 1;
        }

        if (!check_type(&e2, T_BOOL)) {
            lgx_type_t t;
            lgx_type_init(&t, T_BOOL);
            compiler_type_error(c, node, &t, &e2.v_type);
            ret = 1;
        }

        if (is_local(&e2) || is_temp(&e2)) {
            bc_mov(c, e->u.local, e2.u.local);
        } else {
            int r = load_to_reg(c, node, &e2);
            if (r < 0) {
                ret = 1;
            }
            bc_mov(c, e->u.local, r);
            reg_push(c, node, r);
        }

        int pos2 = c->bc.length;
        bc_jmpi(c, 0);

        bc_set_param_d(c, pos1, c->bc.length - pos1 - 1);

        // e1 == false
        lgx_expr_result_t tmp;
        tmp.type = EXPR_LITERAL;
        tmp.v_type.type = T_BOOL;
        tmp.v.l = 0;

        int reg = lgx_const_get(&c->constant, &tmp);
        assert(reg >= 0);
        bc_load(c, e->u.local, reg);

        bc_set_param_e(c, pos2, c->bc.length);

        lgx_expr_result_cleanup(c, node, &e2);
    } else {
        lgx_type_t t;
        lgx_type_init(&t, T_BOOL);
        compiler_type_error(c, node, &t, &e1.v_type);
        ret = 1;
    }

    lgx_expr_result_cleanup(c, node, &e1);

    return ret;
}

static int compiler_binary_expression_logic_or(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    assert(node->type == BINARY_EXPRESSION);
    assert(node->u.op == TK_LOGIC_OR);

    // if (e1 == true) {
    //     e = true;
    // } else {
    //     e = e2;
    // }

    // 禁止在逻辑操作中使用赋值表达式，无论它的返回值是否为布尔值
    if (compiler_check_assignment(c, node->child[0])) {
        return 1;
    }
    if (compiler_check_assignment(c, node->child[1])) {
        return 1;
    }

    int ret = 0;

    lgx_expr_result_t e1;
    lgx_expr_result_init(&e1);

    if (compiler_expression(c, node->child[0], &e1)) {
        ret = 1;
    }

    if (check_constant(&e1, T_BOOL)) {
        if (e1.v.l) {
            *e = e1;
        } else {
            lgx_expr_result_t e2;
            lgx_expr_result_init(&e2);

            if (compiler_expression(c, node->child[1], &e2)) {
                ret = 1;
            }

            if (!check_type(&e2, T_BOOL)) {
                lgx_type_t t;
                lgx_type_init(&t, T_BOOL);
                compiler_type_error(c, node, &t, &e2.v_type);
                ret = 1;
            }

            *e = e2;
        }
    } else if (check_variable(&e1, T_BOOL)) {
        e->type = EXPR_TEMP;
        e->v_type.type = T_BOOL;
        e->u.local = reg_pop(c, node);
        if (e->u.local < 0) {
            ret = 1;
        }

        int pos1 = c->bc.length;

        if (is_local(&e1) || is_temp(&e1)) {
            bc_test(c, e1.u.local, 0);
        } else {
            int r = load_to_reg(c, node, &e1);
            if (r < 0) {
                ret = 1;
            }
            bc_test(c, r, 0);
            reg_push(c, node, r);
        }

        // e1 == true
        lgx_expr_result_t tmp;
        tmp.type = EXPR_LITERAL;
        tmp.v_type.type = T_BOOL;
        tmp.v.l = 1;

        int reg = lgx_const_get(&c->constant, &tmp);
        assert(reg >= 0);
        bc_load(c, e->u.local, reg);

        int pos2 = c->bc.length;
        bc_jmpi(c, 0);

        bc_set_param_d(c, pos1, c->bc.length - pos1 - 1);

        // e1 == false
        lgx_expr_result_t e2;
        lgx_expr_result_init(&e2);

        if (compiler_expression(c, node->child[1], &e2)) {
            ret = 1;
        }

        if (!check_type(&e2, T_BOOL)) {
            lgx_type_t t;
            lgx_type_init(&t, T_BOOL);
            compiler_type_error(c, node, &t, &e2.v_type);
            ret = 1;
        }

        if (is_local(&e2) || is_temp(&e2)) {
            bc_mov(c, e->u.local, e2.u.local);
        } else {
            int r = load_to_reg(c, node, &e2);
            if (r < 0) {
                ret = 1;
            }
            bc_mov(c, e->u.local, r);
            reg_push(c, node, r);
        }

        bc_set_param_e(c, pos2, c->bc.length);

        lgx_expr_result_cleanup(c, node, &e2);
    } else {
        lgx_type_t t;
        lgx_type_init(&t, T_BOOL);
        compiler_type_error(c, node, &t, &e1.v_type);
        ret = 1;
    }

    return ret;
}

static int compiler_binary_expression_math(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    assert(node->type == BINARY_EXPRESSION);

    int ret = 0;

    lgx_expr_result_t e1, e2;
    lgx_expr_result_init(&e1);
    lgx_expr_result_init(&e2);

    if (compiler_expression(c, node->child[0], &e1)) {
        ret = 1;
    }

    if (compiler_expression(c, node->child[1], &e2)) {
        ret = 1;
    }

    if (node->u.op == TK_ADD && check_type(&e1, T_STRING) && check_type(&e2, T_STRING)) {
        e->v_type.type = T_STRING;
        node->u.op = TK_CONCAT;
        ret = binary_operator(c, node, e, &e1, &e2);
    } else if ((check_type(&e1, T_LONG) && check_type(&e2, T_LONG)) ||
        (check_type(&e1, T_DOUBLE) && check_type(&e2, T_DOUBLE))) {
        e->v_type.type = e1.v_type.type;
        switch (node->u.op) {
            case TK_ADD:
            case TK_SUB:
            case TK_MUL:
            case TK_DIV:
            case TK_MOD:
                ret = binary_operator(c, node, e, &e1, &e2);
                break;
            default:
                compiler_error(c, node, "unknown math operator %d\n", node->u.op);
                ret = 1;
        }
    } else {
        compiler_error(c, node, "invalid math expression\n");
        ret = 1;
    }

    lgx_expr_result_cleanup(c, node, &e2);
    lgx_expr_result_cleanup(c, node, &e1);

    return ret;
}

static int compiler_binary_expression_relation(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    assert(node->type == BINARY_EXPRESSION);

    int ret = 0;

    lgx_expr_result_t e1, e2;
    lgx_expr_result_init(&e1);
    lgx_expr_result_init(&e2);

    if (compiler_expression(c, node->child[0], &e1)) {
        ret = 1;
    }

    if (compiler_expression(c, node->child[1], &e2)) {
        ret = 1;
    }

    if (node->u.op == TK_EQUAL && check_type(&e1, T_STRING) && check_type(&e2, T_STRING)) {
        // TODO 字符串比较
    } else if ((check_type(&e1, T_LONG) && check_type(&e2, T_LONG)) ||
        (check_type(&e1, T_DOUBLE) && check_type(&e2, T_DOUBLE))) {
        switch (node->u.op) {
            case TK_EQUAL:
                if (check_type(&e1, T_DOUBLE) && check_type(&e2, T_DOUBLE)) {
                    // TODO warning: 对浮点数使用 == 操作符可能不会得到预期结果
                }
            case TK_NOT_EQUAL:
            case TK_GREATER:
            case TK_GREATER_EQUAL:
            case TK_LESS:
            case TK_LESS_EQUAL:
                ret = binary_operator(c, node, e, &e1, &e2);
                break;
            default:
                compiler_error(c, node, "unknown relation operator %d\n", node->u.op);
                ret = 1;
        }
    } else {
        compiler_error(c, node, "invalid relation expression\n");
        ret = 1;
    }

    e->v_type.type = T_BOOL;

    lgx_expr_result_cleanup(c, node, &e2);
    lgx_expr_result_cleanup(c, node, &e1);

    return ret;
}

static int compiler_binary_expression_bitwise(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    assert(node->type == BINARY_EXPRESSION);

    int ret = 0;

    lgx_expr_result_t e1, e2;
    lgx_expr_result_init(&e1);
    lgx_expr_result_init(&e2);

    if (compiler_expression(c, node->child[0], &e1)) {
        ret = 1;
    }

    if (compiler_expression(c, node->child[1], &e2)) {
        ret = 1;
    }

    if (check_type(&e1, T_LONG) && check_type(&e2, T_LONG)) {
        switch (node->u.op) {
            case TK_SHL:
            case TK_SHR:
                ret = binary_operator(c, node, e, &e1, &e2);
                break;
            default:
                compiler_error(c, node, "unknown bitwise operator %d\n", node->u.op);
                ret = 1;
        }
    } else {
        compiler_error(c, node, "invalid bitwise expression\n");
        ret = 1;
    }

    e->v_type.type = T_LONG;

    lgx_expr_result_cleanup(c, node, &e2);
    lgx_expr_result_cleanup(c, node, &e1);

    return ret;
}

static int compiler_binary_expression_assignment(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    assert(node->type == BINARY_EXPRESSION);

    int ret = 0;

    lgx_expr_result_t e1, e2;
    lgx_expr_result_init(&e1);
    lgx_expr_result_init(&e2);

    if (compiler_expression(c, node->child[1], &e2)) {
        ret = 1;
    }

    int r;
    if (is_local(&e2) || is_temp(&e2)) {
        r = e2.u.local;
    } else {
        r = load_to_reg(c, node, &e2);
        if (r < 0) {
            ret = 1;
        }
    }

    if (node->child[0]->type == IDENTIFIER_TOKEN) {
        // 局部变量&全局变量赋值
        if (compiler_expression(c, node->child[0], &e1)) {
            ret = 1;
        }

        if (e2.type == EXPR_LITERAL) {
            if (!lgx_type_is_fit(&e2.v_type, &e1.v_type)) {
                compiler_type_error(c, node, &e1.v_type, &e2.v_type);
                ret = 1;
            }
        } else {
            if (lgx_type_cmp(&e1.v_type, &e2.v_type)) {
                compiler_type_error(c, node, &e1.v_type, &e2.v_type);
                ret = 1;
            }
        }

        if (is_global(&e1)) {
            bc_global_set(c, e1.u.global, r);
        } else if (is_local(&e1)) {
            bc_mov(c, e1.u.local, r);
        } else {
            compiler_error(c, node, "invalid left variable for assignment\n");
            ret = 1;
        }
    } else if (node->child[0]->type == BINARY_EXPRESSION && node->child[0]->u.op == TK_LEFT_BRACK) {
        assert(node->child[0]->children == 1 || node->child[0]->children == 2);

        // 数组&图赋值
        if (compiler_expression(c, node->child[0]->child[0], &e1)) {
            ret = 1;
        }

        if (check_variable(&e1, T_ARRAY)) {
            if (e2.type == EXPR_LITERAL) {
                if (!lgx_type_is_fit(&e2.v_type, &e1.v_type.u.arr->value)) {
                    compiler_type_error(c, node, &e1.v_type.u.arr->value, &e2.v_type);
                    ret = 1;
                }
            } else {
                if (lgx_type_cmp(&e1.v_type.u.arr->value, &e2.v_type)) {
                    compiler_type_error(c, node, &e1.v_type.u.arr->value, &e2.v_type);
                    ret = 1;
                }
            }

            int index = 0;
            lgx_expr_result_t k;
            lgx_expr_result_init(&k);
            if (node->child[0]->children == 2) {
                if (compiler_expression(c, node->child[0]->child[1], &k)) {
                    ret = 1;
                }

                if (is_local(&k) || is_temp(&k)) {
                    index = k.u.local;
                } else {
                    index = load_to_reg(c, node, &k);
                    if (index < 0) {
                        ret = 1;
                    }
                }
            }

            int reg;
            if (is_local(&e1) || is_temp(&e1)) {
                reg = e1.u.local;
            } else {
                reg = load_to_reg(c, node, &e1);
                if (reg < 0) {
                    ret = 1;
                }
            }

            bc_array_set(c, reg, index, r);

            if (!is_local(&e1) && !is_temp(&e1)) {
                reg_push(c, node, reg);
            }

            if (node->child[0]->children == 2) {
                if (!is_local(&k) && !is_temp(&k)) {
                    reg_push(c, node, index);
                }
            }
            lgx_expr_result_cleanup(c, node, &k);
        } else if (check_variable(&e1, T_MAP)) {
            // TODO
        } else {
            compiler_error(c, node, "left variable should be array or map\n");
            ret = 1;
        }
    } else if (node->child[0]->type == BINARY_EXPRESSION &&
        (node->child[0]->u.op == TK_DOT || node->child[0]->u.op == TK_ARROW)) {
        // TODO 结构体赋值
    } else {
        compiler_error(c, node, "invalid left variable for assignment\n");
        ret = 1;
    }

    if (!is_local(&e2) && !is_temp(&e2) && r >= 0) {
        reg_push(c, node, r);
    }

    // TODO 设置表达式返回值

    lgx_expr_result_cleanup(c, node, &e2);
    lgx_expr_result_cleanup(c, node, &e1);

    return ret;
}

static int compiler_binary_expression_call(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e, unsigned is_tail) {
    assert(node->type == BINARY_EXPRESSION);
    assert(node->u.op == TK_LEFT_PAREN);
    assert(node->children == 2);
    assert(node->child[1]->type == FUNCTION_CALL_PARAMETER);

    int ret = 0;

    lgx_expr_result_t e1;
    lgx_expr_result_init(&e1);

    if (node->child[0]->type == BINARY_EXPRESSION &&
        (node->child[0]->u.op == TK_DOT || node->child[0]->u.op == TK_ARROW)) {
        // TODO 方法调用
    } else {
        if (compiler_expression(c, node->child[0], &e1)) {
            ret = 1;
        }
    }

    if (!check_type(&e1, T_FUNCTION)) {
        compiler_error(c, node, "`cannot call a non-function\n");
        return 1;
    }

    lgx_type_function_t* fun = e1.v_type.u.fun;

    // 实参数量必须等于形参
    if (node->child[1]->children != fun->arg_len) {
        compiler_error(c, node, "arguments length mismatch\n");
        ret = 1;
    }

    // 编译参数
    int i;
    lgx_expr_result_t *expr = (lgx_expr_result_t *)xcalloc(node->child[1]->children, sizeof(lgx_expr_result_t));
    for(i = 0; i < node->child[1]->children; i++) {
        if (compiler_expression(c, node->child[1]->child[i], &expr[i])) {
            ret = 1;
        }
    }

    if (node->child[0]->type == BINARY_EXPRESSION &&
        (node->child[0]->u.op == TK_DOT || node->child[0]->u.op == TK_ARROW)) {
        // TODO 方法调用，传入 receiver
    }

    int r;

    if (is_local(&e1) || is_temp(&e1)) {
        r = e1.u.local;
    } else {
        r = load_to_reg(c, node, &e1);
        if (r < 0 ) {
            ret = 1;
        }
    }

    bc_call_new(c, r);

    int base = 4;
    for(i = 0; i < node->child[1]->children; i++) {
        if (lgx_type_cmp(&fun->args[i], &expr[i].v_type)) {
            compiler_type_error(c, node, &fun->args[i], &expr[i].v_type);
            ret = 1;
        }

        if (is_local(&expr[i]) || is_temp(&expr[i])) {
            bc_call_set(c, i + base, expr[i].u.local);
        } else {
            int r = load_to_reg(c, node, &expr[i]);
            if (r < 0 ) {
                ret = 1;
            }
            bc_call_set(c, i + base, r);
        }
        
        lgx_expr_result_cleanup(c, node, &expr[i]);
    }

    xfree(expr);

    e->type = EXPR_TEMP;
    e->u.local = reg_pop(c, node);
    if (is_tail) {
        bc_tail_call(c, r);
    } else {
        bc_call(c, r, e->u.local);
    }
    
    if (!is_local(&e1) && !is_temp(&e1) && r >= 0) {
        reg_push(c, node, r);
    }

    // 设置返回值类型
    if (lgx_type_dup(&fun->ret, &e->v_type)) {
        ret = 1;
    }

    lgx_expr_result_cleanup(c, node, &e1);

    return ret;
}

static int compiler_binary_expression_index(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    assert(node->type == BINARY_EXPRESSION);
    assert(node->u.op == TK_LEFT_BRACK);
    assert(node->children == 2);

    int ret = 0;

    lgx_expr_result_t e1, e2;
    lgx_expr_result_init(&e1);
    lgx_expr_result_init(&e2);

    if (compiler_expression(c, node->child[0], &e1)) {
        ret = 1;
    }

    if (!check_type(&e1, T_ARRAY)) {
        compiler_error(c, node, "cannot index into non-array\n");
        return 1;
    }

    if (compiler_expression(c, node->child[1], &e2)) {
        ret = 1;
    }

    // TODO 这里应该也能够接受任意实现了 toString 方法的类型
    if (!check_type(&e2, T_LONG) && !!check_type(&e1, T_STRING)) {
        compiler_error(c, node, "key of array should be integer or string\n");
        ret = 1;
    }

    if (binary_operator(c , node, e, &e1, &e2)) {
        ret = 1;
    }

    // 设置返回值类型
    if (e->v_type.type == T_UNKNOWN) {
        if (lgx_type_dup(&e1.v_type.u.arr->value, &e->v_type)) {
            ret = 1;
        }
    }

    lgx_expr_result_cleanup(c, node, &e2);
    lgx_expr_result_cleanup(c, node, &e1);

    return ret;
}

static int compiler_binary_expression(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    assert(node->type == BINARY_EXPRESSION);

    int ret = 0;

    switch (node->u.op) {
        case TK_LOGIC_AND:
            ret = compiler_binary_expression_logic_and(c, node, e);
            break;
        case TK_LOGIC_OR:
            ret = compiler_binary_expression_logic_or(c, node, e);
            break;
        case TK_ADD: case TK_SUB: case TK_MUL: case TK_DIV: case TK_MOD:
            ret = compiler_binary_expression_math(c, node, e);
            break;
        case TK_EQUAL: case TK_NOT_EQUAL:
        case TK_GREATER: case TK_LESS:
        case TK_GREATER_EQUAL: case TK_LESS_EQUAL:
            ret = compiler_binary_expression_relation(c, node, e);
            break;
        case TK_SHL: case TK_SHR:
        case TK_AND: case TK_OR: case TK_XOR:
            ret = compiler_binary_expression_bitwise(c, node, e);
            break;
        case TK_ASSIGN:
        // TODO
        /*
	    case TK_ADD_ASSIGN: case TK_SUB_ASSIGN:
	    case TK_MUL_ASSIGN: case TK_DIV_ASSIGN:
	    case TK_MOD_ASSIGN:
	    case TK_AND_ASSIGN: case TK_OR_ASSIGN:
	    case TK_XOR_ASSIGN: case TK_NOT_ASSIGN:
	    case TK_SHL_ASSIGN: case TK_SHR_ASSIGN:
        */
            ret = compiler_binary_expression_assignment(c, node, e);
            break;
        case TK_LEFT_PAREN:
            ret = compiler_binary_expression_call(c, node, e, 0);
            break;
        case TK_LEFT_BRACK:
            ret = compiler_binary_expression_index(c, node, e);
            break;
        default:
            compiler_error(c, node, "unknown binary operator %d\n", node->u.op);
            ret = 1;
    }

    return ret;
}

static int compiler_unary_expression_logic_not(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    assert(node->type == UNARY_EXPRESSION);
    assert(node->u.op == TK_LOGIC_NOT);

    int ret = 0;

    lgx_expr_result_t e1;
    lgx_expr_result_init(&e1);

    if (compiler_expression(c, node->child[0], &e1)) {
        ret = 1;
    }

    if (check_type(&e1, T_BOOL)) {
        if (unary_operator(c, node, e, &e1)) {
            ret = 1;
        }
    } else {
        lgx_type_t t;
        lgx_type_init(&t, T_BOOL);
        compiler_type_error(c, node, &t, &e1.v_type);
        ret = 1;
    }

    lgx_expr_result_cleanup(c, node, &e1);

    return ret;
}

static int compiler_unary_expression_bitwise_not(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    assert(node->type == UNARY_EXPRESSION);
    assert(node->u.op == TK_NOT);

    int ret = 0;

    lgx_expr_result_t e1;
    lgx_expr_result_init(&e1);

    if (compiler_expression(c, node->child[0], &e1)) {
        ret = 1;
    }

    if (check_type(&e1, T_LONG)) {
        if (unary_operator(c, node, e, &e1)) {
            ret = 1;
        }
    } else {
        lgx_type_t t;
        lgx_type_init(&t, T_LONG);
        compiler_type_error(c, node, &t, &e1.v_type);
        ret = 1;
    }

    lgx_expr_result_cleanup(c, node, &e1);

    return ret;
}

static int compiler_unary_expression_math_negative(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    assert(node->type == UNARY_EXPRESSION);
    assert(node->u.op == TK_SUB);

    int ret = 0;

    lgx_expr_result_t e1;
    lgx_expr_result_init(&e1);

    if (compiler_expression(c, node->child[0], &e1)) {
        ret = 1;
    }

    if (check_type(&e1, T_LONG) || check_type(&e1, T_DOUBLE)) {
        if (unary_operator(c, node, e, &e1)) {
            ret = 1;
        }
    } else {
        lgx_type_t t;
        lgx_type_init(&t, T_LONG); // TODO 也可以是浮点数
        compiler_type_error(c, node, &t, &e1.v_type);
        ret = 1;
    }

    lgx_expr_result_cleanup(c, node, &e1);

    return ret;
}

static int compiler_unary_expression(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    assert(node->type == UNARY_EXPRESSION);

    int ret = 0;

    switch (node->u.op) {
        case TK_LOGIC_NOT:
            ret = compiler_unary_expression_logic_not(c, node, e);
            break;
        case TK_NOT:
            ret = compiler_unary_expression_bitwise_not(c, node, e);
            break;
        case TK_SUB:
            ret = compiler_unary_expression_math_negative(c, node, e);
            break;
        default:
            compiler_error(c, node, "unknown unary operator %d\n", node->u.op);
            return 1;
    }

    return ret;
}

static int compiler_array_expression(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    assert(node->type == ARRAY_EXPRESSION);

    unsigned pos = c->bc.length;
    unsigned is_const = 1;
    int ret = 0;

    e->type = EXPR_TEMP;
    if (lgx_type_init(&e->v_type, T_ARRAY)) {
        compiler_error(c, node, "out of memory\n");
        return 1;
    }
    if (lgx_type_init(&e->v_type.u.arr->value, T_UNKNOWN)) {
        compiler_error(c, node, "out of memory\n");
        return 1;
    }
    if (lgx_ht_init(&e->v.arr, node->children)) {
        compiler_error(c, node, "out of memory\n");
        return 1;
    }

    e->u.local = reg_pop(c, node);
    if (e->u.local < 0) {
        ret = 1;
    }

    bc_array_new(c, e->u.local);

    int i;
    for(i = 0; i < node->children; i++) {
        lgx_expr_result_t t;
        lgx_expr_result_init(&t);
        if (compiler_expression(c, node->child[i], &t)) {
            ret = 1;
        } else {
            if (lgx_type_is_definite(&e->v_type.u.arr->value)) {
                if (t.type == EXPR_LITERAL) {
                    if (!lgx_type_is_fit(&t.v_type, &e->v_type.u.arr->value)) {
                        compiler_type_error(c, node, &t.v_type, &e->v_type.u.arr->value);
                        ret = 1;
                    }
                } else {
                    if (lgx_type_cmp(&t.v_type, &e->v_type.u.arr->value)) {
                        compiler_type_error(c, node, &t.v_type, &e->v_type.u.arr->value);
                        ret = 1;
                    }
                }
            } else {
                if (lgx_type_inference(&t.v_type, &e->v_type.u.arr->value)) {
                    compiler_type_error(c, node, &e->v_type.u.arr->value, &t.v_type);
                    ret = 1;
                }
            }

            if (is_const && is_constant(&t)) {
                lgx_value_t* v = xcalloc(1, sizeof(lgx_value_t));
                if (v) {
                    if (lgx_expr_to_value(&t, v)) {
                        xfree(v);
                        ret = 1;
                    } else {
                        lgx_str_t key;
                        long long num = i;
                        key.buffer = (char *)&num;
                        key.length = sizeof(num);
                        key.size = 0;
                        if (lgx_ht_set(&e->v.arr, &key, v)) {
                            lgx_value_cleanup(v);
                            xfree(v);
                            ret = 1;
                        }
                    }
                } else {
                    ret = 1;
                }
            } else {
                is_const = 0;
            }
            if (is_temp(&t) || is_local(&t)) {
                bc_array_add(c, e->u.local, t.u.local);
            } else {
                int r = load_to_reg(c, node, &t);
                if (r < 0) {
                    ret = 1;
                }
                bc_array_add(c, e->u.local, r);
                reg_push(c, node, r);
            }
        }
        lgx_expr_result_cleanup(c, node, &t);
    }

    // 如果是常量数组，则无需创建临时数组
    if (is_const) {
        // 丢弃之前生成的用于创建临时数组的字节码
        c->bc.length = pos;

        reg_push(c, node, e->u.local);
        e->u.local = 0;

        e->type = EXPR_LITERAL;
        e->literal.buffer = c->ast->lex.source.content + node->offset;
        e->literal.length = node->length;
        e->literal.size = 0;
    } else {
        // 丢弃解析的常量数组
        lgx_array_value_cleanup(&e->v.arr);
        lgx_ht_cleanup(&e->v.arr);
    }

    return ret;
}

static int compiler_expression(lgx_compiler_t* c, lgx_ast_node_t *node, lgx_expr_result_t* e) {
    switch (node->type) {
        case STRING_TOKEN:
            return compiler_string_token(c, node, e);
        case CHAR_TOKEN:
            return compiler_char_token(c, node, e);
        case LONG_TOKEN:
            return compiler_long_token(c, node, e);
        case DOUBLE_TOKEN:
            return compiler_double_token(c, node, e);
        case IDENTIFIER_TOKEN:
            return compiler_identifier_token(c, node, e);
        case TRUE_TOKEN:
            return compiler_true_token(c, node, e);
        case FALSE_TOKEN:
            return compiler_false_token(c, node, e);
        case BINARY_EXPRESSION:
            return compiler_binary_expression(c, node, e);
        case UNARY_EXPRESSION:
            return compiler_unary_expression(c, node, e);
        case ARRAY_EXPRESSION:
            return compiler_array_expression(c, node, e);
        case STRUCT_EXPRESSION:
            return 0;
        default:
            compiler_error(c, node, "expression expected, ast-node type: %d\n", node->type);
            return 1;
    }
}

static int compiler_statement(lgx_compiler_t* c, lgx_ast_node_t *node);
static int compiler_block_statement(lgx_compiler_t* c, lgx_ast_node_t *node);

static int compiler_if_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == IF_STATEMENT);
    assert(node->children == 2);
    assert(node->child[1]->type == BLOCK_STATEMENT);

    if (compiler_check_assignment(c, node->child[0])) {
        return 1;
    }

    lgx_expr_result_t e;
    lgx_expr_result_init(&e);
    if (compiler_expression(c, node->child[0], &e)) {
        return 1;
    }

    if (check_constant(&e, T_BOOL)) {
        if (e.v.l == 0) {
            // if 条件始终为 false
            return 0;
        } else {
            return compiler_block_statement(c, node->child[1]);
        }
    }

    if (check_variable(&e, T_BOOL)) {
        int ret = 0;

        unsigned pos = c->bc.length; // 跳出指令位置
        if (is_global(&e)) {
            int r = load_to_reg(c, node, &e);
            if (r >= 0) {
                bc_test(c, r, 0);
                reg_push(c, node, r);
            } else {
                ret = 1;
            }
        } else {
            bc_test(c, e.u.local, 0);
        }
        lgx_expr_result_cleanup(c, node, &e);

        if (compiler_block_statement(c, node->child[1])) {
            return 1;
        }

        bc_set_param_d(c, pos, c->bc.length - pos - 1);

        return ret;
    }

    lgx_expr_result_cleanup(c, node, &e);

    lgx_type_t t;
    lgx_type_init(&t, T_BOOL);
    compiler_type_error(c, node, &t, &e.v_type);

    return 1;
}

static int compiler_if_else_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == IF_ELSE_STATEMENT);
    assert(node->children == 3);
    assert(node->child[1]->type == BLOCK_STATEMENT);
    assert(node->child[2]->type == BLOCK_STATEMENT || node->child[2]->type == IF_STATEMENT || node->child[2]->type == IF_ELSE_STATEMENT);

    if (compiler_check_assignment(c, node->child[0])) {
        return 1;
    }

    lgx_expr_result_t e;
    lgx_expr_result_init(&e);
    if (compiler_expression(c, node->child[0], &e)) {
        return 1;
    }

    if (check_constant(&e, T_BOOL)) {
        if (e.v.l == 0) {
            if (node->child[2]->type == BLOCK_STATEMENT) {
                return compiler_block_statement(c, node->child[2]);
            } else {
                return compiler_statement(c, node->child[2]);
            }
        } else {
            return compiler_block_statement(c, node->child[1]);
        }
    }

    if (check_variable(&e, T_BOOL)) {
        int ret = 0;

        unsigned pos1 = c->bc.length; // 跳转指令位置
        if (is_global(&e)) {
            int r = load_to_reg(c, node, &e);
            if (r >= 0) {
                bc_test(c, r, 0);
                reg_push(c, node, r);
            } else {
                ret = 1;
            }
        } else {
            bc_test(c, e.u.local, 0);
        }
        lgx_expr_result_cleanup(c, node, &e);

        if (compiler_block_statement(c, node->child[1])) {
            ret = 1;
        }

        unsigned pos2 = c->bc.length; // 跳转指令位置
        bc_jmpi(c, 0);
        bc_set_param_d(c, pos1, c->bc.length - pos1 - 1);

        if (node->child[2]->type == BLOCK_STATEMENT) {
            if (compiler_block_statement(c, node->child[2])) {
                ret = 1;
            }
        } else {
            if (compiler_statement(c, node->child[2])) {
                ret = 1;
            }
        }

        bc_set_param_e(c, pos2, c->bc.length);

        return ret;
    }

    lgx_expr_result_cleanup(c, node, &e);

    lgx_type_t t;
    lgx_type_init(&t, T_BOOL);
    compiler_type_error(c, node, &t, &e.v_type);

    return 1;
}

static int compiler_for_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == FOR_STATEMENT);
    assert(node->children == 4);
    assert(node->child[0]->type == FOR_CONDITION);
    assert(node->child[0]->children <= 1);
    assert(node->child[1]->type == FOR_CONDITION);
    assert(node->child[1]->children <= 1);
    assert(node->child[2]->type == FOR_CONDITION);
    assert(node->child[2]->children <= 1);
    assert(node->child[3]->type == BLOCK_STATEMENT);

    if (node->child[1]->children == 1 && compiler_check_assignment(c, node->child[1]->child[0])) {
        return 1;
    }

    // exp1: 循环开始前执行一次
    if (node->child[0]->children == 1) {
        lgx_expr_result_t e;
        lgx_expr_result_init(&e);
        if (compiler_expression(c, node->child[0]->child[0], &e)) {
            return 1;
        }
        lgx_expr_result_cleanup(c, node, &e);
    }

    int ret = 0;

    unsigned pos1 = c->bc.length; // 跳转指令位置
    unsigned pos2 = 0; // 跳出指令位置

    // exp2: 每次循环开始前执行一次，如果值为假，则跳出循环
    if (node->child[1]->children == 1) {
        lgx_expr_result_t e;
        lgx_expr_result_init(&e);

        if (compiler_expression(c, node->child[1]->child[0], &e)) {
            return 1;
        }

        if (check_constant(&e, T_BOOL)) {
            if (e.v.l == 0) {
                return 0;
            }
        } else if (check_variable(&e, T_BOOL)) {
            pos2 = c->bc.length; 
            if (is_global(&e)) {
                int r = load_to_reg(c, node, &e);
                if (r >= 0) {
                    bc_test(c, r, 0);
                    reg_push(c, node, r);
                } else {
                    ret = 1;
                }
            } else {
                bc_test(c, e.u.local, 0);
            }
            lgx_expr_result_cleanup(c, node, &e);
        } else {
            lgx_type_t t;
            lgx_type_init(&t, T_BOOL);
            compiler_type_error(c, node, &t, &e.v_type);
            return 1;
        }
    }

    // 循环主体
    if (compiler_block_statement(c, node->child[3])) {
        return 1;
    }

    unsigned pos3 = c->bc.length; // 跳转指令位置

    // exp3: 每次循环结束后执行一次
    if (node->child[2]->children == 1) {
        lgx_expr_result_t e;
        lgx_expr_result_init(&e);
        if (compiler_expression(c, node->child[2]->child[0], &e)) {
            return 1;
        }
        lgx_expr_result_cleanup(c, node, &e);
    }

    bc_jmpi(c, pos1);

    if (pos2) {
        bc_set_param_d(c, pos2, c->bc.length - pos2 - 1);
    }

    jmp_fix(c, node, pos3, c->bc.length);

    return ret;
}

static int compiler_while_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == WHILE_STATEMENT);
    assert(node->children == 2);
    assert(node->child[1]->type == BLOCK_STATEMENT);

    if (compiler_check_assignment(c, node->child[0])) {
        return 1;
    }

    int ret = 0;

    unsigned start = c->bc.length; // 循环起始位置

    lgx_expr_result_t e;
    lgx_expr_result_init(&e);
    if (compiler_expression(c, node->child[0], &e)) {
        return 1;
    }

    if (check_constant(&e, T_BOOL)) {
        if (e.v.l == 0) {
            return 0;
        } else {
            if (compiler_block_statement(c, node->child[1])) {
                return 1;
            }
            // 写入无条件跳转
            bc_jmpi(c, start);
        }
    } else if (check_variable(&e, T_BOOL)) {
        unsigned pos = c->bc.length; // 循环跳出指令位置
        if (is_global(&e)) {
            int r = load_to_reg(c, node, &e);
            if (r >= 0) {
                bc_test(c, r, 0);
                reg_push(c, node, r);
            } else {
                ret = 1;
            }
        } else {
            bc_test(c, e.u.local, 0);
        }
        lgx_expr_result_cleanup(c, node, &e);

        if (compiler_block_statement(c, node->child[1])) {
            return 1;
        }

        // 写入无条件跳转
        bc_jmpi(c, start);
        // 更新条件跳转
        bc_set_param_d(c, pos, c->bc.length - pos - 1);
    } else {
        lgx_expr_result_cleanup(c, node, &e);
        lgx_type_t t;
        lgx_type_init(&t, T_BOOL);
        compiler_type_error(c, node, &t, &e.v_type);
        return 1;
    }

    jmp_fix(c, node, start, c->bc.length);

    return ret;
}

static int compiler_do_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == DO_STATEMENT);
    assert(node->children == 2);
    assert(node->child[0]->type == BLOCK_STATEMENT);

    if (compiler_check_assignment(c, node->child[1])) {
        return 1;
    }

    int ret = 0;

    unsigned start = c->bc.length; // 循环起始位置

    if (compiler_block_statement(c, node->child[0])) {
        return 1;
    }

    lgx_expr_result_t e;
    lgx_expr_result_init(&e);
    if (compiler_expression(c, node->child[1], &e)) {
        return 1;
    }

    if (check_constant(&e, T_BOOL)) {
        if (e.v.l == 0) {
            // 循环条件始终为 false
        } else {
            // 写入无条件跳转
            bc_jmpi(c, start);
        }
    } else if (check_variable(&e, T_BOOL)) {
        unsigned pos = c->bc.length;
        if (is_global(&e)) {
            int r = load_to_reg(c, node, &e);
            if (r >= 0) {
                bc_test(c, r, 0);
                reg_push(c, node, r);
            } else {
                ret = 1;
            }
        } else {
            bc_test(c, e.u.local, 0);
        }
        bc_jmpi(c, start);
        lgx_expr_result_cleanup(c, node, &e);

        bc_set_param_d(c, pos, c->bc.length - pos - 1);
    } else {
        lgx_type_t t;
        lgx_type_init(&t, T_BOOL);
        compiler_type_error(c, node, &t, &e.v_type);
        return 1;
    }

    jmp_fix(c, node, start, c->bc.length);

    return ret;
}

static int compiler_continue_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == CONTINUE_STATEMENT);
    assert(node->children == 0);

    // 只能出现在循环语句中
    if (jmp_add(c, node)) {
        // error
        return 1;
    }

    node->u.pos = c->bc.length;
    bc_jmpi(c, 0);

    return 0;
}

static int compiler_break_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == BREAK_STATEMENT);
    assert(node->children == 0);

    // 只能出现在循环语句和 switch 语句中
    if (jmp_add(c, node)) {
        // error
        return 1;
    }

    node->u.pos = c->bc.length;
    bc_jmpi(c, 0);

    return 0;
}

static int compiler_switch_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == SWITCH_STATEMENT);
    assert(node->children >= 1);

    // 执行表达式
    lgx_expr_result_t e;
    lgx_expr_result_init(&e);
    if (compiler_expression(c, node->child[0], &e)) {
        return 1;
    }

    if (!check_type(&e, T_LONG) && !check_type(&e, T_STRING)) {
        compiler_error(c, node, "switch condition should be integer or string\n");
        return 1;
    }

    if (node->children == 1) {
        // 没有 case 和 default，什么都不用做
        lgx_expr_result_cleanup(c, node, &e);
    } else {
/*
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
                    compiler_error(c, child, "only constant expression allowed in case label");
                    return 1;
                }

                if (lgx_hash_get(condition.v.arr, &case_node.k)) {
                    compiler_error(c, child, "duplicate case value");
                    return 1;
                }

                case_nod&e.v_type = T_LONG;
                case_node.v.v.l = 0;

                lgx_hash_set(condition.v.arr, &case_node);
            } else {
                has_default = 1;
            }
        }

        lgx_val_t undef;
        undef.type = T_UNDEFINED;
        undef.u.symbol.reg_type = R_TEMP;
        undef.u.symbol.reg_num = reg_pop(bc);

        lgx_val_t tmp, result;
        tmp.u.symbol.reg_type = R_TEMP;
        tmp.u.symbol.reg_num = reg_pop(bc);
        result.u.symbol.reg_type = R_TEMP;
        result.u.symbol.reg_num = reg_pop(bc);

        bc_load(bc, &tmp, &condition);
        bc_array_get(bc, &tmp, &tmp, &e);
        bc_load(bc, &undef, &undef);
        bc_eq(bc, &result, &tmp, &undef);
        bc_test(bc, &result, 1);

        unsigned pos = c->bc.length;
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
                n->v.v.l = c->bc.length;

                // 这里只应该出现 T_LONG 和 T_STRING
                if (IS_GC_VALUE(&k)) {
                    lgx_val_free(&k);
                }

                if (bc_stat(bc, child->child[1])) {
                    return 1;
                }
            } else { // child->type == DEFAULT_STATEMENT
                bc_set_pe(bc, pos, c->bc.length);

                if (bc_stat(bc, child->child[0])) {
                    return 1;
                }
            }
        }

        if (!has_default) {
            bc_set_pe(bc, pos, c->bc.length);
        }
*/
    }

    jmp_fix(c, node, 0, c->bc.length);
    return 0;
}

static int compiler_try_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == TRY_STATEMENT);
    assert(node->children >= 1);

    int ret = 0;

    lgx_exception_t *exception = lgx_exception_new();

    exception->try_block.start = c->bc.length;
    if (compiler_block_statement(c, node->child[0])) {
        ret = 1;
    }
    exception->try_block.end = c->bc.length;

    if (exception->try_block.start == exception->try_block.end) {
        // try block 没有编译产生任何代码
        lgx_exception_del(exception);
        return ret;
    } else {
        exception->try_block.end --;
    }

    int i;
    for (i = 1; i < node->children; i ++) {
        lgx_ast_node_t *n = node->child[i];
        assert(n->type == CATCH_STATEMENT);

        lgx_exception_block_t *block = lgx_exception_block_new();

        unsigned pos = c->bc.length;
        bc_jmpi(c, 0);

        // 编译 catch block
        block->start = c->bc.length;
        if (compiler_block_statement(c, n->child[1])) {
            ret = 1;
        }
        block->end = c->bc.length;

        if (block->start == block->end) {
            // catch block 没有编译产生任何代码
            bc_nop(c);
        }
        block->end --;

        bc_set_param_e(c, pos, c->bc.length);

        lgx_list_add_tail(&block->head, &exception->catch_blocks);

        // 保存该 catch block 的参数

        // 处理参数
        lgx_str_t param;
        param.buffer = c->ast->lex.source.content + n->child[0]->child[0]->offset;
        param.length = n->child[0]->child[0]->length;
        param.size = 0;

        lgx_symbol_t* symbol = lgx_symbol_get(n->child[1], &param, n->child[1]->offset);
        assert(symbol);

        if (lgx_type_dup(&symbol->type, &block->e)) {
            ret = 1;
        }

        block->reg = symbol->r;
    }

    if (lgx_list_empty(&exception->catch_blocks)) {
        // 如果没有任何 catch block
        lgx_exception_del(exception);
        return ret;
    }

    // 添加 exception
    unsigned long long key = ((unsigned long long)exception->try_block.start << 32) | (0XFFFFFFFF - exception->try_block.end);

    lgx_str_t k;
    k.buffer = (char *)&key;
    k.length = sizeof(key);

    lgx_rb_node_t *n = lgx_rb_set(&c->exception, &k);
    assert(n);
    n->value = exception;

    return ret;
}

static int compiler_throw_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == THROW_STATEMENT);
    assert(node->children == 1);

    lgx_expr_result_t e;
    lgx_expr_result_init(&e);

    if (compiler_expression(c, node->child[0], &e)) {
        return 1;
    }

    int ret = 0;

    if (is_local(&e) || is_temp(&e)) {
        bc_throw(c, e.u.local);
    } else {
        int r = load_to_reg(c, node, &e);
        if (r >= 0) {
            bc_throw(c, r);
            reg_push(c, node, r);
        } else {
            ret = 1;
        }
    }

    lgx_expr_result_cleanup(c, node, &e);

    return ret;
}

static int compiler_return_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == RETURN_STATEMENT);
    assert(node->children == 1);

    lgx_expr_result_t e;
    lgx_expr_result_init(&e);

    int ret = 0;

    // 获取函数原型
    lgx_ast_node_t *n = find_function(node);
    assert(n);

    lgx_str_t name;
    name.buffer = c->ast->lex.source.content + n->child[1]->offset;
    name.length = n->child[1]->length;
    name.size = 0;

    lgx_symbol_t* symbol = lgx_symbol_get(n, &name, n->offset);
    assert(symbol);
    assert(symbol->type.type == T_FUNCTION);

    lgx_type_function_t* fun = symbol->type.u.fun;

    unsigned is_tail = 0;

    // 计算返回值
    if (node->child[0]) {
        if (node->child[0]->type == BINARY_EXPRESSION && node->child[0]->u.op == TK_LEFT_PAREN) {
            // 尾调用
            if (compiler_binary_expression_call(c, node->child[0], &e, 1)) {
                ret = 1;
            }
        } else {
            if (compiler_expression(c, node->child[0], &e)) {
                ret = 1;
            }
        }

        if (lgx_type_cmp(&fun->ret, &e.v_type) != 0) {
            compiler_type_error(c, node, &fun->ret, &e.v_type);
            ret = 1;
        }
    } else {
        if (fun->ret.type != T_UNKNOWN) {
            compiler_error(c, node, "function `%.*s` must return with a value\n", name.length, name.buffer);
            ret = 1;
        }
    }

    // 写入返回指令
    if (!is_tail) {
        if (is_local(&e) || is_temp(&e)) {
            bc_ret(c, e.u.local);
        } else {
            int r = load_to_reg(c, node, &e);
            if (r >= 0) {
                bc_ret(c, r);
                reg_push(c, node, r);
            } else {
                ret = 1;
            }
        }
    }
    
    lgx_expr_result_cleanup(c, node, &e);

    return ret;
}

static int compiler_echo_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == ECHO_STATEMENT);
    assert(node->children == 1);

    lgx_expr_result_t e;
    lgx_expr_result_init(&e);

    if (compiler_expression(c, node->child[0], &e)) {
        return 1;
    }

    int ret = 0;

    if (is_local(&e) || is_temp(&e)) {
        bc_echo(c, e.u.local);
    } else {
        int r = load_to_reg(c, node, &e);
        if (r >= 0) {
            bc_echo(c, r);
            reg_push(c, node, r);
        } else {
            ret = 1;
        }
    }

    lgx_expr_result_cleanup(c, node, &e);

    return ret;
}

static int compiler_expression_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == EXPRESSION_STATEMENT);
    assert(node->children == 1);

    lgx_expr_result_t e;
    lgx_expr_result_init(&e);
    if (compiler_expression(c, node->child[0], &e)) {
        return 1;
    }
    lgx_expr_result_cleanup(c, node, &e);

    return 0;
}

static int compiler_global_variable_declaration(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == VARIABLE_DECLARATION);
    assert(node->children >= 2);
    assert(node->child[0]->type == IDENTIFIER_TOKEN);
    assert(node->child[1]->type == TYPE_EXPRESSION);

    int ret = 0;

    lgx_expr_result_t e1, e2;
    lgx_expr_result_init(&e1);
    lgx_expr_result_init(&e2);

    lgx_str_t name;
    name.buffer = c->ast->lex.source.content + node->child[0]->offset;
    name.length = name.size = node->child[0]->length;

    if (compiler_identifier_token(c, node->child[0], &e1)) {
        ret = 1;
    }

    assert(is_global(&e1));

    if (node->children > 2) {
        if (compiler_expression(c, node->child[2], &e2)) {
            ret = 1;
        }

        // 全局变量只能用常量或字面量初始化
        if (!is_constant(&e2)) {
            compiler_error(c, node, "global variable `%.*s` can only be initialized with constants\n", name.length, name.buffer);
            ret = 1;
        }

        // 类型检查
        if (check_type(&e1, T_UNKNOWN)) {
            // 根据表达式进行类型推断
            if (lgx_type_inference(&e2.v_type, &e1.symbol->type)) {
                compiler_type_error(c, node, &e1.symbol->type, &e2.v_type);
                ret = 1;
            }
            if (!lgx_type_is_definite(&e1.symbol->type)) {
                compiler_error(c, node, "global variable `%.*s` has unknown type\n", name.length, name.buffer);
                ret = 1;
            }
        } else {
            if (e2.type == EXPR_LITERAL) {
                if (!lgx_type_is_fit(&e2.v_type, &e1.v_type)) {
                    compiler_type_error(c, node, &e1.v_type, &e2.v_type);
                    ret = 1;
                }
            } else {
                if (lgx_type_cmp(&e1.v_type, &e2.v_type)) {
                    compiler_type_error(c, node, &e1.v_type, &e2.v_type);
                    ret = 1;
                }
            }
        }

        // 初始化全局变量的值
        if (lgx_expr_to_value(&e2, &e1.symbol->v)) {
            return 1;
        }
    } else {
        if (check_type(&e1, T_UNKNOWN)) {
            compiler_error(c, node, "unable to determine the type of global variable `%.*s`\n", name.length, name.buffer);
            ret = 1;
        }
    }

    lgx_expr_result_cleanup(c, node, &e2);
    lgx_expr_result_cleanup(c, node, &e1);

    return ret;
}

static int compiler_local_variable_declaration(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == VARIABLE_DECLARATION);
    assert(node->children >= 2);
    assert(node->child[0]->type == IDENTIFIER_TOKEN);
    assert(node->child[1]->type == TYPE_EXPRESSION);

    int ret = 0;

    lgx_expr_result_t e1, e2;
    lgx_expr_result_init(&e1);
    lgx_expr_result_init(&e2);

    lgx_str_t name;
    name.buffer = c->ast->lex.source.content + node->child[0]->offset;
    name.length = name.size = node->child[0]->length;

    if (compiler_identifier_token(c, node->child[0], &e1)) {
        ret = 1;
    }

    assert(is_local(&e1));

    if (node->children > 2) {
        if (compiler_expression(c, node->child[2], &e2)) {
            ret = 1;
        }

        // 类型检查
        if (check_type(&e1, T_UNKNOWN)) {
            // 根据表达式进行类型推断
            if (lgx_type_inference(&e2.v_type, &e1.symbol->type)) {
                compiler_type_error(c, node, &e1.symbol->type, &e2.v_type);
                ret = 1;
            }
            if (!lgx_type_is_definite(&e1.symbol->type)) {
                compiler_error(c, node, "local variable `%.*s` has unknown type\n", name.length, name.buffer);
                ret = 1;
            }
        } else {
            if (e2.type == EXPR_LITERAL) {
                if (!lgx_type_is_fit(&e2.v_type, &e1.v_type)) {
                    compiler_type_error(c, node, &e1.v_type, &e2.v_type);
                    ret = 1;
                }
            } else {
                if (lgx_type_cmp(&e1.v_type, &e2.v_type)) {
                    compiler_type_error(c, node, &e1.v_type, &e2.v_type);
                    ret = 1;
                }
            }
        }

        if (is_local(&e2) || is_temp(&e2)) {
            bc_mov(c, e1.u.local, e2.u.local);
        } else {
            int r = load_to_reg(c, node, &e2);
            if (r >= 0) {
                bc_mov(c, e1.u.local, r);
                reg_push(c, node, r);
            } else {
                ret = 1;
            }
        }
    } else {
        if (check_type(&e1, T_UNKNOWN)) {
            compiler_error(c, node, "unable to determine the type of local variable `%.*s`\n", name.length, name.buffer);
            ret = 1;
        }
    }

    lgx_expr_result_cleanup(c, node, &e2);
    lgx_expr_result_cleanup(c, node, &e1);

    return ret;
}

static int compiler_constant_declaration(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == CONSTANT_DECLARATION);
    assert(node->children == 3);
    assert(node->child[0]->type == IDENTIFIER_TOKEN);
    assert(node->child[1]->type == TYPE_EXPRESSION);

    int ret = 0;

    lgx_expr_result_t e1, e2;
    lgx_expr_result_init(&e1);
    lgx_expr_result_init(&e2);

    lgx_str_t name;
    name.buffer = c->ast->lex.source.content + node->child[0]->offset;
    name.length = name.size = node->child[0]->length;

    if (compiler_identifier_token(c, node->child[0], &e1)) {
        ret = 1;
    }

    assert(is_const(&e1));

    if (compiler_expression(c, node->child[2], &e2)) {
        ret = 1;
    }

    if (!is_constant(&e2)) {
        compiler_error(c, node, "invalid constant initializer\n");
        ret = 1;
    } else {
        // 类型检查
        if (check_type(&e1, T_UNKNOWN)) {
            // 根据表达式进行类型推断
            if (lgx_type_inference(&e2.v_type, &e1.symbol->type)) {
                compiler_type_error(c, node, &e1.symbol->type, &e2.v_type);
                ret = 1;
            }
            if (!lgx_type_is_definite(&e1.symbol->type)) {
                compiler_error(c, node, "constant `%.*s` has unknown type\n", name.length, name.buffer);
                ret = 1;
            }
        } else {
            if (e2.type == EXPR_LITERAL) {
                if (!lgx_type_is_fit(&e2.v_type, &e1.v_type)) {
                    compiler_type_error(c, node, &e1.v_type, &e2.v_type);
                    ret = 1;
                }
            } else {
                if (lgx_type_cmp(&e1.v_type, &e2.v_type)) {
                    compiler_type_error(c, node, &e1.v_type, &e2.v_type);
                    ret = 1;
                }
            }
        }

        // 写入常量表
        int r = lgx_const_get(&c->constant, &e2);
        assert(r >= 0);
        e1.symbol->r = r;
    }

    lgx_expr_result_cleanup(c, node, &e2);
    lgx_expr_result_cleanup(c, node, &e1);

    return ret;
}

static int compiler_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    switch(node->type) {
        case IF_STATEMENT: return compiler_if_statement(c, node);
        case IF_ELSE_STATEMENT: return compiler_if_else_statement(c, node);
        case FOR_STATEMENT: return compiler_for_statement(c, node);
        case WHILE_STATEMENT: return compiler_while_statement(c, node);
        case DO_STATEMENT: return compiler_do_statement(c, node);
        case CONTINUE_STATEMENT: return compiler_continue_statement(c, node);
        case BREAK_STATEMENT: return compiler_break_statement(c, node);
        case SWITCH_STATEMENT: return compiler_switch_statement(c, node);
        case TRY_STATEMENT: return compiler_try_statement(c, node);
        case THROW_STATEMENT: return compiler_throw_statement(c, node);
        case RETURN_STATEMENT: return compiler_return_statement(c, node);
        case ECHO_STATEMENT: return compiler_echo_statement(c, node);
        case EXPRESSION_STATEMENT: return compiler_expression_statement(c, node);
        case VARIABLE_DECLARATION: return compiler_local_variable_declaration(c, node);
        case CONSTANT_DECLARATION: return compiler_constant_declaration(c, node);
        case TYPE_DECLARATION:
            // TODO 更新类型表
            break;
        default:
            compiler_error(c, node, "invalid ast-node type %d\n", node->type);
            return 1;
    }

    return 0;
}

static int compiler_block_statement(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == BLOCK_STATEMENT);

    lgx_ast_node_t *fun_node = find_function(node);
    assert(fun_node);

    int ret = 0;

    // 为块作用域的变量分配寄存器
    lgx_ht_node_t* n;
    for (n = lgx_ht_first(node->u.symbols); n; n = lgx_ht_next(n)) {
        lgx_symbol_t *symbol = (lgx_symbol_t *)n->v;
        // 如果符号类型为变量
        if (symbol->s_type == S_VARIABLE) {
            int reg = lgx_reg_pop(fun_node->u.regs);
            if (reg < 0) {
                compiler_error(c, symbol->node, "too many local variables\n");
                ret = 1;
            }
            symbol->r = reg;
        }
    }

    int i;
    for(i = 0; i < node->children; ++i) {
        if (compiler_statement(c, node->child[i])) {
            ret = 1;
        }
    }

    // 释放块作用域的变量
    for (n = lgx_ht_first(node->u.symbols); n; n = lgx_ht_next(n)) {
        lgx_symbol_t *symbol = (lgx_symbol_t *)n->v;
        // 如果符号类型为变量
        if (symbol->s_type == S_VARIABLE) {
            lgx_reg_push(fun_node->u.regs, symbol->r);
        }
    }

    return ret;
}

static int compiler_function_declaration(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == FUNCTION_DECLARATION);
    assert(node->children == 5);
    assert(node->child[0]->type == FUNCTION_RECEIVER);
    assert(node->child[1]->type == IDENTIFIER_TOKEN);
    assert(node->child[2]->type == FUNCTION_DECL_PARAMETER);
    assert(node->child[3]->type == TYPE_EXPRESSION);
    assert(node->child[4]->type == BLOCK_STATEMENT);

    lgx_str_t name;
    name.buffer = c->ast->lex.source.content + node->child[1]->offset;
    name.length = node->child[1]->length;
    name.size = 0;

    lgx_symbol_t* symbol = lgx_symbol_get(node, &name, node->offset);
    assert(symbol);
    assert(symbol->s_type == S_CONSTANT);
    assert(symbol->type.type == T_FUNCTION);
    assert(symbol->v.v.fun == NULL);

    symbol->v.v.fun = xcalloc(1, sizeof(lgx_function_t));
    if (!symbol->v.v.fun) {
        return 1;
    }
    if (lgx_type_dup(&symbol->type, &symbol->v.v.fun->gc.type)) {
        xfree(symbol->v.v.fun);
        return 1;
    }
    if (lgx_str_init(&symbol->v.v.fun->name, name.length)) {
        lgx_type_cleanup(&symbol->v.v.fun->gc.type);
        xfree(symbol->v.v.fun);
        return 1;
    }
    lgx_str_dup(&name, &symbol->v.v.fun->name);

    int ret = 0;

    // 为函数调用预留 4 个寄存器
    int i;
    for (i = 0; i < 4; ++i) {
        lgx_reg_pop(node->u.regs);
    }

    symbol->v.v.fun->addr = c->bc.length;

    // 编译语句
    if (compiler_block_statement(c, node->child[4])) {
        ret = 1;
    }

    // TODO 检查函数是否总是有返回值

    // 写入一句 ret null
    bc_ret(c, 0);

    symbol->v.v.fun->end = c->bc.length - 1;
    symbol->v.v.fun->stack_size = node->u.regs->max + 1;

    return ret;
}

static int compiler(lgx_compiler_t* c, lgx_ast_node_t *node) {
    assert(node->type == BLOCK_STATEMENT);

    int ret = 0;

    // 为全局变量分配寄存器
    lgx_ht_node_t* n;
    for (n = lgx_ht_first(node->u.symbols); n; n = lgx_ht_next(n)) {
        lgx_symbol_t *symbol = (lgx_symbol_t *)n->v;
        if (symbol->s_type == S_VARIABLE) {
            // 如果符号类型为变量
            if (c->global.length >= 65535) {
                compiler_error(c, symbol->node, "too many global variables\n");
                ret = 1;
            }
            symbol->r = c->global.length;

            if (lgx_ht_set(&c->global, &n->k, symbol)) {
                ret = 1;
            }
        } else if (symbol->s_type == S_CONSTANT) {
            if (symbol->type.type == T_FUNCTION) {
                symbol->r = lgx_const_get_function(&c->constant, &n->k);
                if (symbol->r < 0) {
                    ret = 1;
                }
            }
        }
    }

    int i;
    for(i = 0; i < node->children; ++i) {
        switch (node->child[i]->type) {
            case FUNCTION_DECLARATION:
                if (compiler_function_declaration(c, node->child[i])) {
                    ret = 1;
                }
                break;
            case VARIABLE_DECLARATION:
                if (compiler_global_variable_declaration(c, node->child[i])) {
                    ret = 1;
                }
                break;
            case CONSTANT_DECLARATION:
                if (compiler_constant_declaration(c, node->child[i])) {
                    ret = 1;
                }
                break;
            case PACKAGE_DECLARATION:
            case IMPORT_DECLARATION:
            case EXPORT_DECLARATION:
                // TODO
                break;
            default:
                compiler_error(c, node->child[i], "invalid ast-node %d\n", node->child[i]->type);
                ret = 1;
                break;
        }
    }

    // 更新常量表中的函数值
    for (n = lgx_ht_first(node->u.symbols); n; n = lgx_ht_next(n)) {
        lgx_symbol_t *symbol = (lgx_symbol_t *)n->v;
        if (symbol->s_type == S_CONSTANT) {
            if (symbol->type.type == T_FUNCTION) {
                lgx_const_update_function(&c->constant, symbol->v.v.fun);
            }
        }
    }

    return ret;
}

int lgx_compiler_init(lgx_compiler_t* c) {
    memset(c, 0, sizeof(lgx_compiler_t));

    c->bc.size = 1024;
    c->bc.buffer = (uint32_t*)xmalloc(c->bc.size * sizeof(uint32_t));
    if (!c->bc.buffer) {
        lgx_compiler_cleanup(c);
        return 1;
    }

    if (lgx_ht_init(&c->constant, 32)) {
        lgx_compiler_cleanup(c);
        return 1;
    }

    if (lgx_ht_init(&c->global, 32)) {
        lgx_compiler_cleanup(c);
        return 1;
    }

    if (lgx_rb_init(&c->exception, LGX_RB_KEY_INTEGER)) {
        lgx_compiler_cleanup(c);
        return 1;
    }

    return 0;
}

int lgx_compiler_generate(lgx_compiler_t* c, lgx_ast_t *ast) {
    c->ast = ast;
    int ret = compiler(c, ast->root);
    c->ast = NULL;

    return ret;
}

void lgx_compiler_cleanup(lgx_compiler_t* c) {
    // 释放字节码缓存
    if(c->bc.buffer) {
        xfree(c->bc.buffer);
    }

    // 释放常量表
    lgx_ht_node_t *n;
    for (n = lgx_ht_first(&c->constant); n; n = lgx_ht_next(n)) {
        lgx_const_del(n->v);
        n->v = NULL;
    }
    lgx_ht_cleanup(&c->constant);

    // 释放全局变量
    for (n = lgx_ht_first(&c->global); n; n = lgx_ht_next(n)) {
        n->v = NULL;
    }
    lgx_ht_cleanup(&c->global);

    // 释放异常信息
    lgx_rb_node_t *node;
    for (node = lgx_rb_first(&c->exception); node; node = lgx_rb_next(node)) {
        lgx_exception_del((lgx_exception_t *)node->value);
    }
    lgx_rb_cleanup(&c->exception);

    // 重新初始化内存，避免野指针
    memset(c, 0, sizeof(lgx_compiler_t));
}