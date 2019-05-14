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

    return 0;
}