#ifndef LGX_EXCEPTION_H
#define LGX_EXCEPTION_H

#include "vm.h"

typedef struct {
    lgx_list_t head;
    // block position
    unsigned start;
    unsigned end;
} lgx_exception_block_t;

typedef struct {
    // try block position
    lgx_exception_block_t try_block;
    // catch block list
    lgx_list_t catch_blocks;
    // finally block
    lgx_exception_block_t finally_block;
} lgx_exception_t;

void lgx_exception_throw(lgx_vm_t *vm, lgx_val_t *e);

#endif // LGX_EXCEPTION_H