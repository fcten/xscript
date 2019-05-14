#include <stdio.h>
#include "bytecode.h"

const char* op_name[] = {
    "NOP",
    "LOAD",
    "MOV",
    "MOVI",
    "ADD",
    "ADDI",
    "SUB",
    "SUBI",
    "MUL",
    "MULI",
    "DIV",
    "DIVI",
    "NEG",
    "SHL",
    "SHLI",
    "SHR",
    "SHRI",
    "AND",
    "OR",
    "XOR",
    "NOT",
    "EQ",
    "EQI",
    "LE",
    "LEI",
    "LT",
    "LTI",
    "GEI",
    "GTI",
    "LNOT",
    "TYPEOF",
    "TEST",
    "JMP",
    "JMPI",
    "CALL_NEW",
    "CALL_SET",
    "CALL",
    "RET",
    "TAIL_CALL",
    "ARRAY_NEW",
    "ARRAY_GET",
    "ARRAY_SET",
    "GLOBAL_GET",
    "GLOBAL_SET",
    "THROW",
    "AWAIT",
    "HLT"
};

void lgx_bc_echo(unsigned n, unsigned i) {
    switch(OP(i)) {
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_EQ:
        case OP_LE:
        case OP_LT:
        case OP_SHL:
        case OP_SHR:
        case OP_AND:
        case OP_OR:
        case OP_XOR:
        case OP_ARRAY_GET:
        case OP_ARRAY_SET:
            printf("%4d %11s R[%d] R[%d] R[%d]\n", n, op_name[OP(i)], PA(i), PB(i), PC(i));
            break;
        case OP_ADDI:
        case OP_SUBI:
        case OP_MULI:
        case OP_DIVI:
        case OP_EQI:
        case OP_GEI:
        case OP_LEI:
        case OP_GTI:
        case OP_LTI:
        case OP_SHLI:
        case OP_SHRI:
            printf("%4d %11s R[%d] R[%d] %d\n", n, op_name[OP(i)], PA(i), PB(i), PC(i));
            break;
        case OP_MOV:
        case OP_NEG:
        case OP_NOT:
        case OP_LNOT:
        case OP_CALL_SET:
        case OP_TYPEOF:
        case OP_CALL:
            printf("%4d %11s R[%d] R[%d]\n", n, op_name[OP(i)], PA(i), PB(i));
            break;
        case OP_GLOBAL_GET:
        case OP_GLOBAL_SET:
            printf("%4d %11s R[%d] G[%d]\n", n, op_name[OP(i)], PA(i), PB(i));
            break;
        case OP_LOAD:
            printf("%4d %11s R[%d] C[%d]\n", n, op_name[OP(i)], PA(i), PD(i));
            break;
        case OP_MOVI:
        case OP_TEST:
            printf("%4d %11s R[%d] %d\n", n, op_name[OP(i)], PA(i), PD(i));
            break;
        case OP_JMP:
        case OP_ARRAY_NEW:
        case OP_RET:
        case OP_CALL_NEW:
        case OP_THROW:
        case OP_TAIL_CALL:
            printf("%4d %11s R[%d]\n", n, op_name[OP(i)], PA(i));
            break;
        case OP_JMPI:
            printf("%4d %11s %d\n", n, op_name[OP(i)], PE(i));
            break;
        case OP_NOP:
        case OP_HLT:
            printf("%4d %11s\n", n, op_name[OP(i)]);
            break;
        default:
            printf("%4d %11s\n", n, "UNKNOWN");
    }
}

int lgx_bc_print(unsigned *bc, unsigned bc_size) {
    unsigned i, n = 0;

    while (n < bc_size) {
        i = bc[n];

        lgx_bc_echo(n, i);

        n ++;
    }
    
    return 0;
}

static int bc_append(lgx_compiler_t* c, unsigned i) {
    if (c->bc.length >= c->bc.size) {
        c->bc.size *= 2;
        unsigned* buf = (unsigned *)xrealloc(c->bc.buffer, c->bc.size * sizeof(unsigned));
        if (!buf) {
            return 1;
        }
        c->bc.buffer = buf;
    }

    c->bc.buffer[c->bc.length] = i;
    c->bc.length ++;

    return 0;
}

void bc_set(lgx_compiler_t* c, unsigned pos, unsigned i) {
    c->bc.buffer[pos] = i;
}

void bc_set_param_a(lgx_compiler_t* c, unsigned pos, unsigned pa) {
    c->bc.buffer[pos] &= 0xFFFF00FF;
    c->bc.buffer[pos] |= pa << 8;
}

void bc_set_param_d(lgx_compiler_t* c, unsigned pos, unsigned pd) {
    c->bc.buffer[pos] &= 0x0000FFFF;
    c->bc.buffer[pos] |= pd << 16;
}

