#include <stdio.h>
#include <stdlib.h>

#include "../common/bytecode.h"
#include "vm.h"

void vm_push(lgx_vm_t *vm, lgx_val_t *v) {
    vm->stack[vm->stack_top++] = *v;
}

lgx_val_t* vm_pop(lgx_vm_t *vm) {
    return &vm->stack[vm->stack_top--];
}

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

static inline unsigned read32(lgx_vm_t *vm) {
    unsigned ret = vm->bc[vm->pc++];
    ret = vm->bc[vm->pc++] + (ret << 8);
    ret = vm->bc[vm->pc++] + (ret << 8);
    ret = vm->bc[vm->pc++] + (ret << 8);
    return ret;
}

static inline unsigned short read16(lgx_vm_t *vm) {
    unsigned short ret = vm->bc[vm->pc++];
    ret = vm->bc[vm->pc++] + (ret << 8);
    return ret;
}

#define read8(vm) vm->bc[vm->pc++]

#define R(r) vm->stack[vm->stack_top + r]

// LOAD R C
static inline void op_load(lgx_vm_t *vm) {
    unsigned char  r = read8(vm);
    unsigned short c = read16(vm);

}

// MOV  R R
static inline void op_mov(lgx_vm_t *vm) {
    unsigned char  r1 = read8(vm);
    unsigned char  r2 = read8(vm);

    vm->stack[vm->stack_top + r1].type = vm->stack[vm->stack_top + r2].type;
    vm->stack[vm->stack_top + r1].v.l = vm->stack[vm->stack_top + r2].v.l;
}

// PUSH R

// POP  R

// INC  R

// DEC  R

// ADDI R R I
static inline void op_addi(lgx_vm_t *vm) {
    unsigned char r1 = read8(vm);
    unsigned char r2 = read8(vm);
    short i = (short)read16(vm);

    switch (vm->stack[vm->stack_top + r2].type) {
        case T_LONG:
            vm->stack[vm->stack_top + r1].v.l = vm->stack[vm->stack_top + r2].v.l + i;
            break;
        case T_DOUBLE:
            vm->stack[vm->stack_top + r1].v.d = vm->stack[vm->stack_top + r2].v.d + i;
            break;
        default:
            // error
            return;
    }
    vm->stack[vm->stack_top + r1].type = vm->stack[vm->stack_top + r2].type;
}

// SUB  R R R

// MUL  R R R

// DIV  R R R
// DIVI R R I
// NEG  R

// SHL  R R R
// SHLI R R I
// SHR  R R R
// SHRI R R I
// AND  R R R
// ANDI R R I
// OR   R R R
// ORI  R R I
// XOR  R R R
// XORI R R I
// NOT  R

// EQ   R R R
static inline void op_eq(lgx_vm_t *vm) {
    unsigned char r1 = read8(vm);
    unsigned char r2 = read8(vm);
    unsigned char r3 = read8(vm);

    vm->stack[vm->stack_top + r1].type = T_BOOL;
    vm->stack[vm->stack_top + r1].v.l = 0;

    if (vm->stack[vm->stack_top + r2].type == vm->stack[vm->stack_top + r3].type &&
        vm->stack[vm->stack_top + r2].v.l == vm->stack[vm->stack_top + r3].v.l) {
        vm->stack[vm->stack_top + r1].v.l = 1;
    }
}

// LE   R R R
static inline void op_le(lgx_vm_t *vm) {
    unsigned char r1 = read8(vm);
    unsigned char r2 = read8(vm);
    unsigned char r3 = read8(vm);

    vm->stack[vm->stack_top + r1].type = T_BOOL;
    vm->stack[vm->stack_top + r1].v.l = 0;

    if (vm->stack[vm->stack_top + r2].type == vm->stack[vm->stack_top + r3].type) {
        switch (vm->stack[vm->stack_top + r2].type) {
            case T_LONG:
                if (vm->stack[vm->stack_top + r2].v.l <= vm->stack[vm->stack_top + r3].v.l) {
                    vm->stack[vm->stack_top + r1].v.l = 1;
                }
                break;
            case T_DOUBLE:
                if (vm->stack[vm->stack_top + r2].v.d <= vm->stack[vm->stack_top + r3].v.d) {
                    vm->stack[vm->stack_top + r1].v.l = 1;
                }
                break;
        }
    }
}

// LT   R R R


// EQI  R R I
// GEI  R R I
// LE   R R I

// LTI  R R I

// LAND R R R
// LOR  R R R
// LNOT R


// JMP  R


// CALL R
// CALI L
// RET
// SCAL C

#define OP(i) ((i) & 0xFF)
#define PA(i) (((i)>>8) & 0xFF)
#define PB(i) (((i)>>16) & 0xFF)
#define PC(i) ((i)>>24)
#define PD(i) ((i)>>16)
#define PE(i) ((i)>>8)

int lgx_vm_start(lgx_vm_t *vm) {
    unsigned i;

    for(;;) {
        i = vm->bc[vm->pc++];

        switch(OP(i)) {
            case OP_NOP:  break;
            case OP_LOAD: break;
            case OP_MOV:  break;
            case OP_MOVI:{
                R(PA(i)).type = T_LONG;
                R(PA(i)).v.l = PD(i);
                break;
            }
            case OP_PUSH: break;
            case OP_POP:  break;
            case OP_INC:  break;
            case OP_DEC:  break;
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
            case OP_ADDI: op_addi(vm); break;
            case OP_SUB:  op_mov(vm);  break;
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
            case OP_MUL:  break;
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
            case OP_DIV:  break;
            case OP_DIVI:   break;
            case OP_NEG:  break;
            case OP_SHL:  break;
            case OP_SHLI:  break;
            case OP_SHR:  break;
            case OP_SHRI:  break;
            case OP_AND:  break;
            case OP_ANDI:  break;
            case OP_OR:  break;
            case OP_ORI:  break;
            case OP_XOR:  break;
            case OP_XORI:   break;
            case OP_NOT:   break;
            case OP_EQ:     break;
            case OP_LE:    break;
            case OP_LT:     break;
            case OP_EQI:   break;
            case OP_GEI:   break;
            case OP_LEI:   break;
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
            case OP_LTI:    break;
            case OP_LAND:   break;
            case OP_LOR:   break;
            case OP_LNOT: break;
            case OP_TEST:{
                if (R(PA(i)).v.l) {
                    vm->pc ++;
                }
                break;
            }
            case OP_JMP:   break;
            case OP_JMPI:{
                vm->pc = PE(i);
                break;
            }
            case OP_CALL:  break;
            case OP_CALI:   break;
            case OP_RET:   break;
            case OP_SCAL:  break;
            case OP_HLT:  return 0;
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