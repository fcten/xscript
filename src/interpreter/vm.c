#include "../common/common.h"
#include "../common/bytecode.h"
#include "../common/operator.h"
#include "../common/fun.h"
#include "vm.h"
#include "gc.h"

#define R(r)  (vm->regs[r])
#define C(r)  (vm->constant->table[r].v)
#define G(r)  (vm->stack.buf[r])
#define LGX_MAX_STACK_SIZE (256 << 8)

// 输出当前的调用栈
int lgx_vm_backtrace(lgx_vm_t *vm) {
    unsigned int base = vm->stack.base;
    while (1) {
        printf("  <function:%d> %d\n", vm->stack.buf[base].v.fun->addr, base);

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
    stack->buf = xcalloc(stack->size, sizeof(lgx_val_t));
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

    lgx_list_init(&vm->heap.young);
    lgx_list_init(&vm->heap.old);
    vm->heap.young_size = 0;
    vm->heap.old_size = 0;

    vm->bc = bc;
    
    vm->constant = bc->constant;

    vm->pc = 0;

    // 初始化 main 函数
    R(0).type = T_FUNCTION;
    R(0).v.fun = lgx_fun_new(0);
    R(0).v.fun->addr = 0;
    R(0).v.fun->stack_size = bc->reg->max + 1;
    // 写入返回值地址
    R(1).type = T_LONG;
    R(1).v.l = 1;
    // 写入返回地址
    R(2).type = T_LONG;
    R(2).v.l = -1;
    // 写入堆栈地址
    R(3).type = T_LONG;
    R(3).v.l = 0;

    return 0;
}

int lgx_vm_cleanup(lgx_vm_t *vm) {
    // 释放 main 函数
    lgx_fun_delete(vm->regs[0].v.fun);

    // 释放栈
    xfree(vm->stack.buf);

    // 清理所有变量
    lgx_list_t *list = vm->heap.young.next;
    while(list != &vm->heap.young) {
        lgx_list_t *next = list->next;
        xfree(list);
        list = next;
    }
    list = vm->heap.old.next;
    while(list != &vm->heap.old) {
        lgx_list_t *next = list->next;
        xfree(list);
        list = next;
    }

    lgx_bc_cleanup(vm->bc);

    return 0;
}

// 确保堆栈上有足够的剩余空间
int lgx_vm_checkstack(lgx_vm_t *vm, unsigned int stack_size) {
    if (vm->stack.base + R(0).v.fun->stack_size + stack_size < vm->stack.size) {
        return 0;
    }

    unsigned int size = vm->stack.size;
    while (vm->stack.base + R(0).v.fun->stack_size + stack_size >= size) {
        size *= 2;
    }

    if (size > LGX_MAX_STACK_SIZE) {
        return 1;
    }

    lgx_val_t *s = xrealloc(vm->stack.buf, size * sizeof(lgx_val_t));
    if (!s) {
        return 1;
    }
    // 初始化新空间
    memset(s + vm->stack.size, 0, size - vm->stack.size);

    vm->stack.buf = s;
    vm->stack.size = size;
    vm->regs = vm->stack.buf + vm->stack.base;

    return 0;
}

int lgx_vm_start(lgx_vm_t *vm) {
    unsigned i;
    unsigned *bc = vm->bc->bc;

    for(;;) {
        i = bc[vm->pc++];

        // 单步执行
        //lgx_bc_echo(vm->pc-1, i);
        //getchar();

        switch(OP(i)) {
            case OP_MOV:{
                lgx_gc_ref_del(&R(PA(i)));
                lgx_gc_ref_add(&R(PB(i)));

                R(PA(i)).type = R(PB(i)).type;
                R(PA(i)).v.l = R(PB(i)).v.l;

                break;
            }
            case OP_MOVI:{
                lgx_gc_ref_del(&R(PA(i)));

                R(PA(i)).type = T_LONG;
                R(PA(i)).v.l = PD(i);
                break;
            }
            case OP_ADD:{
                lgx_gc_ref_del(&R(PA(i)));

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
                    lgx_gc_trace(vm, &R(PA(i)));
                    lgx_gc_ref_add(&R(PA(i)));
                }
                break;
            }
            case OP_SUB:{
                lgx_gc_ref_del(&R(PA(i)));

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
                lgx_gc_ref_del(&R(PA(i)));

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
                lgx_gc_ref_del(&R(PA(i)));

                if (R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG) {
                    if (UNEXPECTED(R(PC(i)).v.l == 0)) {
                        throw_exception(vm, "division by zero\n");
                    } else {
                        R(PA(i)).type = T_LONG;
                        R(PA(i)).v.l = R(PB(i)).v.l / R(PC(i)).v.l;
                    }
                } else if (R(PB(i)).type == T_DOUBLE && R(PC(i)).type == T_DOUBLE) {
                    if (UNEXPECTED(R(PC(i)).v.d == 0)) {
                        throw_exception(vm, "division by zero\n");
                    } else {
                        R(PA(i)).type = T_DOUBLE;
                        R(PA(i)).v.d = R(PB(i)).v.d / R(PC(i)).v.d;
                    }
                } else {
                    if (lgx_op_div(&R(PA(i)), &R(PB(i)), &R(PC(i)))) {
                        throw_exception(vm, "error operation: %s %s %s\n", lgx_val_typeof(&R(PB(i))), "/", lgx_val_typeof(&R(PC(i))));
                    }
                }
                break;
            }
            case OP_ADDI:{
                lgx_gc_ref_del(&R(PA(i)));

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
                lgx_gc_ref_del(&R(PA(i)));

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
                lgx_gc_ref_del(&R(PA(i)));

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
                lgx_gc_ref_del(&R(PA(i)));

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
                lgx_gc_ref_del(&R(PA(i)));

                if (EXPECTED(R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG)) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l << R(PC(i)).v.l;
                } else {
                    throw_exception(vm, "error operation: %s %s %s\n", lgx_val_typeof(&R(PB(i))), "<<", lgx_val_typeof(&R(PC(i))));
                }
                break;
            }
            case OP_SHR:{
                lgx_gc_ref_del(&R(PA(i)));

                if (EXPECTED(R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG)) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l >> R(PC(i)).v.l;
                } else {
                    throw_exception(vm, "error operation: %s %s %s\n", lgx_val_typeof(&R(PB(i))), ">>", lgx_val_typeof(&R(PC(i))));
                }
                break;
            }
            case OP_SHLI:{
                lgx_gc_ref_del(&R(PA(i)));

                if (EXPECTED(R(PB(i)).type == T_LONG)) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l << PC(i);
                } else {
                    throw_exception(vm, "makes integer from %s without a cast\n", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_SHRI:{
                lgx_gc_ref_del(&R(PA(i)));

                if (EXPECTED(R(PB(i)).type == T_LONG)) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l >> PC(i);
                } else {
                    throw_exception(vm, "makes integer from %s without a cast\n", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_AND:{
                lgx_gc_ref_del(&R(PA(i)));

                if (EXPECTED(R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG)) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l & R(PC(i)).v.l;
                } else {
                    throw_exception(vm, "error operation: %s %s %s\n", lgx_val_typeof(&R(PB(i))), "&", lgx_val_typeof(&R(PC(i))));
                }
                break;
            }
            case OP_OR:{
                lgx_gc_ref_del(&R(PA(i)));

                if (EXPECTED(R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG)) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l | R(PC(i)).v.l;
                } else {
                    throw_exception(vm, "error operation: %s %s %s\n", lgx_val_typeof(&R(PB(i))), "|", lgx_val_typeof(&R(PC(i))));
                }
                break;
            }
            case OP_XOR:{
                lgx_gc_ref_del(&R(PA(i)));

                if (EXPECTED(R(PB(i)).type == T_LONG && R(PC(i)).type == T_LONG)) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = R(PB(i)).v.l ^ R(PC(i)).v.l;
                } else {
                    throw_exception(vm, "error operation: %s %s %s\n", lgx_val_typeof(&R(PB(i))), "^", lgx_val_typeof(&R(PC(i))));
                }
                break;
            }
            case OP_NOT:{
                lgx_gc_ref_del(&R(PA(i)));

                if (EXPECTED(R(PB(i)).type == T_LONG)) {
                    R(PA(i)).type = T_LONG;
                    R(PA(i)).v.l = ~R(PB(i)).v.l;
                } else {
                    throw_exception(vm, "error operation: %s %s %s\n", lgx_val_typeof(&R(PB(i))), "~", lgx_val_typeof(&R(PC(i))));
                }
                break;
            }
            case OP_EQ:{
                lgx_gc_ref_del(&R(PA(i)));

                R(PA(i)).type = T_BOOL;

                if (lgx_val_cmp(&R(PB(i)), &R(PC(i)))) {
                    R(PA(i)).v.l = 1;
                } else {
                    R(PA(i)).v.l = 0;
                }
                break;
            }
            case OP_LE:{
                lgx_gc_ref_del(&R(PA(i)));

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
                lgx_gc_ref_del(&R(PA(i)));

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
                lgx_gc_ref_del(&R(PA(i)));

                R(PA(i)).type = T_BOOL;

                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l == PC(i);
                } else {
                    R(PA(i)).v.l = 0;
                }
                break;
            }
            case OP_GEI:{
                lgx_gc_ref_del(&R(PA(i)));

                R(PA(i)).type = T_BOOL;
                
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l >= PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d >= PC(i);
                } else {
                    throw_exception(vm, "makes number from %s without a cast\n", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_LEI:{
                lgx_gc_ref_del(&R(PA(i)));

                R(PA(i)).type = T_BOOL;
                
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l <= PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d <= PC(i);
                } else {
                    throw_exception(vm, "makes number from %s without a cast\n", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_GTI:{
                lgx_gc_ref_del(&R(PA(i)));

                R(PA(i)).type = T_BOOL;
                
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l > PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d > PC(i);
                } else {
                    throw_exception(vm, "makes number from %s without a cast\n", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_LTI:{
                lgx_gc_ref_del(&R(PA(i)));

                R(PA(i)).type = T_BOOL;
                
                if (R(PB(i)).type == T_LONG) {
                    R(PA(i)).v.l = R(PB(i)).v.l < PC(i);
                } else if (R(PB(i)).type == T_DOUBLE) {
                    R(PA(i)).v.l = R(PB(i)).v.d < PC(i);
                } else {
                    throw_exception(vm, "makes number from %s without a cast\n", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_LNOT:{
                lgx_gc_ref_del(&R(PA(i)));

                R(PA(i)).type = T_BOOL;

                if (EXPECTED(R(PB(i)).type == T_BOOL)) {
                    R(PA(i)).v.l = !R(PB(i)).v.l;
                } else {
                    throw_exception(vm, "makes boolean from %s without a cast\n", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_TEST:{
                if (EXPECTED(R(PA(i)).type == T_BOOL)) {
                    if (!R(PA(i)).v.l) {
                        vm->pc += PD(i);
                    }
                } else {
                    throw_exception(vm, "makes boolean from %s without a cast\n", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_JMP:{                
                if (R(PA(i)).type == T_LONG) {
                    vm->pc = R(PA(i)).v.l;
                } else {
                    throw_exception(vm, "makes integer from %s without a cast\n", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_JMPI:{
                vm->pc = PE(i);
                break;
            }
            case OP_CALL_NEW:{
                if (EXPECTED(C(PA(i)).type == T_FUNCTION)) {
                    // 确保空余堆栈空间足够容纳本次函数调用
                    if (UNEXPECTED(lgx_vm_checkstack(vm, C(PA(i)).v.fun->stack_size) != 0)) {
                        // runtime error
                        throw_exception(vm, "maximum call stack size exceeded");
                    }
                } else {
                    // runtime error
                    throw_exception(vm, "attempt to call a %s value, function expected", lgx_val_typeof(&C(PA(i))));
                }
                break;
            }
            case OP_CALL_SET:{
                R(R(0).v.fun->stack_size + PA(i)) = R(PB(i));
                break;
            }
            case OP_CALL:{
                if (EXPECTED(C(PD(i)).type == T_FUNCTION)) {
                    lgx_fun_t *fun = C(PD(i)).v.fun;
                    unsigned int base = R(0).v.fun->stack_size;

                    // 写入函数信息
                    R(base + 0) = C(PD(i));

                    // 写入返回值地址
                    R(base + 1).type = T_LONG;
                    R(base + 1).v.l = PA(i);

                    // 写入返回地址
                    R(base + 2).type = T_LONG;
                    R(base + 2).v.l = vm->pc;

                    // 写入堆栈地址
                    R(base + 3).type = T_LONG;
                    R(base + 3).v.l = vm->stack.base;

                    if (fun->buildin) {
                        fun->buildin(vm);
                    } else {
                        // 切换执行堆栈
                        vm->stack.base += R(0).v.fun->stack_size;
                        vm->regs = vm->stack.buf + vm->stack.base;

                        // 跳转到函数入口
                        vm->pc = fun->addr;
                    }
                } else {
                    // runtime error
                    throw_exception(vm, "attempt to call a %s value, function expected", lgx_val_typeof(&C(PD(i))));
                }
                break;
            }
            case OP_RET:{
                // 跳转到调用点
                long long pc = R(2).v.l;

                // 判断返回值
                int ret_idx = R(1).v.l;
                int has_ret = PA(i);
                lgx_val_t ret_val;
                if (has_ret) {
                    ret_val = R(PA(i));
                    lgx_gc_ref_add(&ret_val);
                }

                // 释放所有局部变量和临时变量
                int n;
                for (n = 4; n < R(0).v.fun->stack_size; n ++) {
                    lgx_gc_ref_del(&R(n));
                    R(n).type = T_UNDEFINED;
                }

                // 切换执行堆栈
                vm->stack.base = R(3).v.l;
                vm->regs = vm->stack.buf + vm->stack.base;

                // 写入返回值
                lgx_gc_ref_del(&R(ret_idx));
                if (has_ret) {
                    R(ret_idx) = ret_val;
                } else {
                    R(ret_idx).type = T_UNDEFINED;
                }

                if (UNEXPECTED(pc < 0)) {
                    // 如果在顶层作用域 return，则终止运行
                    // 此时，寄存器 1 中保存着返回值
                    return 0;
                } else {
                    vm->pc = pc;
                }

                break;
            }
            case OP_ARRAY_SET:{
                if (EXPECTED(R(PA(i)).type == T_ARRAY)) {
                    if (EXPECTED(R(PB(i)).type == T_LONG || R(PB(i)).type == T_STRING)) {
                        lgx_hash_node_t n;
                        n.k = R(PB(i));
                        n.v = R(PC(i));
                        lgx_hash_set(R(PA(i)).v.arr, &n);
                        lgx_gc_ref_add(&R(PB(i)));
                        lgx_gc_ref_add(&R(PC(i)));
                    } else {
                        // runtime warning
                        throw_exception(vm, "attempt to set a %s key, integer or string expected", lgx_val_typeof(&R(PA(i))));
                    }
                } else {
                    // runtime error
                    throw_exception(vm, "attempt to set a %s value, array expected", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_ARRAY_ADD:{
                if (EXPECTED(R(PA(i)).type == T_ARRAY)) {
                    lgx_hash_add(R(PA(i)).v.arr, &R(PB(i)));
                    lgx_gc_ref_add(&R(PB(i)));
                } else {
                    // runtime error
                    throw_exception(vm, "attempt to set a %s value, array expected", lgx_val_typeof(&R(PA(i))));
                }
                break;
            }
            case OP_ARRAY_NEW:{
                lgx_gc_ref_del(&R(PA(i)));

                R(PA(i)).type = T_ARRAY;
                R(PA(i)).v.arr = lgx_hash_new(0);
                if (UNEXPECTED(!R(PA(i)).v.arr)) {
                    throw_exception(vm, "out of memory");
                }
                lgx_gc_ref_add(&R(PA(i)));
                lgx_gc_trace(vm, &R(PA(i)));
                break;
            }
            case OP_ARRAY_GET:{
                lgx_gc_ref_del(&R(PA(i)));

                if (EXPECTED(R(PB(i)).type == T_ARRAY)) {
                    if (EXPECTED(R(PC(i)).type == T_LONG || R(PC(i)).type == T_STRING)) {
                        lgx_hash_node_t *n = lgx_hash_get(R(PB(i)).v.arr, &R(PC(i)));
                        if (n) {
                            R(PA(i)) = n->v;
                        } else {
                            // runtime warning
                            R(PA(i)).type = T_UNDEFINED;
                        }
                    } else {
                        // runtime warning
                        throw_exception(vm, "attempt to index a %s key, integer or string expected", lgx_val_typeof(&R(PC(i))));
                    }
                } else {
                    throw_exception(vm, "attempt to index a %s value, array expected", lgx_val_typeof(&R(PB(i))));
                }
                break;
            }
            case OP_LOAD:{
                lgx_gc_ref_del(&R(PA(i)));
                lgx_gc_ref_add(&C(PD(i)));
                R(PA(i)) = C(PD(i));
                break;
            }
            case OP_GLOBAL_GET:{
                lgx_gc_ref_del(&R(PA(i)));
                lgx_gc_ref_add(&G(PD(i)));
                R(PA(i)) = G(PD(i));
                break;
            }
            case OP_GLOBAL_SET:{
                lgx_gc_ref_del(&G(PD(i)));
                lgx_gc_ref_add(&R(PA(i)));
                G(PD(i)) = R(PA(i));
                break;
            }
            case OP_NOP: break;
            case OP_HLT: {
                // 释放所有局部变量和临时变量
                int n;
                for (n = 4; n < R(0).v.fun->stack_size; n ++) {
                    lgx_gc_ref_del(&R(n));
                    R(n).type = T_UNDEFINED;
                }

                // 写入返回值
                R(1).type = T_UNDEFINED;

                return 0;
            }
            case OP_ECHO:{
                lgx_val_print(&R(PA(i)));
                break;
            }
            case OP_TYPEOF:{
                lgx_gc_ref_del(&R(PA(i)));
                lgx_op_typeof(&R(PA(i)), &R(PB(i)));
                break;
            }
            default:
                printf("unknown op %d @ %d\n", OP(i), vm->pc);
                return 1;
        }
    }
}