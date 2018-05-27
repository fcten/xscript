#include <stdio.h>
#include <stdlib.h>

#include "bytecode.h"
#include "vm.h"

void vm_push(lgx_vm_t *vm, lgx_val_t *v) {
    vm->stack[vm->stack_top++] = *v;
}

lgx_val_t* vm_pop(lgx_vm_t *vm) {
    return &vm->stack[vm->stack_top--];
}

int lgx_vm_init(lgx_vm_t *vm, lgx_bc_t *bc) {
    vm->stack_size = 1024;
    vm->stack = malloc(vm->stack_size * sizeof(lgx_val_t*));
    if (!vm->stack) {
        vm->stack_size = 0;
        return 1;
    }

    // 为虚拟内部寄存器分配地址
    vm->pc = &vm->stack[0];
    vm->ret = &vm->stack[1];

    vm->stack_top = 2;
    
    vm->bc = bc->bc;
    vm->bc_size = bc->bc_size;

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

int lgx_vm_start(lgx_vm_t *vm) {
    unsigned char op;//, a, b, c;
    while(vm->pc->v.l < vm->bc_size) {
        op = vm->bc[vm->pc->v.l] >> 10;
        //a = (vm->bc[vm->pc->v.l] & 0b0000001111100000) >> 5;
        //b = vm->bc[vm->pc->v.l] & 0b0000000000011111;
        //c = vm->bc[vm->pc->v.l] & 0b0000001111111111;

        switch(op) {
            case OP_MOV:

                break;
            case OP_LOAD:

                break;
            case OP_PUSH:

                break;
            case OP_POP:

                break;
            case OP_CMP:

                break;
            case OP_ADD:

                break;
            case OP_TEST:

                break;
            case OP_JMP:

                break;
            case OP_CALL:

                break;
            case OP_RET:

                break;
            case OP_NOP:

                break;
            default:
                // error
                break;
        }

        vm->pc->v.l ++;
    }
    
    return 0;
}