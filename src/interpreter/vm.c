#include <stdio.h>
#include <stdlib.h>

#include "../common/bytecode.h"
#include "vm.h"

int lgx_vm_init(lgx_vm_t *vm, unsigned *bc, unsigned bc_size) {
    vm->stack_size = 1024;
    vm->stack = malloc(vm->stack_size * sizeof(lgx_val_t*));
    if (!vm->stack) {
        vm->stack_size = 0;
        return 1;
    }

    vm->stack_top = 0;
    
    vm->bc = bc;
    vm->bc_size = bc_size;

    return 0;
}

void lgx_vardump(lgx_val_t* v) {
    switch (v->type) {
        case T_UNDEFINED:
            printf("undefined\n");
            break;
        case T_UNINITIALIZED:
            printf("uninitialized\n");
            break;
        case T_LONG:
            printf("%lld\n", v->v.l);
            break;
        case T_DOUBLE:
            printf("%f\n", v->v.d);
            break;
        case T_STRING:
            printf("\"string\"\n");
            break;
        case T_ARRAY:
            printf("[array]\n");
            break;
        case T_OBJECT:
            printf("{object}\n");
            break;
        case T_RESOURCE:
            printf("<resource>\n");
            break;
        case T_REDERENCE:
            lgx_vardump(v->v.ref);
            break;
        case T_FUNCTION:
            printf("function\n");
            break;
        default:
            printf("error\n");
    }
}

#define R(r)  (vm->stack[vm->stack_top + r])
#define C(r)  (vm->constant[r])

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
                if (R(PA(i)).type == T_LONG && R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l += R(PB(i)).v.l;
                } else if (R(PA(i)).type == T_DOUBLE && R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.d += R(PB(i)).v.d;
                } else {
                    // 类型转换
                }

                break;
            }
            case OP_SUB:{
                if (R(PA(i)).type == T_LONG && R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l -= R(PB(i)).v.l;
                } else if (R(PA(i)).type == T_DOUBLE && R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.d -= R(PB(i)).v.d;
                } else {
                    // 类型转换
                }

                break;
            }
            case OP_MUL:{
                if (R(PA(i)).type == T_LONG && R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l *= R(PB(i)).v.l;
                } else if (R(PA(i)).type == T_DOUBLE && R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.d *= R(PB(i)).v.d;
                } else {
                    // 类型转换
                }

                break;
            }
            case OP_DIV:{
                if (R(PA(i)).type == T_LONG && R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l /= R(PB(i)).v.l;
                } else if (R(PA(i)).type == T_DOUBLE && R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.d /= R(PB(i)).v.d;
                } else {
                    // 类型转换
                }

                break;
            }
            case OP_ADDI:{
                if (R(PA(i)).type == T_LONG) {
                    R(PA(i)).v.l += PD(i);
                } else if (R(PA(i)).type == T_DOUBLE) {
                    R(PA(i)).v.d += PD(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_SUBI:{
                if (R(PA(i)).type == T_LONG) {
                    R(PA(i)).v.l -= PD(i);
                } else if (R(PA(i)).type == T_DOUBLE) {
                    R(PA(i)).v.d -= PD(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_MULI:{
                if (R(PA(i)).type == T_LONG) {
                    R(PA(i)).v.l *= PD(i);
                } else if (R(PA(i)).type == T_DOUBLE) {
                    R(PA(i)).v.d *= PD(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_DIVI:{
                if (R(PA(i)).type == T_LONG) {
                    R(PA(i)).v.l /= PD(i);
                } else if (R(PA(i)).type == T_DOUBLE) {
                    R(PA(i)).v.d /= PD(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_NEG:{
                if (R(PA(i)).type == T_LONG) {
                    R(PA(i)).v.l = -R(PA(i)).v.l;
                } else if (R(PA(i)).type == T_DOUBLE) {
                    R(PA(i)).v.d = -R(PA(i)).v.d;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_SHL:{
                if (R(PA(i)).type == T_LONG && R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l <<= R(PB(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_SHR:{
                if (R(PA(i)).type == T_LONG && R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l >>= R(PB(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_SHLI:{
                if (R(PA(i)).type == T_LONG) {
                    R(PA(i)).v.l <<= PD(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_SHRI:{
                if (R(PA(i)).type == T_LONG) {
                    R(PA(i)).v.l >>= PD(i);
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_AND:{
                if (R(PA(i)).type == T_LONG && R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l &= R(PB(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_OR:{
                if (R(PA(i)).type == T_LONG && R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l |= R(PB(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_XOR:{
                if (R(PA(i)).type == T_LONG && R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l ^= R(PB(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_NOT:{
                if (R(PA(i)).type == T_LONG) {
                    R(PA(i)).v.l = ~R(PA(i)).v.l;
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
                if (R(PA(i)).type == T_BOOL) {
                    R(PA(i)).v.l = !R(PA(i)).v.l;
                } else {
                    // 类型转换
                }
                break;
            }
            case OP_TEST:{
                if (R(PA(i)).v.l) {
                    vm->pc ++;
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
                switch (R(PA(i)).type) {
                    case T_LONG:
                        printf("[R:%d] [INT] %lld\n", PA(i), R(PA(i)).v.l);
                        break;
                    case T_DOUBLE:
                        printf("[R:%d] [FLOAT] %f\n", PA(i), R(PA(i)).v.d);
                        break;
                }
                break;
            }
            default:
                printf("unknown op %d @ %d\n", OP(i), vm->pc);
                return 1;
        }
    }
}