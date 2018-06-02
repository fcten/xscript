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

int lgx_vm_init(lgx_vm_t *vm, unsigned char *bc, unsigned bc_size) {
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

static inline unsigned char read8(lgx_vm_t *vm) {
    return vm->bc[vm->pc++];
}

// LOAD R C
static inline void op_load(lgx_vm_t *vm) {
    unsigned char  r = read8(vm);
    unsigned short c = read16(vm);

}

// MOV  R R
static inline void op_mov(lgx_vm_t *vm) {
    unsigned char  r1 = read8(vm);
    unsigned char  r2 = read8(vm);

}

// MOVI R I

// PUSH R

// POP  R

// INC  R

// DEC  R

// ADD  R R R
// ADD  R R I
// SUB  R R R
// SUB  R R I
// MUL  R R R
// MUL  R R I
// DIV  R R R
// DIV  R R I
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

// CMP  R R R
// GE   R R R
// LE   R R R
// GT   R R R
// LT   R R R
// LAND R R R
// LOR  R R R
// LNOT R

// JC   R
// JMP  L

// CALL L
// RET
// SCAL C

int lgx_vm_start(lgx_vm_t *vm) {
    unsigned char op;

    while(vm->pc < vm->bc_size) {
        op = read8(vm);

        switch(op) {
            case OP_NOP:  break;
            case OP_LOAD: op_load(vm); break;
            case OP_MOV:  op_mov(vm);  break;
            case OP_MOVI: op_mov(vm);  break;
            case OP_PUSH: op_mov(vm);  break;
            case OP_POP:  op_mov(vm);  break;
            case OP_INC:  op_mov(vm);  break;
            case OP_DEC:  op_mov(vm);  break;
            case OP_ADD:  op_mov(vm);  break;
            case OP_ADDI: op_mov(vm);  break;
            case OP_SUB:  op_mov(vm);  break;
            case OP_SUBI: op_mov(vm);  break;
            case OP_MUL:  op_mov(vm);  break;
            case OP_MULI: op_mov(vm);  break;
            case OP_DIV:  op_mov(vm);  break;
            case OP_DIVI: op_mov(vm);  break;
            case OP_NEG:  op_mov(vm);  break;
            case OP_SHL:  op_mov(vm);  break;
            case OP_SHLI: op_mov(vm);  break;
            case OP_SHR:  op_mov(vm);  break;
            case OP_SHRI: op_mov(vm);  break;
            case OP_AND:  op_mov(vm);  break;
            case OP_ANDI: op_mov(vm);  break;
            case OP_OR:   op_mov(vm);  break;
            case OP_ORI:  op_mov(vm);  break;
            case OP_XOR:  op_mov(vm);  break;
            case OP_XORI: op_mov(vm);  break;
            case OP_NOT:  op_mov(vm);  break;
            case OP_CMP:  op_mov(vm);  break;
            case OP_GE:   op_mov(vm);  break;
            case OP_LE:   op_mov(vm);  break;
            case OP_GT:   op_mov(vm);  break;
            case OP_LT:   op_mov(vm);  break;
            case OP_LAND: op_mov(vm);  break;
            case OP_LOR:  op_mov(vm);  break;
            case OP_LNOT: op_mov(vm);  break;
            case OP_JC:   op_mov(vm);  break;
            case OP_JMP:  op_mov(vm);  break;
            case OP_CALL: op_mov(vm);  break;
            case OP_RET:  op_mov(vm);  break;
            default:
                printf("unknown op %d @ %d\n", op, vm->pc);
                return 1;
        }
    }
    
    return 0;
}