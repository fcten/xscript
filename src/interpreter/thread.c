#include "thread.h"
#include "vm.h"

int read_file(const char* file, char** p) {
    FILE* fp;
    fp = fopen(file, "rb");  // localfile文件名

    fseek(fp, 0L, SEEK_END);  /* 定位到文件末尾 */
    int flen = ftell(fp);     /* 得到文件大小 */
    *p = (char*)xmalloc(flen); /* 根据文件大小动态分配内存空间 */
    if (*p == NULL) {
        fclose(fp);
        return 0;
    }
    fseek(fp, 0L, SEEK_SET);       /* 定位到文件开头 */
    return fread(*p, 1, flen, fp); /* 一次性读取全部文件内容 */
}

void *worker(void *args) {
    lgx_thread_t *thread = args;
    //wbt_init();

    lgx_ast_t ast;
    lgx_ast_init(&ast);
    //ast.lex.length = read_file(argv[1], &ast.lex.source);
    ast.lex.length = read_file("./test.x", &ast.lex.source);

    lgx_ast_parser(&ast);
    if (ast.err_no) {
        printf("%.*s\n", ast.err_len, ast.err_info);
        return NULL;
    }

    //lgx_ast_print(ast.root, 0);

    //printf("%lld\n", execute(ast.root));

    lgx_bc_t bc;
    lgx_bc_compile(&ast, &bc);
    if (bc.err_no) {
        printf("%.*s\n", bc.err_len, bc.err_info);
        return NULL;
    }

    //lgx_bc_print(bc.bc, bc.bc_top);

    //lgx_hash_print(bc.constant);
    //printf("\n");

    lgx_vm_t vm;
    lgx_vm_init(&vm, &bc);
//    lgx_vm_init(&vm, opcode, sizeof(opcode)/sizeof(unsigned));

    vm.thread = thread;
    thread->vm = &vm;

    lgx_vm_start(&vm);
    //printf("\nprogram exit with return value: ");
    //lgx_val_print(&vm.regs[1]);
    //printf("\n");

    lgx_vm_cleanup(&vm);

    printf("thread exit\n");

    return NULL;
}

lgx_thread_t* lgx_thread_create(lgx_thread_pool_t *pool, unsigned thread_id) {
    lgx_thread_t *thread = xmalloc(sizeof(lgx_thread_t));

    thread->id = thread_id;
    thread->pool = pool;

    pthread_create(&thread->ntid, NULL, worker, thread);

    return thread;
}

lgx_thread_pool_t* lgx_thread_pool_create() {
    unsigned size = 1;

    lgx_thread_pool_t *pool = xmalloc(sizeof(lgx_thread_pool_t) + size * sizeof(lgx_thread_t *));

    int i;
    for (i = 0; i < size; i ++) {
        pool->threads[i] = lgx_thread_create(pool, i);
    }

    pool->size = size;
    pool->count = 0;

    return pool;
}

void lgx_thread_pool_wait(lgx_thread_pool_t *pool) {
    int i;
    for (i = 0; i < pool->size; i ++) {
        pthread_join(pool->threads[i]->ntid, NULL);
    }
}

lgx_thread_t* lgx_thread_get(lgx_thread_pool_t *pool) {
    lgx_thread_t *thread = pool->threads[pool->count % (pool->size - 1) + 1];
    pool->count ++;

    return thread;
}