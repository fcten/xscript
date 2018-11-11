#include "common.h"
#include "exception.h"

lgx_exception_t* lgx_exception_new() {
    lgx_exception_t *exception = (lgx_exception_t *)xmalloc(sizeof(lgx_exception_t));
    if (!exception) {
        return NULL;
    }

    wbt_list_init(&exception->catch_blocks);

    return exception;
}

void lgx_exception_delete(lgx_exception_t *exception) {
    while (!wbt_list_empty(&exception->catch_blocks)) {
        lgx_exception_block_t *block = wbt_list_first_entry(&exception->catch_blocks, lgx_exception_block_t, head);
        wbt_list_del(&block->head);
        lgx_exception_block_delete(block);
    }

    xfree(exception);
}

lgx_exception_block_t* lgx_exception_block_new() {
    lgx_exception_block_t *block = (lgx_exception_block_t *)xmalloc(sizeof(lgx_exception_block_t));
    if (!block) {
        return NULL;
    }

    wbt_list_init(&block->head);

    block->start = 0;
    block->end = 0;

    return block;
}

void lgx_exception_block_delete(lgx_exception_block_t *block) {
    xfree(block);
}