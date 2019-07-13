#include "../common/common.h"
#include "exception.h"

lgx_exception_t* lgx_exception_new() {
    lgx_exception_t *exception = (lgx_exception_t *)xmalloc(sizeof(lgx_exception_t));
    if (!exception) {
        return NULL;
    }

    lgx_list_init(&exception->catch_blocks);

    return exception;
}

void lgx_exception_del(lgx_exception_t *exception) {
    while (!lgx_list_empty(&exception->catch_blocks)) {
        lgx_exception_block_t *block = lgx_list_first_entry(&exception->catch_blocks, lgx_exception_block_t, head);
        lgx_list_del(&block->head);
        lgx_exception_block_del(block);
    }

    xfree(exception);
}

lgx_exception_block_t* lgx_exception_block_new() {
    lgx_exception_block_t *block = (lgx_exception_block_t *)xmalloc(sizeof(lgx_exception_block_t));
    if (!block) {
        return NULL;
    }

    lgx_list_init(&block->head);

    block->start = 0;
    block->end = 0;

    return block;
}

void lgx_exception_block_del(lgx_exception_block_t *block) {
    lgx_type_cleanup(&block->e);
    xfree(block);
}
