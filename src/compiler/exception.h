#ifndef LGX_EXCEPTION_H
#define LGX_EXCEPTION_H

#include "../common/list.h"
#include "../parser/type.h"

typedef struct {
    lgx_list_t head;
    // block position
    unsigned start;
    unsigned end;
    // catch params
    lgx_value_t e;
} lgx_exception_block_t;

typedef struct {
    // try block position
    lgx_exception_block_t try_block;
    // catch block list
    lgx_list_t catch_blocks;
} lgx_exception_t;

lgx_exception_t* lgx_exception_new();
void lgx_exception_delete(lgx_exception_t *exception);

lgx_exception_block_t* lgx_exception_block_new();
void lgx_exception_block_del(lgx_exception_block_t *block);

#endif // LGX_EXCEPTION_H