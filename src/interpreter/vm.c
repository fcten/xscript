#include <stdio.h>
#include <stdlib.h>

#include "../common/bytecode.h"
#include "vm.h"

int lgx_vm_init(lgx_vm_t *vm, lgx_bc_t *bc) {
    vm->stack_size = 1024;
    vm->stack = malloc(vm->stack_size * sizeof(lgx_val_t*));
    if (!vm->stack) {
        return 1;
    }

    vm->regs = vm->stack;

    vm->bc = bc->bc;
    vm->bc_size = bc->bc_size;
    
    vm->constant = &bc->constant;

    vm->pc = 0;

    return 0;
}

#define R(r)  (vm->regs[r])
#define C(r)  (vm->constant->table[r].k)

int lgx_vm_start(lgx_vm_t *vm) {
    unsigned i;

    for(;;) {
        i = vm->bc[vm->pc++];

        switch(OP(i)) {
            case OP_MOV:{
                R(PA(i)).type = R(PB(i)).type;
                R(PA(i)).v.l = R(PB(i)).v.l;
                break;
            }
            case OP_MOVI:{
                R(PA(i)).type = T_LONG;
                R(PA(i)).v.l = PD(i);
                break;
            }
            case OP_ADD:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l + R(PC(i)).v.l;
                } else if (R(PB(i)).type == T_DOUBLE && R(PC(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d + R(PC(i)).v.d;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_SUB:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l - R(PC(i)).v.l;
                } else if (R(PB(i)).type == T_DOUBLE && R(PC(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d - R(PC(i)).v.d;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_MUL:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l * R(PC(i)).v.l;
                } else if (R(PB(i)).type == T_DOUBLE && R(PC(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d * R(PC(i)).v.d;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_DIV:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l / R(PC(i)).v.l;
                } else if (R(PB(i)).type == T_DOUBLE && R(PC(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d / R(PC(i)).v.d;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_ADDI:{
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l + PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d + PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_SUBI:{
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l - PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d - PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_MULI:{
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l * PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d * PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_DIVI:{
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l / PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d / PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_NEG:{
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = -R(PB(i)).v.l;
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.d = -R(PB(i)).v.d;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_SHL:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l << R(PC(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_SHR:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l >> R(PC(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_SHLI:{
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l << PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_SHRI:{
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l >> PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_AND:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l & R(PC(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_OR:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l | R(PC(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_XOR:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l ^ R(PC(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_NOT:{
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = ~R(PB(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_EQ:{
                R(PA(i)).type = T_BOOL;

                if (R(PB(i)).type == R(PC(i)).type && R(PB(i)).v.l == R(PC(i)).v.l) {
                    R(PA(i)).v.l = 1;
                } else {
                    R(PA(i)).v.l = 0;
                }
                break;
            }
            case OP_LE:{
                R(PA(i)).type = T_BOOL;

                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l <= R(PC(i)).v.l;
                } else if (R(PB(i)).type == T_DOUBLE && R(PC(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d <= R(PC(i)).v.d;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_LT:{
                R(PA(i)).type = T_BOOL;

                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l < R(PC(i)).v.l;
                } else if (R(PB(i)).type == T_DOUBLE && R(PC(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d < R(PC(i)).v.d;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_EQI:{
                R(PA(i)).type = T_BOOL;

                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l == PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d == PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_GEI:{
                R(PA(i)).type = T_BOOL;
                
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l >= PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d >= PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_LEI:{
                R(PA(i)).type = T_BOOL;
                
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l <= PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d <= PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_GTI:{
                R(PA(i)).type = T_BOOL;
                
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l > PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d > PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_LTI:{
                R(PA(i)).type = T_BOOL;
                
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l < PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d < PC(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_LAND:{
                R(PA(i)).type = T_BOOL;
                
                if (R(PB(i)).type == T_BOOL && R(PC(i)).type == T_BOOL) {
                    R(PA(i)).v.l = R(PB(i)).v.l && R(PC(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_LOR:{
                R(PA(i)).type = T_BOOL;
                
                if (R(PB(i)).type == T_BOOL && R(PC(i)).type == T_BOOL) {
                    R(PA(i)).v.l = R(PB(i)).v.l || R(PC(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_LNOT:{
                R(PA(i)).type = T_BOOL;

                if (R(PB(i)).type == T_BOOL) {
                    R(PA(i)).v.l = !R(PB(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_TEST:{
                if (!R(PA(i)).v.l) {
                    vm->pc = PD(i);
                }
                break;
            }
            case OP_JMP:{                
                if (R(PA(i)).type == T_LONG) {
                    vm->pc = R(PA(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_JMPI:{
                vm->pc = PE(i);
                break;
            }
            case OP_CALL:{

                break;
            }
            case OP_CALI:{

                break;
            }
            case OP_RET:{

                break;
            }
            case OP_LOAD:{
                R(PA(i)) = C(PD(i));
                break;
            }
            case OP_NOP: break;
            case OP_HLT: return 0;
            case OP_ECHO:{
                lgx_val_print(&R(PA(i)));
                break;
            }
            default:
                printf("unknown op %d @ %d\n", OP(i), vm->pc);
                return 1;
        }
    }
}