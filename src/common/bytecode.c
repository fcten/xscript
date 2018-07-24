#include <stdio.h>
#include <stdlib.h>

#include "../tokenizer/lex.h"
#include "val.h"
#include "list.h"
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
    "LE",
    "LT",
    "EQI",
    "GEI",
    "LEI",
    "GTI",
    "LTI",
    "LAND",
    "LOR",
    "LNOT",
    "TEST",
    "JMP",
    "JMPI",
    "CALL",
    "RET",
    "ARRAY_NEW",
    "ARRAY_ADD",
    "ARRAY_GET",
    "ARRAY_SET",
    "HLT",
    "ECHO"
};

int lgx_bc_print(unsigned *bc, unsigned bc_size) {
    unsigned i, n = 0;

    while (n < bc_size) {
        i = bc[n];

        switch(OP(i)) {
            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV:
            case OP_EQ:
            case OP_LE:
            case OP_LT:
            case OP_LAND:
            case OP_LOR:
            case OP_SHL:
            case OP_SHR:
            case OP_AND:
            case OP_OR:
            case OP_XOR:
            case OP_ARRAY_GET:
            case OP_ARRAY_SET:
                printf("%4d %4s R[%d] R[%d] R[%d]\n", n, op_name[OP(i)], PA(i), PB(i), PC(i));
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
                printf("%4d %4s R[%d] R[%d] %d\n", n, op_name[OP(i)], PA(i), PB(i), PC(i));
                break;
            case OP_MOV:
            case OP_NEG:
            case OP_NOT:
            case OP_LNOT:
            case OP_ARRAY_ADD:
                printf("%4d %4s R[%d] R[%d]\n", n, op_name[OP(i)], PA(i), PB(i));
                break;
            case OP_LOAD:
                printf("%4d %4s R[%d] C[%d]\n", n, op_name[OP(i)], PA(i), PD(i));
                break;
            case OP_MOVI:
            case OP_TEST:
                printf("%4d %4s R[%d] %d\n", n, op_name[OP(i)], PA(i), PD(i));
                break;
            case OP_JMP:
            case OP_CALL:
            case OP_ECHO:
            case OP_ARRAY_NEW:
                printf("%4d %4s R[%d]\n", n, op_name[OP(i)], PA(i));
                break;
            case OP_JMPI:
                printf("%4d %4s %d\n", n, op_name[OP(i)], PE(i));
                break;
            case OP_RET:
            case OP_NOP:
            case OP_HLT:
                printf("%4d %4s\n", n, op_name[OP(i)]);
                break;
            default:
                printf("%4d UNKNOWN\n", n);
        }
        n ++;
    }
    
    return 0;
}