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
    OP_ANDI,  // ANDI R I
    OP_OR,    // OR   R R
    OP_ORI,   // ORI  R I
    OP_XOR,   // XOR  R R
    OP_XORI,  // XORI R I
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
    OP_SCAL,  // SCAL C

    // 终止执行
    OP_HLT,

    // DEBUG
    OP_ECHO   // ECHO R
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