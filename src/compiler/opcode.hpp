#ifndef _XSCRIPT_COMPILER_OPCODE_HPP_
#define _XSCRIPT_COMPILER_OPCODE_HPP_

#include <cstdint>
#include <vector>

namespace xscript::compiler {

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
} op_t;

class opcode {
private:
    std::vector<uint32_t> sequence;

public:
};

}

#endif // _XSCRIPT_COMPILER_OPCODE_HPP_