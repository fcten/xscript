#include "../common/common.h"
#include "constant.h"

// TODO 这里使用了一种非常低效的方式

// 返回该常量的编号，不存在返回 -1
int const_get(lgx_bc_t *bc, lgx_val_t *v) {
    lgx_hash_node_t *n = lgx_hash_find(bc->constant, v);
    if (!n) {
        lgx_hash_add(bc->constant, v);
        n = lgx_hash_find(bc->constant, v);
    } else {
        // TODO 如果已存在，则释放 v 避免内存泄漏
    }

    return n->k.v.l;
}