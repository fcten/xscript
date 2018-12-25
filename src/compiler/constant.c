#include "../common/common.h"
#include "constant.h"

// TODO 这里使用了一种非常低效的方式

// 返回该常量的编号，不存在返回 -1
int const_get(lgx_bc_t *bc, lgx_val_t *v) {
    lgx_hash_node_t *n = lgx_hash_find(bc->constant, v);
    if (!n) {
        lgx_hash_add(bc->constant, v);
        n = lgx_hash_find(bc->constant, v);
        // 标记为常量
        n->v.u.symbol.type = S_CONSTANT;
    } else {
        // 如果已存在，则释放 v
        if (IS_GC_VALUE(v)) {
            if (v->v.gc->ref_cnt == 0) {
                lgx_val_free(v);
            }
        }
    }

    return n->k.v.l;
}