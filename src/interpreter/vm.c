#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../common/bytecode.h"
#include "../common/operator.h"
#include "vm.h"

// 输出当前的调用栈
int lgx_vm_backtrace(lgx_vm_t *vm) {
    unsigned int base = vm->stack.base;
    while (1) {
        printf("  <function:%d>\n", vm->stack.buf[base].v.fun->addr);

        if (vm->stack.buf[base+3].type == T_LONG) {
            base = vm->stack.buf[base+3].v.l;
        } else {
            break;
        }
    }

    return 0;
}

// TODO 使用真正的异常处理机制
void throw_exception(lgx_vm_t *vm, const char *fmt, ...) {
    printf("[runtime error: %d] ", vm->pc-1);

    static char buf[128];

    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, 128, fmt, args);
    va_end(args);

    printf("%.*s\n", len, buf);

    lgx_vm_backtrace(vm);

    //vm->pc = vm->bc_size - 1;
    exit(1);
}

int lgx_vm_stack_init(lgx_vm_stack_t *stack, unsigned size) {
    lgx_list_init(&stack->head);
    stack->size = size;
    stack->base = 0;
    stack->buf = malloc(stack->size * sizeof(lgx_val_t));
    if (!stack->buf) {
        return 1;
    }
    return 0;
}

int lgx_vm_init(lgx_vm_t *vm, lgx_bc_t *bc) {
    // 初始化栈内存
    if (lgx_vm_stack_init(&vm->stack, 256)) {
        return 1;
    }
    vm->regs = vm->stack.buf + vm->stack.base;

    // 初始化老年代堆内存
    if (lgx_vm_stack_init(&vm->heap.old, 4096)) {
        return 1;
    }

    // 初始化新生代堆内存
    int i;
    lgx_list_init(&vm->heap.young.head);
    for (i = 0; i < 16; i++) {
        lgx_vm_stack_t *stack = malloc(sizeof(lgx_vm_stack_t));
        if (!stack) {
            return 1;
        }
        if (lgx_vm_stack_init(stack, 512)) {
            return 1;
        }
        lgx_list_add_tail(&stack->head, &vm->heap.young.head);
    }

    vm->bc = bc->bc;
    vm->bc_size = bc->bc_size;
    
    vm->constant = &bc->constant;

    vm->pc = 0;

    // 初始化 main 函数
    vm->regs[0].type = T_FUNCTION;
    vm->regs[0].v.fun = lgx_fun_new();
    vm->regs[0].v.fun->addr = 0;
    vm->regs[0].v.fun->stack_size = 256;

    return 0;
}

#define R(r)  (vm->regs[r])
#define C(r)  (vm->constant->table[r].k)

