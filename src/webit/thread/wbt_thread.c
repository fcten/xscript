#define _GNU_SOURCE

#include "../common/wbt_error.h"
#include "wbt_thread.h"

static long get_cpu_num() {
#ifdef _SC_NPROCESSORS_ONLN
	long nprocs       = -1;
	long nprocs_max   = -1;

	nprocs = sysconf(_SC_NPROCESSORS_ONLN);
	if (nprocs < 1) {
		wbt_debug("Could not determine number of CPUs on line, %s",
			strerror(errno));
		return 0;
	}
	nprocs_max = sysconf(_SC_NPROCESSORS_CONF);
	if (nprocs_max < 1) {
		wbt_debug("Could not determine number of CPUs in host, %s",
			strerror(errno));
		return 0;
	}
	wbt_debug("%d of %d CPUs online", nprocs, nprocs_max);
	return nprocs;
#else
	wbt_debug("Could not determine number of CPUs");
	return 0;
#endif
}

int wbt_thread_init(wbt_thread_t *thread) {
    thread->queue = wbt_queue_create(256);
    if (!thread->queue) {
        return 1;
    }

    if (pthread_create(&thread->ntid, NULL, thread->fn, thread) != 0) {
        wbt_queue_delete(thread->queue);
        thread->queue = NULL;
        return 1;
    }

    return 0;
}

int wbt_thread_wait(wbt_thread_t *thread) {
    return pthread_join(thread->ntid, NULL);
}

int wbt_thread_send(wbt_thread_t *thread, wbt_socket_t val) {
    if (wbt_queue_write(thread->queue, val) != 0) {
        return 1;
    }

    pthread_kill(thread->ntid, SIGUSR1);

    return 0;
}

int wbt_thread_recv(wbt_thread_t *thread, wbt_socket_t *val) {
    return wbt_queue_read(thread->queue, val);
}

wbt_thread_pool_t * wbt_thread_create_pool(unsigned size, void* (*fn)(void *thread), void *ctx) {
    long cpu_num = get_cpu_num();
    if (cpu_num > 0) {
        size = cpu_num;
    }

    wbt_thread_pool_t *pool = wbt_malloc(sizeof(wbt_thread_pool_t) + size * sizeof(wbt_thread_t));
    if (!pool) {
        return NULL;
    }

    pool->size = size;
    pool->count = 0;

    int i;
    for (i = 0; i < size; i ++) {
        pool->threads[i].fn = fn;
        pool->threads[i].ctx = ctx;
        if (wbt_thread_init(&pool->threads[i]) != 0) {
            if (i == 0) {
                wbt_free(pool);
                return NULL;
            } else {
                pool->size = i;
                return pool;
            }
        }
        cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(i, &cpuset);
		if (pthread_setaffinity_np(pool->threads[i].ntid, sizeof(cpu_set_t), &cpuset) != 0) {
			wbt_debug("can not set thread %d affinity, %s\n", i, strerror(errno));
		}
    }

    return pool;
}

wbt_thread_t * wbt_thread_get(wbt_thread_pool_t *pool) {
    return &pool->threads[pool->count ++ % pool->size];
}