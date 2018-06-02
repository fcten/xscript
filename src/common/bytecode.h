#ifndef LGX_BYTECODE_H
#define LGX_BYTECODE_H

#include "val.h"
#include "hash.h"
#include "../parser/ast.h"

// C: 16位 常量编号
// R:  8位 寄存器编号
// I: 16位 立即数
// L: 32位 立即数
enum {
    // 空指令
    OP_NOP,   // NOP

    // 读取一个常量
    OP_LOAD,  // LOAD R C

    // 寄存器赋值
    OP_MOV,   // MOV  R R
    OP_MOVI,  // MOVI R I

    // 压栈
    OP_PUSH,  // PUSH R
    // 出栈
    OP_POP,   // POP  R

    // 自增 & 自减
    OP_INC,   // INC  R
    OP_DEC,   // DEC  R

    // 数学运算
    OP_ADD,   // ADD  R R R
    OP_ADDI,  // ADD  R R I
    OP_SUB,   // SUB  R R R
    OP_SUBI,  // SUB  R R I
    OP_MUL,   // MUL  R R R
    OP_MULI,  // MUL  R R I
    OP_DIV,   // DIV  R R R
    OP_DIVI,  // DIV  R R I
    OP_NEG,   // NEG  R

    // 位运算
    OP_SHL,   // SHL  R R R
    OP_SHLI,  // SHLI R R I
    OP_SHR,   // SHR  R R R
    OP_SHRI,  // SHRI R R I
    OP_AND,   // AND  R R R
    OP_ANDI,  // ANDI R R I
    OP_OR,    // OR   R R R
    OP_ORI,   // ORI  R R I
    OP_XOR,   // XOR  R R R
    OP_XORI,  // XORI R R I
    OP_NOT,   // NOT  R

    // 逻辑运算
    OP_CMP,   // CMP  R R R
    OP_GE,    // GE   R R R
    OP_LE,    // LE   R R R
    OP_GT,    // GT   R R R
    OP_LT,    // LT   R R R
    OP_LAND,  // LAND R R R
    OP_LOR,   // LOR  R R R
    OP_LNOT,  // LNOT R

    // 跳转
    OP_JC,    // JC   R
    OP_JMP,   // JMP  L

    // 函数调用
    OP_CALL,  // CALL L
    OP_RET,   // RET
    OP_SCAL   // SCAL C
};

typedef struct {
    // 待编译的抽象语法树
    lgx_ast_t* ast;

    // 字节码缓存
    unsigned *bc;
    unsigned bc_size;
    unsigned bc_offset;
    
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