// 确保堆栈上有足够的剩余空间
int lgx_vm_checkstack(lgx_vm_t *vm, unsigned int stack_size) {
    if (vm->stack.base + R(0).v.fun->stack_size + stack_size < vm->stack.size) {
        return 0;
    }

    unsigned int size = vm->stack.size;
    while (vm->stack.base + R(0).v.fun->stack_size + stack_size >= size) {
        size *= 2;
    }

    lgx_val_t *s = realloc(vm->stack.buf, size * sizeof(lgx_val_t));
    if (!s) {
        return 1;
    }

    vm->stack.buf = s;
    vm->stack.size = size;
    vm->regs = vm->stack.buf + vm->stack.base;

    return 0;
}

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
                    if (lgx_op_add(&R(PA(i)), &R(PB(i)), &R(PC(i)))) {
                        throw_exception(vm, "error operation: %s %s %s\n", lgx_val_typeof(&R(PB(i))), "+", lgx_val_typeof(&R(PC(i))));
                    }
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
                    if (lgx_op_sub(&R(PA(i)), &R(PB(i)), &R(PC(i)))) {
                        throw_exception(vm, "error operation: %s %s %s\n", lgx_val_typeof(&R(PB(i))), "-", lgx_val_typeof(&R(PC(i))));
                    }
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
                    if (lgx_op_mul(&R(PA(i)), &R(PB(i)), &R(PC(i)))) {
                        throw_exception(vm, "error operation: %s %s %s\n", lgx_val_typeof(&R(PB(i))), "*", lgx_val_typeof(&R(PC(i))));
                    }
                }
                break;
            }
            case OP_DIV:{
                // TODO 判断除数是否为 0
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l / R(PC(i)).v.l;
                } else if (R(PB(i)).type == T_DOUBLE && R(PC(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d / R(PC(i)).v.d;
                } else {
                    if (lgx_op_div(&R(PA(i)), &R(PB(i)), &R(PC(i)))) {
                        throw_exception(vm, "error operation: %s %s %s\n", lgx_val_typeof(&R(PB(i))), "/", lgx_val_typeof(&R(PC(i))));
                    }
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
                    lgx_val_t c;
                    c.type = T_LONG;
                    c.v.l = PC(i);
                    if (lgx_op_add(&R(PA(i)), &R(PB(i)), &c)) {
                        throw_exception(vm, "makes number from %s without a cast\n", lgx_val_typeof(&R(PB(i))));
                    }
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
                    lgx_val_t c;
                    c.type = T_LONG;
                    c.v.l = PC(i);
                    if (lgx_op_sub(&R(PA(i)), &R(PB(i)), &c)) {
                        throw_exception(vm, "makes number from %s without a cast\n", lgx_val_typeof(&R(PB(i))));
                    }
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
                    lgx_val_t c;
                    c.type = T_LONG;
                    c.v.l = PC(i);
                    if (lgx_op_mul(&R(PA(i)), &R(PB(i)), &c)) {
                        throw_exception(vm, "makes number from %s without a cast\n", lgx_val_typeof(&R(PB(i))));
                    }
                }
                break;
            }
            case OP_DIVI:{
                // TODO 判断除数是否为 0
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l / PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).type = T_DOUBLE;
                    R(PA(i)).v.d = R(PB(i)).v.d / PC(i);
                } else {
                    lgx_val_t c;
                    c.type = T_LONG;
                    c.v.l = PC(i);
                    if (lgx_op_add(&R(PA(i)), &R(PB(i)), &c)) {
                        throw_exception(vm, "makes number from %s without a cast\n", lgx_val_typeof(&R(PB(i))));
                    }
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
                    throw_exception(vm, "makes number from %s without a cast\n", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_SHL:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l << R(PC(i)).v.l;
                } else {
                    throw_exception(vm, "error operation: %s %s %s\n", lgx_val_typeof(&R(PB(i))), "<<", lgx_val_typeof(&R(PC(i))));
                }
                break;
            }
            case OP_SHR:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l >> R(PC(i)).v.l;
                } else {
                    throw_exception(vm, "error operation: %s %s %s\n", lgx_val_typeof(&R(PB(i))), ">>", lgx_val_typeof(&R(PC(i))));
                }
                break;
            }
            case OP_SHLI:{
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l << PC(i);
                } else {
                    throw_exception(vm, "makes integer from %s without a cast\n", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_SHRI:{
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l >> PC(i);
                } else {
                    throw_exception(vm, "makes integer from %s without a cast\n", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_AND:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l & R(PC(i)).v.l;
                } else {
                    throw_exception(vm, "error operation: %s %s %s\n", lgx_val_typeof(&R(PB(i))), "&", lgx_val_typeof(&R(PC(i))));
                }
                break;
            }
            case OP_OR:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l | R(PC(i)).v.l;
                } else {
                    throw_exception(vm, "error operation: %s %s %s\n", lgx_val_typeof(&R(PB(i))), "|", lgx_val_typeof(&R(PC(i))));
                }
                break;
            }
            case OP_XOR:{
                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l ^ R(PC(i)).v.l;
                } else {
                    throw_exception(vm, "error operation: %s %s %s\n", lgx_val_typeof(&R(PB(i))), "^", lgx_val_typeof(&R(PC(i))));
                }
                break;
            }
            case OP_NOT:{
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = ~R(PB(i)).v.l;
                } else {
                    throw_exception(vm, "error operation: %s %s %s\n", lgx_val_typeof(&R(PB(i))), "~", lgx_val_typeof(&R(PC(i))));
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
            case OP_CALL_NEW:{
                if (R(PA(i)).type == T_FUNCTION) {
                    // 确保空余堆栈空间足够容纳本次函数调用
                    if (lgx_vm_checkstack(vm, R(PA(i)).v.fun->stack_size) != 0) {
                        // runtime error
                        throw_exception(vm, "check stack failed");
                    }
                } else {
                    // runtime error
                    throw_exception(vm, "attempt to call a %s value, function expected", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_CALL_SET:{
                if (R(PA(i)).type == T_FUNCTION) {
                    R(R(0).v.fun->stack_size + PB(i)) = R(PC(i));
                } else {
                    // runtime error
                    throw_exception(vm, "attempt to call a %s value, function expected", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_CALL:{
                if (R(PA(i)).type == T_FUNCTION) {
                    lgx_fun_t *fun = R(PA(i)).v.fun;
                    unsigned int base = R(0).v.fun->stack_size;

                    // 写入函数信息
                    R(base + 0) = R(PA(i));

                    // 初始化返回值
                    R(base + 1).type = T_UNDEFINED;

                    // 写入返回地址
                    R(base + 2).type = T_LONG;
                    R(base + 2).v.l = vm->pc;

                    // 写入堆栈地址
                    R(base + 3).type = T_LONG;
                    R(base + 3).v.l = vm->stack.base;

                    // 切换执行堆栈
                    vm->stack.base += R(0).v.fun->stack_size;
                    vm->regs = vm->stack.buf + vm->stack.base;

                    // 跳转到函数入口
                    vm->pc = fun->addr;
                } else {
                    // runtime error
                    throw_exception(vm, "attempt to call a %s value, function expected", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_CALL_END:{
                if (R(PA(i)).type == T_FUNCTION) {
                    // 读取返回值
                    unsigned int base = R(0).v.fun->stack_size;
                    if (R(base + 1).type == T_LONG) {
                        R(PB(i)) = R(base + R(base + 1).v.l);
                    } else {
                        R(PB(i)).type = T_UNDEFINED;
                    }
                } else {
                    // runtime error
                    throw_exception(vm, "attempt to call a %s value, function expected", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_RET:{
                // 跳转到调用点
                vm->pc = R(2).v.l;

                // 写入返回值
                if (PA(i)) {
                    R(1).type = T_LONG;
                    R(1).v.l = PA(i);
                }

                // 切换执行堆栈
                vm->stack.base = R(3).v.l;
                vm->regs = vm->stack.buf + vm->stack.base;
                break;
            }
            case OP_ARRAY_SET:{
                if (R(PA(i)).type == T_ARRAY) {
                    if (R(PB(i)).type == T_LONG) {
                        lgx_hash_node_t n;
                        n.k = R(PB(i));
                        n.v = R(PC(i));
                        lgx_hash_set(R(PA(i)).v.arr, &n);
                    } else {
                        // runtime warning
                        throw_exception(vm, "attempt to set a %s key, integer expected", lgx_val_typeof(&R(PA(i))));
                    }
                } else {
                    // runtime error
                    throw_exception(vm, "attempt to set a %s value, array expected", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_ARRAY_ADD:{
                if (R(PA(i)).type == T_ARRAY) {
                    lgx_hash_add(R(PA(i)).v.arr, &R(PB(i)));
                } else {
                    // runtime error
                    throw_exception(vm, "attempt to set a %s value, array expected", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_ARRAY_NEW:{
                // TODO GC
                R(PA(i)).type = T_ARRAY;
                R(PA(i)).v.arr = malloc(sizeof(lgx_hash_t));

                lgx_hash_init(R(PA(i)).v.arr, 32);

                break;
            }
            case OP_ARRAY_GET:{
                if (R(PB(i)).type == T_ARRAY) {
                    if (R(PC(i)).type == T_LONG) {
                        if (R(PC(i)).v.l >= 0 && R(PC(i)).v.l < R(PB(i)).v.arr->table_offset) {
                            R(PA(i)) = R(PB(i)).v.arr->table[R(PC(i)).v.l].v;
                        } else {
                            // runtime warning
                            R(PA(i)).type = T_UNDEFINED;
                        }
                    } else {
                        // runtime warning
                        throw_exception(vm, "attempt to index a %s key, integer expected", lgx_val_typeof(&R(PC(i))));
                    }
                } else {
                    throw_exception(vm, "attempt to index a %s value, array expected", lgx_val_typeof(&R(PB(i))));
                }
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
                printf("\n");
                break;
            }
            default:
                printf("unknown op %d @ %d\n", OP(i), vm->pc);
                return 1;
        }
    }
}