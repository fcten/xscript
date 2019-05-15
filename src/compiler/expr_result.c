#include "expr_result.h"

int lgx_expr_result_init(lgx_expr_result_t* e) {
    memset(e, 0, sizeof(lgx_expr_result_t));
    return 0;
}

int lgx_expr_result_cleanup(lgx_expr_result_t* e) {
    // 释放寄存器
    if (is_temp(e)) {
        lgx_reg_push(e->regs, e->u.local);
    }

    switch (e->v.type) {
        case T_STRING:
            lgx_str_cleanup(&e->v.v.str);
            break;
        default:
            break;
    }

    return 0;
}