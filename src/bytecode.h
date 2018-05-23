#ifndef LGX_BYTECODE_H
#define LGX_BYTECODE_H

#include "ast.h"

enum {
    OP_MOV,
    OP_PUSH,
    OP_POP,
    OP_CMP,

    OP_TEST, // if a != 0 then pc ++
    OP_JMP,  // pc += c

    OP_CALL,
    OP_RET,


    OP_NOP
};

void lgx_bc_gen(lgx_ast_node_t* node);
void lgx_bc_print();

#endif // LGX_BYTECODE_H