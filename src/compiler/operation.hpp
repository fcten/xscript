#ifndef _XSCRIPT_COMPILER_OPERATION_HPP_
#define _XSCRIPT_COMPILER_OPERATION_HPP_

#include <cstdint>

namespace xscript::compiler {

enum class operation : std::uint8_t {
    // NOP
    // 空指令/无效指令
    NOP,

    // HLT
    // 终止指令
    HLT,

    // BKP
    // 断点调试指令
    BKP,

    // LOAD R C
    // R = C
    LOAD,

    // MOV R1 R2
    // R1 = R2
    MOV,

    // 语法：MOVI R I
    // R(int64_t) = I(int16_t)
    MOVI,

    // --------------- 整数算术运算指令 ---------------

    // ADD R1 R2 R3
    // R1(int64_t) = R2(int64_t) + R3(int64_t)
    ADD,

    // ADDI R1 R2 I
    // R1(int64_t) = R2(int64_t) + I(int8_t)
    ADDI,

    // SUB R1 R2 R3
    // R1(int64_t) = R2(int64_t) - R3(int64_t)
    SUB,

    // SUBI R1 R2 I
    // R1(int64_t) = R2(int64_t) - I(int8_t)
    SUBI,

    // MUL R1 R2 R3
    // R1(int64_t) = R2(int64_t) * R3(int64_t)
    MUL,

    // MULI R1 R2 I
    // R1(int64_t) = R2(int64_t) * I(int8_t)
    MULI,

    // DIV R1 R2 R3
    // R1(int64_t) = R2(int64_t) / R3(int64_t)
    DIV,

    // DIVI R1 R2 I
    // R1(int64_t) = R2(int64_t) / I(int8_t)
    DIVI,

    // NEG R1 R2
    // R1(int64_t) = -R2(int64_t)
    NEG,

    // --------------- 浮点数算术运算指令 ---------------

    // FADD R1 R2 R3
    // R1(double) = R2(double) + R3(double)
    FADD,

    // FSUB R1 R2 R3
    // R1(double) = R2(double) - R3(double)
    FSUB,

    // FMUL R1 R2 R3
    // R1(double) = R2(double) * R3(double)
    FMUL,

    // FDIV R1 R2 R3
    // R1(double) = R2(double) / R3(double)
    FDIV,

    // FNEG R1 R2
    // R1(double) = -R2(double)
    FNEG,

    // --------------- 位运算指令 ---------------

    // SHL R1 R2 R3
    SHL,

    // SHLI R1 R2 I
    SHLI,

    // SHR R1 R2 R3
    SHR,

    // SHRI R1 R2 I
    SHRI,

    // AND R1 R2 R3
    AND,

    // OR R1 R2 R3
    OR,

    // XOR R1 R2 R3
    XOR,

    // NOT R R
    NOT,

    // --------------- 逻辑运算指令 ---------------

    EQ,    // EQ   R R R
    EQI,   // EQI  R R I
    LE,    // LE   R R R
    LEI,   // LEI  R R I
    LT,    // LT   R R R
    LTI,   // LTI  R R I
    GEI,   // GEI  R R I
    GTI,   // GTI  R R I
    LNOT,  // LNOT R R

    // --------------- 类型运算指令 ---------------

    // TYPEOF R R
    TYPEOF,

    // --------------- 跳转指令 ---------------

    // TEST 指令限制了单次控制转移距离上限为 64K
    // 超过限制时应当使用 TEST JMPI 组合指令
    TEST,  // TEST R I
    // JMPI 范围为 0 - 16M，超过范围时可以使用常量表
    JMP,   // JMP  R
    JMPI,  // JMPI L

    // --------------- 函数调用指令 ---------------

    CALL_NEW,  // CALL_NEW F         确保足够的栈空间
    CALL_SET,  // CALL_SET R R       设置参数
    CALL,      // CALL     R F       执行调用，返回值写入到 R 中
    RET,       // RET      R

    TAIL_CALL, // TAIL_CALL F        复用当前的函数栈，执行调用
    CO_CALL,   // CO_CALL   F        创建新的调用栈，执行调用

    // 数组
    ARRAY_NEW, // ARRAY_NEW R        R = []
    ARRAY_GET, // ARRAY_GET R R R    R1 = R2[R3]
    ARRAY_SET, // ARRAY_SET R R R    R1[R2] = R3

    // 全局变量
    // 全局变量数量上限为 64K 个
    GLOBAL_GET,// GLOBAL_GET R G     R = G
    GLOBAL_SET,// GLOBAL_SET R G     G = R

    // 异常处理
    THROW,

    // 字符串拼接
    CONCAT,

    // 打印
    ECHO
};

}

#endif // _XSCRIPT_COMPILER_OPERATION_HPP_