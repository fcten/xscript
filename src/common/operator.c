#include "../tokenizer/tokens.h"
#include "operator.h"
#include "cast.h"

int lgx_op_add(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right) {
    if (left->type == T_LONG && right->type == T_LONG) {
        ret->type = T_LONG;
        ret->v.l = left->v.l + right->v.l;
    } else {
        lgx_val_t l,r;
        if (lgx_cast_double(&l, left) || lgx_cast_double(&r, right)) {
            return 1;
        }
        ret->type = T_DOUBLE;
        ret->v.d = l.v.d + r.v.d;
    }

    return 0;
}

int lgx_op_sub(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right) {
    if (left->type == T_LONG && right->type == T_LONG) {
        ret->type = T_LONG;
        ret->v.l = left->v.l - right->v.l;
    } else {
        lgx_val_t l,r;
        if (lgx_cast_double(&l, left) || lgx_cast_double(&r, right)) {
            return 1;
        }
        ret->type = T_DOUBLE;
        ret->v.d = l.v.d - r.v.d;
    }

    return 0;
}

int lgx_op_mul(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right) {
    if (left->type == T_LONG && right->type == T_LONG) {
        ret->type = T_LONG;
        ret->v.l = left->v.l * right->v.l;
    } else {
        lgx_val_t l,r;
        if (lgx_cast_double(&l, left) || lgx_cast_double(&r, right)) {
            return 1;
        }
        ret->type = T_DOUBLE;
        ret->v.d = l.v.d * r.v.d;
    }

    return 0;
}

int lgx_op_div(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right) {
    // TODO 判断除数为 0 的情况
    if (left->type == T_LONG && right->type == T_LONG) {
        ret->type = T_LONG;
        ret->v.l = left->v.l / right->v.l;
    } else {
        lgx_val_t l,r;
        if (lgx_cast_double(&l, left) || lgx_cast_double(&r, right)) {
            return 1;
        }
        ret->type = T_DOUBLE;
        ret->v.d = l.v.d / r.v.d;
    }

    return 0;
}

int lgx_op_mod(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right) {
    if (left->type == T_LONG && right->type == T_LONG) {
        ret->type = T_LONG;
        ret->v.l = left->v.l % right->v.l;
    } else {
        // error
        return 1;
    }

    return 0;
}

int lgx_op_shl(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right) {
    if (left->type == T_LONG && right->type == T_LONG) {
        ret->type = T_LONG;
        ret->v.l = left->v.l << right->v.l;
    } else {
        // error
        return 1;
    }

    return 0;
}

int lgx_op_shr(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right) {
    if (left->type == T_LONG && right->type == T_LONG) {
        ret->type = T_LONG;
        ret->v.l = left->v.l >> right->v.l;
    } else {
        // error
        return 1;
    }

    return 0;
}

int lgx_op_gt(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right) {
    if (left->type == T_LONG && right->type == T_LONG) {
        ret->type = T_BOOL;
        ret->v.l = left->v.l > right->v.l;
    } else if (left->type == T_LONG && right->type == T_DOUBLE) {
        ret->type = T_BOOL;
        ret->v.l = left->v.l > right->v.d;
    } else if (left->type == T_DOUBLE && right->type == T_LONG) {
        ret->type = T_BOOL;
        ret->v.l = left->v.d > right->v.l;
    } else if (left->type == T_DOUBLE && right->type == T_DOUBLE) {
        ret->type = T_BOOL;
        ret->v.l = left->v.d > right->v.d;
    } else {
        // error
        return 1;
    }

    return 0;
}

int lgx_op_lt(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right) {
    if (left->type == T_LONG && right->type == T_LONG) {
        ret->type = T_BOOL;
        ret->v.l = left->v.l < right->v.l;
    } else if (left->type == T_LONG && right->type == T_DOUBLE) {
        ret->type = T_BOOL;
        ret->v.l = left->v.l < right->v.d;
    } else if (left->type == T_DOUBLE && right->type == T_LONG) {
        ret->type = T_BOOL;
        ret->v.l = left->v.d < right->v.l;
    } else if (left->type == T_DOUBLE && right->type == T_DOUBLE) {
        ret->type = T_BOOL;
        ret->v.l = left->v.d < right->v.d;
    } else {
        // error
        return 1;
    }

    return 0;
}

int lgx_op_ge(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right) {
    if (left->type == T_LONG && right->type == T_LONG) {
        ret->type = T_BOOL;
        ret->v.l = left->v.l >= right->v.l;
    } else if (left->type == T_LONG && right->type == T_DOUBLE) {
        ret->type = T_BOOL;
        ret->v.l = left->v.l >= right->v.d;
    } else if (left->type == T_DOUBLE && right->type == T_LONG) {
        ret->type = T_BOOL;
        ret->v.l = left->v.d >= right->v.l;
    } else if (left->type == T_DOUBLE && right->type == T_DOUBLE) {
        ret->type = T_BOOL;
        ret->v.l = left->v.d >= right->v.d;
    } else {
        // error
        return 1;
    }

    return 0;
}

