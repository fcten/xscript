#include <pthread.h>

#include "./common/common.h"
#include "./tokenizer/lex.h"
#include "./parser/ast.h"
#include "./parser/scope.h"
#include "./common/bytecode.h"
#include "./interpreter/vm.h"
#include "./interpreter/thread.h"
#include "./compiler/compiler.h"

int main(int argc, char* argv[]) {
    //wbt_init();

    lgx_lex_init();

    lgx_thread_pool_t *pool = lgx_thread_pool_create();

    lgx_thread_pool_wait(pool);

    return 0;
}