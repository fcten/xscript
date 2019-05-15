#ifndef LGX_EXPRESSION_H
#define LGX_EXPRESSION_H

#include "../parser/type.h"
#include "register.h"

typedef enum {
    // 字面量
    EXPR_LITERAL = 0,
    // 局部变量
    EXPR_LOCAL,
    // 全局变量
    EXPR_GLOBAL,
    // 临时（中间）变量
    EXPR_TEMP,
    // 常量
    EXPR_CONSTANT
} lgx_expr_type_t;

typedef struct {
    lgx_expr_type_t type;

    // 如果结果为字面量，存储字面量具体的值
    // 否则，存储常量或变量的类型
    lgx_value_t v;

    union {
        // 如果结果为常量，存储常量编号
        int constant;
        // 如果结果为局部变量或临时（中间）变量，存储寄存器编号
        int local;
        // 如果结果为全局变量，存储全局变量编号
        int global;
    } u;

    // 如果结果为局部变量，指向对应的寄存器分配器
    lgx_reg_t* regs;
} lgx_expr_result_t;

#define is_literal(e)   ((e)->type == EXPR_LITERAL)
#define is_const(e)     ((e)->type == EXPR_CONSTANT)
#define is_global(e)    ((e)->type == EXPR_GLOBAL)
#define is_local(e)     ((e)->type == EXPR_LOCAL)
#define is_temp(e)      ((e)->type == EXPR_TEMP)
#define is_constant(e)  (is_literal(e) || is_const(e))
#define is_variable(e)  (is_global(e) || is_local(e) || is_temp(e))

#define check_type(e, t)     ((e)->v.type == t)
#define check_constant(e, t) (is_constant(e) && check_type(e, t))
#define check_variable(e, t) (is_variable(e) && check_type(e, t))

int lgx_expr_result_init(lgx_expr_result_t* e);
int lgx_expr_result_cleanup(lgx_expr_result_t* e);

#endif // LGX_EXPRESSION_H