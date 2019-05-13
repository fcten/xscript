#ifndef LGX_LIST_H
#define	LGX_LIST_H

#include "common.h"

typedef struct lgx_list_s {  
    struct lgx_list_s *next, *prev;  
} lgx_list_t;

/**
 * 用于通过 lgx_list_t 指针获得完整结构体的指针
 * @ptr:            lgx_list_t 结构体指针
 * @type:           想要获取的完整结构体类型
 * @member:         lgx_list_t 成员在完整结构体中的名称
 */
#define lgx_list_entry(ptr, type, member) \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/**
 * 用于 lgx_list_t 初始化，必须先声明再调用
 * @ptr:     lgx_list_t 结构体指针
 */
#define lgx_list_init(ptr) do { \
    (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

/* 对 lgx_list_t 结构体进行遍历 */
#define lgx_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/* 获取链表的第一个元素 */
#define lgx_list_first_entry(ptr, type, member) \
	lgx_list_entry((ptr)->next, type, member)

/* 获取链表的最后一个元素 */
#define lgx_list_last_entry(ptr, type, member) \
	lgx_list_entry((ptr)->prev, type, member)

/* 获取链表的下一个元素 */
#define lgx_list_next_entry(pos, type, member) \
	lgx_list_entry((pos)->member.next, type, member)

/* 获取链表的上一个元素 */
#define lgx_list_prev_entry(pos, type, member) \
	lgx_list_entry((pos)->member.prev, type, member)

/**
 * 对 lgx_list_t 结构体进行遍历并返回完整结构体的指针
 * @pos:            用于遍历的完整结构体临时指针
 * @head:           链表头中的 lgx_list_t 指针
 * @member:         lgx_list_t 成员在完整结构体中的名称
 */
#define lgx_list_for_each_entry(pos, type, head, member)			\
	for (pos = lgx_list_first_entry(head, type, member);	\
	     &pos->member != (head);					\
	     pos = lgx_list_next_entry(pos, type, member))

static lgx_inline void __lgx_list_add(lgx_list_t *list, lgx_list_t *prev, lgx_list_t *next) {
    next->prev = list;
    list->next = next;
    list->prev = prev;
    prev->next = list;
}

static lgx_inline void __lgx_list_del(lgx_list_t *prev, lgx_list_t *next) {
    next->prev = prev;
    prev->next = next;
}
  
static lgx_inline void lgx_list_add(lgx_list_t *list, lgx_list_t *head) {
    __lgx_list_add(list, head, head->next);
}

static lgx_inline void lgx_list_add_tail(lgx_list_t *list, lgx_list_t *head) {
    __lgx_list_add(list, head->prev, head);
}  

static lgx_inline void lgx_list_del(lgx_list_t *entry) {
    __lgx_list_del(entry->prev, entry->next);
}

static lgx_inline int lgx_list_empty(const lgx_list_t *head) {
	return head->next == head;
}

static lgx_inline void lgx_list_move_tail(lgx_list_t *list, lgx_list_t *head) {
	lgx_list_del(list);
	lgx_list_add_tail(list, head);
}

#endif	/* LGX_LIST_H */
