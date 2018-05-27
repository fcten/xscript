#ifndef LGX_LIST_H
#define	LGX_LIST_H

typedef struct lgx_list_s {  
    struct lgx_list_s *next, *prev;  
} lgx_list_t;

#define lgx_list_entry(ptr, type, member) \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#define lgx_list_init(ptr) do { \
    (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

#define lgx_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define lgx_list_first_entry(ptr, type, member) \
	lgx_list_entry((ptr)->next, type, member)

#define lgx_list_last_entry(ptr, type, member) \
	lgx_list_entry((ptr)->prev, type, member)

#define lgx_list_next_entry(pos, type, member) \
	lgx_list_entry((pos)->member.next, type, member)

#define lgx_list_prev_entry(pos, type, member) \
	lgx_list_entry((pos)->member.prev, type, member)

#define lgx_list_for_each_entry(pos, type, head, member)			\
	for (pos = lgx_list_first_entry(head, type, member);	\
	     &pos->member != (head);					\
	     pos = lgx_list_next_entry(pos, type, member))

static inline void __lgx_list_add(lgx_list_t *list, lgx_list_t *prev, lgx_list_t *next) {
    next->prev = list;
    list->next = next;
    list->prev = prev;
    prev->next = list;
}

static inline void __lgx_list_del(lgx_list_t *prev, lgx_list_t *next) {
    next->prev = prev;
    prev->next = next;
}
  
static inline void lgx_list_add(lgx_list_t *list, lgx_list_t *head) {
    __lgx_list_add(list, head, head->next);
}

static inline void lgx_list_add_tail(lgx_list_t *list, lgx_list_t *head) {
    __lgx_list_add(list, head->prev, head);
}  

static inline void lgx_list_del(lgx_list_t *entry) {
    __lgx_list_del(entry->prev, entry->next);
}

static inline int lgx_list_empty(const lgx_list_t *head) {
	return head->next == head;
}

static inline void lgx_list_move_tail(lgx_list_t *list, lgx_list_t *head) {
	lgx_list_del(list);
	lgx_list_add_tail(list, head);
}

#endif	/* LGX_LIST_H */

