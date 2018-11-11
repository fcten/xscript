#ifndef LGX_EXCEPTION_H
#define LGX_EXCEPTION_H

#include "../webit/common/wbt_list.h"

typedef struct {
    wbt_list_t head;
    // block position
    unsigned start;
    unsigned end;
} lgx_exception_block_t;

typedef struct {
    // try block position
    lgx_exception_block_t try_block;
    // catch block list
    wbt_list_t catch_blocks;
    // finally block
    lgx_exception_block_t finally_block;
} lgx_exception_t;

lgx_exception_t* lgx_exception_new();
void lgx_exception_delete(lgx_exception_t *exception);

lgx_exception_block_t* lgx_exception_block_new();
void lgx_exception_block_delete(lgx_exception_block_t *block);

#endif // LGX_EXCEPTION_H