int lgx_op_le(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right) {
    if (left->type == T_LONG && right->type == T_LONG) {
        ret->type = T_BOOL;
        ret->v.l = left->v.l <= right->v.l;
    } else if (left->type == T_LONG && right->type == T_DOUBLE) {
        ret->type = T_BOOL;
        ret->v.l = left->v.l <= right->v.d;
    } else if (left->type == T_DOUBLE && right->type == T_LONG) {
        ret->type = T_BOOL;
        ret->v.l = left->v.d <= right->v.l;
    } else if (left->type == T_DOUBLE && right->type == T_DOUBLE) {
        ret->type = T_BOOL;
        ret->v.l = left->v.d <= right->v.d;
    } else {
        // error
        return 1;
    }

    return 0;
}

int lgx_op_eq(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right) {
    if ( (left->type == right->type) && (left->v.l == right->v.l) ) {
        ret->type = T_BOOL;
        ret->v.l = 1;
    } else {
        ret->type = T_BOOL;
        ret->v.l = 0;
    }

    return 0;
}

int lgx_op_ne(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right) {
    if ( (left->type == right->type) && (left->v.l == right->v.l) ) {
        ret->type = T_BOOL;
        ret->v.l = 0;
    } else {
        ret->type = T_BOOL;
        ret->v.l = 1;
    }

    return 0;
}

int lgx_op_and(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right) {
    if (left->type == T_LONG && right->type == T_LONG) {
        ret->type = T_LONG;
        ret->v.l = left->v.l & right->v.l;
    } else {
        // error
        return 1;
    }

    return 0;
}

int lgx_op_or(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right) {
    if (left->type == T_LONG && right->type == T_LONG) {
        ret->type = T_LONG;
        ret->v.l = left->v.l | right->v.l;
    } else {
        // error
        return 1;
    }

    return 0;
}

int lgx_op_xor(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right) {
    if (left->type == T_LONG && right->type == T_LONG) {
        ret->type = T_LONG;
        ret->v.l = left->v.l ^ right->v.l;
    } else {
        // error
        return 1;
    }

    return 0;
}

int lgx_op_land(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right) {
    if (left->type == T_BOOL && right->type == T_BOOL) {
        ret->type = T_BOOL;
        ret->v.l = left->v.l && right->v.l;
    } else {
        // error
        return 1;
    }

    return 0;
}

int lgx_op_lor(lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right) {
    if (left->type == T_BOOL && right->type == T_BOOL) {
        ret->type = T_BOOL;
        ret->v.l = left->v.l || right->v.l;
    } else {
        // error
        return 1;
    }

    return 0;
}

int lgx_op_lnot(lgx_val_t *ret, lgx_val_t *right) {
    if (right->type == T_BOOL) {
        ret->type = T_BOOL;
        ret->v.l = !right->v.l;
    } else {
        // error
        return 1;
    }

    return 0;
}

int lgx_op_not(lgx_val_t *ret, lgx_val_t *right) {
    if (right->type == T_LONG) {
        ret->type = T_LONG;
        ret->v.l = ~right->v.l;
    } else {
        // error
        return 1;
    }

    return 0;
}

int lgx_op_neg(lgx_val_t *ret, lgx_val_t *right) {
    if (right->type == T_LONG) {
        ret->type = T_LONG;
        ret->v.l = -right->v.l;
    } else if (right->type == T_DOUBLE) {
        ret->type = T_DOUBLE;
        ret->v.d = -right->v.d;
    } else {
        // error
        return 1;
    }

    return 0;
}

int lgx_op_binary(int op, lgx_val_t *ret, lgx_val_t *left, lgx_val_t *right) {
    switch (op) {
        case '+': return lgx_op_add(ret, left, right);
        case '-': return lgx_op_sub(ret, left, right);
        case '*': return lgx_op_mul(ret, left, right);
        case '/': return lgx_op_div(ret, left, right);
        case '%': return lgx_op_mod(ret, left, right);
        case TK_SHL: return lgx_op_shl(ret, left, right);
        case TK_SHR: return lgx_op_shr(ret, left, right);
        case '>': return lgx_op_gt(ret, left, right);
        case '<': return lgx_op_lt(ret, left, right);
        case TK_GE: return lgx_op_ge(ret, left, right);
        case TK_LE: return lgx_op_le(ret, left, right);
        case TK_EQ: return lgx_op_eq(ret, left, right);
        case TK_NE: return lgx_op_ne(ret, left, right);
        case '&': return lgx_op_and(ret, left, right);
        case '^': return lgx_op_xor(ret, left, right);
        case '|': return lgx_op_or(ret, left, right);
        case TK_AND: return lgx_op_land(ret, left, right);
        case TK_OR: return lgx_op_lor(ret, left, right);
        case TK_INDEX:
        case TK_ATTR:
        default:
            // error
            return 1;
    }
}

int lgx_op_unary(int op, lgx_val_t *ret, lgx_val_t *right) {
    switch (op) {
        case '!': return lgx_op_lnot(ret, right);
        case '~': return lgx_op_not(ret, right);
        case '-': return lgx_op_neg(ret, right);
        default:
            // error
            return 1;
    }
}