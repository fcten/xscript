#ifndef LGX_BYTECODE_H
#define LGX_BYTECODE_H

#include "val.h"
#include "hash.h"
#include "../parser/ast.h"
#include "scope.h"

enum {
    OP_MOV,
    OP_LOAD,
    OP_STORE,

    OP_PUSH,
    OP_POP,

    OP_CMP,

    OP_INC, // a = a + 1
    OP_DEC, // a = a - 1

    OP_ADD, // a = a + b
    OP_SUB, // a = a - b
    OP_MUL, // a = a * b
    OP_DIV, // a = a / b

    OP_SHL, // a = a << b 逻辑/算术左移
    OP_SHR, // a = a >> b 算术右移

    OP_NEG, // a = -a

    OP_AND, // a = a & b
    OP_OR,  // a = a | b
    OP_XOR, // a = a ^ b
    OP_NOT, // a = ~a
    OP_TEST, // if a != 0 then pc ++

    OP_LAND, // a = a && b
    OP_LOR,  // a = a || b
    OP_LNOT, // a = !a

    OP_JMP,  // pc += c

    OP_CALL,
    OP_RET,


    OP_NOP
};

typedef struct {
    // 待编译的抽象语法树
    lgx_ast_t* ast;

    // 字节码缓存
    unsigned *bc;
    unsigned bc_size;
    unsigned bc_offset;
    
    // 作用域链
    lgx_scope_t scope_chain;
    
    // 常量表
    lgx_hash_t constants;

    // 栈内存
    lgx_val_t *st;
    unsigned st_size;
    unsigned st_offset;
} lgx_bc_t;

int lgx_bc_init(lgx_bc_t *bc, lgx_ast_t* ast);
int lgx_bc_gen(lgx_bc_t *bc);
int lgx_bc_print();

#endif // LGX_BYTECODE_H