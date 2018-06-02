#ifndef LGX_BYTECODE_H
#define LGX_BYTECODE_H

#include "val.h"
#include "hash.h"
#include "../parser/ast.h"

enum {
    // 空指令
    OP_NOP,   // NOP

    // 读取一个常量
    OP_LOAD,  // LOAD R C

    // 寄存器赋值
    OP_MOV,   // MOV  R R
    OP_MOVI,  // MOVI R I

    // 数学运算
    OP_ADD,   // ADD  R R
    OP_ADDI,  // ADDI R I
    OP_SUB,   // SUB  R R
    OP_SUBI,  // SUBI R I
    OP_MUL,   // MUL  R R
    OP_MULI,  // MULI R I
    OP_DIV,   // DIV  R R
    OP_DIVI,  // DIVI R I
    OP_NEG,   // NEG  R

    // 位运算
    OP_SHL,   // SHL  R R
    OP_SHLI,  // SHLI R I
    OP_SHR,   // SHR  R R
    OP_SHRI,  // SHRI R I
    OP_AND,   // AND  R R
    OP_OR,    // OR   R R
    OP_XOR,   // XOR  R R
    OP_NOT,   // NOT  R

    // 逻辑运算
    OP_EQ,    // EQ   R R R
    OP_LE,    // LE   R R R
    OP_LT,    // LT   R R R
    OP_EQI,   // EQI  R R I
    OP_GEI,   // GEI  R R I
    OP_LEI,   // LE   R R I
    OP_GTI,   // GTI  R R I
    OP_LTI,   // LTI  R R I
    OP_LAND,  // LAND R R R
    OP_LOR,   // LOR  R R R
    OP_LNOT,  // LNOT R

    // 跳转
    OP_TEST,  // TEST R
    OP_JMP,   // JMP  R
    OP_JMPI,  // JMPI L

    // 函数调用
    OP_CALL,  // CALL R
    OP_CALI,  // CALI L
    OP_RET,   // RET

    // 终止执行
    OP_HLT,

    // DEBUG
    OP_ECHO   // ECHO R
};

#define I0(op)          (op)
#define I1(op, e)       (op + (e << 8))
#define I2(op, a, d)    (op + (a << 8) + (d << 16))
#define I3(op, a, b, c) (op + (a << 8) + (b << 16) + (c << 24))

#define OP(i) (   (i)         & 0xFF)
#define PA(i) ( ( (i) >>  8 ) & 0xFF)
#define PB(i) ( ( (i) >> 16 ) & 0xFF)
#define PC(i) (   (i) >> 24         )
#define PD(i) (   (i) >> 16         )
#define PE(i) (   (i) >>  8         )

int lgx_bc_print(unsigned *bc, unsigned bc_size);

#endif // LGX_BYTECODE_H