void bc_set_param_e(lgx_compiler_t* c, unsigned pos, unsigned pe) {
    c->bc.buffer[pos] &= 0x000000FF;
    c->bc.buffer[pos] |= pe << 8;
}

int bc_nop(lgx_compiler_t* c) {
    return bc_append(c, I0(OP_NOP));
}

// 寄存器 = 常量
int bc_load(lgx_compiler_t* c, unsigned char reg, unsigned constant) {
    return bc_append(c, I2(OP_LOAD, reg, constant));
}

// 寄存器1 = 寄存器2
int bc_mov(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2) {
    return bc_append(c, I2(OP_MOV, reg1, reg2));
}

// 寄存器 = 立即数
int bc_movi(lgx_compiler_t* c, unsigned char reg, unsigned num) {
    assert(num <= 65535);
    return bc_append(c, I2(OP_MOVI, reg, num));
}

// 寄存器1 = 寄存器2 + 寄存器3
int bc_add(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3) {
    return bc_append(c, I3(OP_ADD, reg1, reg2, reg3));
}

// 寄存器1 = 寄存器2 + 立即数
int bc_addi(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num) {
    return bc_append(c, I3(OP_ADDI, reg1, reg2, num));
}

// 寄存器1 = 寄存器2 - 寄存器3
int bc_sub(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3) {
    return bc_append(c, I3(OP_SUB, reg1, reg2, reg3));
}

// 寄存器1 = 寄存器2 - 立即数
int bc_subi(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num) {
    return bc_append(c, I3(OP_SUBI, reg1, reg2, num));
}

// 寄存器1 = 寄存器2 * 寄存器3
int bc_mul(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3) {
    return bc_append(c, I3(OP_MUL, reg1, reg2, reg3));
}

// 寄存器1 = 寄存器2 * 立即数
int bc_muli(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num) {
    return bc_append(c, I3(OP_MULI, reg1, reg2, num));
}

// 寄存器1 = 寄存器2 / 寄存器3
int bc_div(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3) {
    return bc_append(c, I3(OP_DIV, reg1, reg2, reg3));
}

// 寄存器1 = 寄存器2 / 立即数
int bc_divi(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num) {
    return bc_append(c, I3(OP_DIVI, reg1, reg2, num));
}

// 寄存器1 = - 寄存器2
int bc_neg(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2) {
    return bc_append(c, I2(OP_NEG, reg1, reg2));
}

// 寄存器1 = 寄存器2 << 寄存器3
int bc_shl(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3) {
    return bc_append(c, I3(OP_SHL, reg1, reg2, reg3));
}

// 寄存器1 = 寄存器2 << 立即数
int bc_shli(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num) {
    return bc_append(c, I3(OP_SHLI, reg1, reg2, num));
}

// 寄存器1 = 寄存器2 >> 寄存器3
int bc_shr(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3) {
    return bc_append(c, I3(OP_SHR, reg1, reg2, reg3));
}

// 寄存器1 = 寄存器2 >> 立即数
int bc_shri(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num) {
    return bc_append(c, I3(OP_SHRI, reg1, reg2, num));
}

// 寄存器1 = 寄存器2 & 寄存器3
int bc_and(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3) {
    return bc_append(c, I3(OP_AND, reg1, reg2, reg3));
}

// 寄存器1 = 寄存器2 | 寄存器3
int bc_or(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3) {
    return bc_append(c, I3(OP_OR, reg1, reg2, reg3));
}

// 寄存器1 = 寄存器2 ^ 寄存器3
int bc_xor(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3) {
    return bc_append(c, I3(OP_XOR, reg1, reg2, reg3));
}

// 寄存器1 = ~ 寄存器2
int bc_not(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2) {
    return bc_append(c, I2(OP_NOT, reg1, reg2));
}

// 寄存器1 = 寄存器2 == 寄存器3
int bc_eq(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3) {
    return bc_append(c, I3(OP_EQ, reg1, reg2, reg3));
}

// 寄存器1 = 寄存器2 == 立即数
int bc_eqi(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num) {
    return bc_append(c, I3(OP_EQI, reg1, reg2, num));
}

// 寄存器1 = 寄存器2 != 寄存器3
int bc_ne(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3) {
    if (bc_eq(c, reg1, reg2, reg3)) {
        return 1;
    }
    return bc_lnot(c, reg1, reg1);
}

// 寄存器1 = 寄存器2 != 立即数
int bc_nei(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num) {
    if (bc_eqi(c, reg1, reg2, num)) {
        return 1;
    }
    return bc_lnot(c, reg1, reg1);
}

// 寄存器1 = 寄存器2 < 寄存器3
int bc_lt(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3) {
    return bc_append(c, I3(OP_LT, reg1, reg2, reg3));
}

