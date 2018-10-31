/* 
 * File:   wbt_heap.c
 * Author: Fcten
 *
 * Created on 2014年8月28日, 下午2:56
 */

#include "wbt_heap.h"
#include "wbt_memory.h"
#include "wbt_error.h"

/* 建立一个空堆 */
wbt_status wbt_heap_init(wbt_heap_t * p, size_t max_size) {
    if( ( p->heap = wbt_malloc((max_size + 1) * sizeof(wbt_timer_t *)) ) == NULL ) {
        return WBT_ERROR;
    }

    p->heap[0] = NULL;
    p->max     = max_size;
    p->size    = 0;
    
    return WBT_OK;
}

/* 向堆中插入一个新元素 */
wbt_status wbt_heap_insert(wbt_heap_t * p, wbt_timer_t * node) {
    if(p->size + 1 == p->max) {
        /* 堆已经满了，尝试扩充大小 */
        void *new_p = wbt_realloc(p->heap, (p->max*2 + 1) * sizeof(wbt_timer_t *));
        if( new_p != NULL ) {
            p->heap = new_p;
            p->max *= 2;
        } else {
            wbt_error("heap overflow at size %u\n", p->max);

            return WBT_ERROR;
        }
    }
    
    // 将元素插入到堆末尾
    int i = ++p->size;
    p->heap[i] = node;
    p->heap[i]->heap_idx = i;
    
    // 调整堆使其符合最小堆性质
    for( ; i/2 > 0 && p->heap[i/2]->timeout > p->heap[i]->timeout; i /= 2 ) { 
        p->heap[i]->heap_idx = i/2;
        p->heap[i/2]->heap_idx = i;

        p->heap[0] = p->heap[i/2];
        p->heap[i/2] = p->heap[i];
        p->heap[i] = p->heap[0];
    }
    
    //wbt_log_debug("heap insert, %d nodes.\n", p->size);
    
    return WBT_OK;
}

/* 获取堆顶元素的值 */
wbt_timer_t * wbt_heap_get(wbt_heap_t * p) {
    if(p->size > 0) {
        return p->heap[1];
    } else {
        return NULL;
    }
}

/* 删除堆顶元素 */
wbt_status wbt_heap_delete(wbt_heap_t * p) {
    if( p->size == 0 ) {
        /* 堆是空的 */
        return WBT_ERROR;
    }
    
    return wbt_heap_remove(p, 1);
}

wbt_status wbt_heap_remove(wbt_heap_t * p, unsigned int heap_idx) {
    // 标记为已删除
    p->heap[heap_idx]->heap_idx = 0;

    // 依次用父元素覆盖当前元素，以空出堆顶位置
    int i = heap_idx;
    for( ; i/2 > 0 ; i /= 2 ) { 
        p->heap[i] = p->heap[i/2];
        p->heap[i]->heap_idx = i;
    }
    // 将最后一个元素移动到堆顶位置
    if( p->size > 1 ) {
        p->heap[1] = p->heap[p->size--];
        p->heap[1]->heap_idx = 1;
    } else {
        p->size = 0;
    }

    // 调整堆使其符合最小堆性质
    unsigned int current = 1, child = 2;
    for( ; child <= p->size ; current = child, child *= 2 ) {
        if( child + 1 <= p->size && p->heap[child+1]->timeout <  p->heap[child]->timeout ) {
            child++;
        }

        if( p->heap[child]->timeout < p->heap[current]->timeout ) {
            p->heap[child]->heap_idx = current;
            p->heap[current]->heap_idx = child;
            
            p->heap[0] = p->heap[current];
            p->heap[current] = p->heap[child];
            p->heap[child] = p->heap[0];
        } else {
            break;
        }
    }

    //wbt_log_debug("heap remove, %d nodes.\n", p->size);

    // 删除元素后尝试释放空间
    // 为每一个最小堆添加定时 GC 任务太复杂了，我认为目前的做法可以接受
    if( p->max >= p->size * 4 && p->size >= 256 ) {
        p->heap = wbt_realloc( p->heap, sizeof(wbt_timer_t *) * (p->max/2 + 1) );
        p->max /= 2;
    }
    
    return WBT_OK;
}

wbt_status wbt_heap_update(wbt_heap_t * p, unsigned int heap_idx) {
    // 保存当前元素
    p->heap[0] = p->heap[heap_idx];
    
    // 依次用父元素覆盖当前元素，以空出堆顶位置
    int i = heap_idx;
    for( ; i/2 > 0 ; i /= 2 ) { 
        p->heap[i] = p->heap[i/2];
        p->heap[i]->heap_idx = i;
    }
    
    // 将当前元素移动到堆顶位置
    p->heap[1] = p->heap[0];
    p->heap[1]->heap_idx = 1;
    
    // 调整堆使其符合最小堆性质
    unsigned int current = 1, child = 2;
    for( ; child <= p->size ; current = child, child *= 2 ) {
        if( child + 1 <= p->size && p->heap[child+1]->timeout <  p->heap[child]->timeout ) {
            child++;
        }

        if( p->heap[child]->timeout < p->heap[current]->timeout ) {
            p->heap[child]->heap_idx = current;
            p->heap[current]->heap_idx = child;
            
            p->heap[0] = p->heap[current];
            p->heap[current] = p->heap[child];
            p->heap[child] = p->heap[0];
        } else {
            break;
        }
    }

    return WBT_OK;
}

/* 删除堆 */
wbt_status wbt_heap_destroy(wbt_heap_t * p) {
    if (p->heap) {
        wbt_free(p->heap);
    }
    p->heap = NULL;
    p->max = 0;
    p->size = 0;

    return WBT_OK;
}

/**
 * 从最小堆定时器中删除所有已超时的事件
 * @param p
 */
extern time_t wbt_cur_mtime;

void wbt_heap_delete_timeout(wbt_heap_t * p) {
    wbt_timer_t * node = wbt_heap_get(p);
    while( node && node->timeout <= wbt_cur_mtime ) {
        wbt_heap_delete(p);
        node = wbt_heap_get(p);
    }
}
