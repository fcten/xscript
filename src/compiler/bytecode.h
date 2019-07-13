#ifndef LGX_BYTECODE_H
#define LGX_BYTECODE_H

// 字节码生成
#include "expr_result.h"
#include "compiler.h"

typedef enum {
    // 空指令
    OP_NOP,   // NOP

    // 读取一个常量
    // 常量数量上限为 64K 个
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
    OP_EQI,   // EQI  R R I
    OP_LE,    // LE   R R R
    OP_LEI,   // LEI  R R I
    OP_LT,    // LT   R R R
    OP_LTI,   // LTI  R R I
    OP_GEI,   // GEI  R R I
    OP_GTI,   // GTI  R R I
    OP_LNOT,  // LNOT R R

    // 类型运算
    OP_TYPEOF, // TYPEOF R R

    // 跳转
    // TEST 指令限制了单次控制转移距离上限为 64K
    // 超过限制时应当使用 TEST JMPI 组合指令
    OP_TEST,  // TEST R I
    // JMPI 范围为 0 - 16M，超过范围时可以使用常量表
    OP_JMP,   // JMP  R
    OP_JMPI,  // JMPI L

    // 函数调用
    OP_CALL_NEW,  // CALL_NEW F         确保足够的栈空间
    OP_CALL_SET,  // CALL_SET R R       设置参数
    OP_CALL,      // CALL     R F       执行调用，返回值写入到 R 中
    OP_RET,       // RET      R

    OP_TAIL_CALL, // TAIL_CALL F        复用当前的函数栈，执行调用
    OP_CO_CALL,   // CO_CALL   F        创建新的调用栈，执行调用

    // 数组
    OP_ARRAY_NEW, // ARRAY_NEW R        R = []
    OP_ARRAY_GET, // ARRAY_GET R R R    R1 = R2[R3]
    OP_ARRAY_SET, // ARRAY_SET R R R    R1[R2] = R3

    // 全局变量
    // 全局变量数量上限为 64K 个
    OP_GLOBAL_GET,// GLOBAL_GET R G     R = G
    OP_GLOBAL_SET,// GLOBAL_SET R G     G = R

    // 异常处理
    OP_THROW,

    // 字符串拼接
    OP_CONCAT,

    // 打印
    OP_ECHO,

    // 终止执行
    OP_HLT
} lgx_op_t;

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
void lgx_bc_echo(unsigned n, unsigned i);

void bc_set(lgx_compiler_t* c, unsigned pos, unsigned i);
void bc_set_param_a(lgx_compiler_t* c, unsigned pos, unsigned pa);
void bc_set_param_d(lgx_compiler_t* c, unsigned pos, unsigned pd);
void bc_set_param_e(lgx_compiler_t* c, unsigned pos, unsigned pe);

int bc_nop(lgx_compiler_t* c);

int bc_load(lgx_compiler_t* c, unsigned char reg, unsigned constant);
int bc_mov(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2);
int bc_movi(lgx_compiler_t* c, unsigned char reg, unsigned num);

int bc_add(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3);
int bc_addi(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num);
int bc_sub(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3);
int bc_subi(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num);
int bc_mul(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3);
int bc_muli(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num);
int bc_div(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3);
int bc_divi(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num);
int bc_neg(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2);

int bc_shl(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3);
int bc_shli(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num);
int bc_shr(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3);
int bc_shri(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num);
int bc_and(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3);
int bc_or(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3);
int bc_xor(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3);
int bc_not(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2);

int bc_eq(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3);
int bc_eqi(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num);
int bc_ne(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3);
int bc_nei(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num);
int bc_lt(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3);
int bc_lti(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num);
int bc_le(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3);
int bc_lei(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num);
int bc_gt(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3);
int bc_gti(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num);
int bc_ge(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3);
int bc_gei(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num);
int bc_lnot(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2);

int bc_call_new(lgx_compiler_t* c, unsigned reg);
int bc_call_set(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2);
int bc_call(lgx_compiler_t* c, unsigned reg1, unsigned char reg2);
int bc_ret(lgx_compiler_t* c, unsigned char reg);

int bc_tail_call(lgx_compiler_t* c, unsigned reg);
int bc_co_call(lgx_compiler_t* c, unsigned reg);

int bc_test(lgx_compiler_t* c, unsigned char reg, unsigned distance);
int bc_jmp(lgx_compiler_t* c, unsigned char reg);
int bc_jmpi(lgx_compiler_t* c, unsigned pos);

int bc_hlt(lgx_compiler_t* c);

int bc_array_new(lgx_compiler_t* c, unsigned char reg);
int bc_array_add(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2);
int bc_array_get(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3);
int bc_array_set(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3);

int bc_typeof(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2);

int bc_global_get(lgx_compiler_t* c, unsigned char reg, unsigned global);
int bc_global_set(lgx_compiler_t* c, unsigned global, unsigned char reg);

int bc_throw(lgx_compiler_t* c, unsigned char reg);

int bc_concat(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3);

int bc_echo(lgx_compiler_t* c, unsigned char reg);

#endif // LGX_BYTECODE_H