// 寄存器1 = 寄存器2 < 立即数
int bc_lti(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num) {
    return bc_append(c, I3(OP_LTI, reg1, reg2, num));
}

// 寄存器1 = 寄存器2 <= 寄存器3
int bc_le(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3) {
    return bc_append(c, I3(OP_LE, reg1, reg2, reg3));
}

// 寄存器1 = 寄存器2 <= 立即数
int bc_lei(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num) {
    return bc_append(c, I3(OP_LEI, reg1, reg2, num));
}

// 寄存器1 = 寄存器2 > 寄存器3
int bc_gt(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3) {
    return bc_append(c, I3(OP_LT, reg1, reg3, reg2));
}

// 寄存器1 = 寄存器2 > 立即数
int bc_gti(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num) {
    return bc_append(c, I3(OP_GTI, reg1, reg2, num));
}

// 寄存器1 = 寄存器2 >= 寄存器3
int bc_ge(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3) {
    return bc_append(c, I3(OP_LE, reg1, reg3, reg2));
}

// 寄存器1 = 寄存器2 >= 立即数
int bc_gei(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char num) {
    return bc_append(c, I3(OP_GEI, reg1, reg2, num));
}

// 寄存器1 = ! 寄存器2
int bc_lnot(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2) {
    return bc_append(c, I2(OP_LNOT, reg1, reg2));
}

// 创建函数调用，函数信息存储于常量表中
int bc_call_new(lgx_compiler_t* c, unsigned constant) {
    assert(constant <= 65535);
    return bc_append(c, I1(OP_CALL_NEW, constant));
}

// 设置函数参数，其中 reg2 位于当前栈上，reg1 位于被调用函数栈上
int bc_call_set(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2) {
    return bc_append(c, I2(OP_CALL_SET, reg1, reg2));
}

// 执行函数调用，函数信息存储于常量表中，函数返回值写入寄存器 reg 中
int bc_call(lgx_compiler_t* c, unsigned constant, unsigned char reg) {
    assert(constant <= 65535);
    return bc_append(c, I2(OP_CALL, reg, constant));
}

// 把寄存器 reg 的值写入函数返回值中
int bc_ret(lgx_compiler_t* c, unsigned char reg) {
    return bc_append(c, I1(OP_RET, reg));
}

// 复用当前函数栈执行函数调用
int bc_tail_call(lgx_compiler_t* c, unsigned constant) {
    assert(constant <= 65535);
    return bc_append(c, I1(OP_TAIL_CALL, constant));
}

// 如果 寄存器 == TRUE，跳过后续 distance 条指令
int bc_test(lgx_compiler_t* c, unsigned char reg, unsigned distance) {
    return bc_append(c, I2(OP_TEST, reg, distance));
}

// 无条件跳转到寄存器 reg 所指示的位置
int bc_jmp(lgx_compiler_t* c, unsigned char reg) {
    return bc_append(c, I1(OP_JMP, reg));
}

// 无条件跳转到立即数 pos 所指示的位置
int bc_jmpi(lgx_compiler_t* c, unsigned pos) {
    assert(pos <= 16777215);
    return bc_append(c, I1(OP_JMPI, pos));
}

int bc_hlt(lgx_compiler_t* c) {
    return bc_append(c, I0(OP_HLT));
}

// 寄存器 = []
int bc_array_new(lgx_compiler_t* c, unsigned char reg) {
    return bc_append(c, I1(OP_ARRAY_NEW, reg));
}

// 寄存器1[] = 寄存器2
int bc_array_add(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2) {
    return bc_append(c, I3(OP_ARRAY_SET, reg1, 0, reg2));
}

// 寄存器1 = 寄存器2[寄存器3]
int bc_array_get(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3) {
    return bc_append(c, I3(OP_ARRAY_GET, reg1, reg2, reg3));
}

// 寄存器1[寄存器2] = 寄存器3
int bc_array_set(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2, unsigned char reg3) {
    return bc_append(c, I3(OP_ARRAY_SET, reg1, reg2, reg3));
}

// 寄存器1 = typeof 寄存器2
int bc_typeof(lgx_compiler_t* c, unsigned char reg1, unsigned char reg2) {
    return bc_append(c, I2(OP_TYPEOF, reg1, reg2));
}

// 寄存器 = 全局变量
int bc_global_get(lgx_compiler_t* c, unsigned char reg, unsigned global) {
    assert(global <= 65535);
    return bc_append(c, I2(OP_GLOBAL_GET, reg, global));
}

// 全局变量 = 寄存器
int bc_global_set(lgx_compiler_t* c, unsigned global, unsigned char reg) {
    return bc_append(c, I2(OP_GLOBAL_SET, reg, global));
}

int bc_throw(lgx_compiler_t* c, unsigned char reg) {
    return bc_append(c, I1(OP_THROW, reg));
}
