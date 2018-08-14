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
    OP_ADD,   // ADD  R R R
    OP_ADDI,  // ADDI R R I
    OP_SUB,   // SUB  R R R
    OP_SUBI,  // SUBI R R I
    OP_MUL,   // MUL  R R R
    OP_MULI,  // MULI R R I
    OP_DIV,   // DIV  R R R
    OP_DIVI,  // DIVI R R I
    OP_NEG,   // NEG  R R

    // 位运算
    OP_SHL,   // SHL  R R R
    OP_SHLI,  // SHLI R R I
    OP_SHR,   // SHR  R R R
    OP_SHRI,  // SHRI R R I
    OP_AND,   // AND  R R R
    OP_OR,    // OR   R R R
    OP_XOR,   // XOR  R R R
    OP_NOT,   // NOT  R R

    // 逻辑运算
    OP_EQ,    // EQ   R R R
    OP_LE,    // LE   R R R
    OP_LT,    // LT   R R R
    OP_EQI,   // EQI  R R I
    OP_GEI,   // GEI  R R I
    OP_LEI,   // LE   R R I
    OP_GTI,   // GTI  R R I
    OP_LTI,   // LTI  R R I
    OP_LNOT,  // LNOT R R

    // 跳转
    // TEST 指令限制了单次控制转移距离上限为 64K
    // TODO 超过限制时应当返回错误，或者新增 TESTI 指令
    OP_TEST,  // TEST R I
    // TODO JMP 范围为 0 - 16M，超过范围时可以使用常量表
    OP_JMP,   // JMP  R
    OP_JMPI,  // JMPI L

    // 函数调用
    OP_CALL_NEW, // CALL_NEW R
    OP_CALL_SET, // CALL_SET R R R
    OP_CALL,     // CALL     R
    OP_CALL_END, // CALL_END R R
    OP_RET,      // RET      R

    // 数组
    OP_ARRAY_NEW, // ARRAY_NEW R
    OP_ARRAY_ADD, // ARRAY_ADD R R      R1[] = R2
    OP_ARRAY_GET, // ARRAY_GET R R R    R1 = R2[R3]
    OP_ARRAY_SET, // ARRAY_SET R R R    R1[R2] = R3

    // 全局变量
    OP_GLOBAL_GET,// GLOBAL_GET R R
    OP_GLOBAL_SET,// GLOBAL_SET R R

    // 类与对象
    OP_OBJECT_NEW,
    OP_OBJECT_GET,
    OP_OBJECT_SET,

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