#ifndef __ST_DLLIST_H
#define __ST_DLLIST_H

#ifdef __cplusplus 
extern "C" {
#endif //__cplusplus 

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "st_others.h"
#include "st_slist.h"

//根据list_head简化得到的双向循环链表


#define dl_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define dl_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

#define dl_last_entry(ptr, type, member) \
	list_entry((ptr)->prev, type, member)

#define dl_next_entry(pos, member) \
	list_entry((pos)->member.next, typeof(*(pos)), member)

#define dl_prev_entry(pos, member) \
	list_entry((pos)->member.prev, typeof(*(pos)), member)

#define dl_for_each_entry(pos, head, member)				\
	for (pos = dl_first_entry(head, typeof(*pos), member);	\
	     &pos->member != (head);					\
	     pos = dl_next_entry(pos, member))

#define dl_for_each_entry_safe(pos, n, head, member)			\
	for (pos = dl_first_entry(head, typeof(*pos), member),	\
		n = dl_next_entry(pos, member);			\
	     &pos->member != (head); 					\
	     pos = n, n = dl_next_entry(n, member))

#define dl_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = dl_last_entry(head, typeof(*pos), member),		\
		n = dl_prev_entry(pos, member);			\
	     &pos->member != (head); 					\
	     pos = n, n = dl_prev_entry(n, member))


typedef struct _dl_head {
    struct _dl_head *prev;
	struct _dl_head *next;
} DL_HEAD, *P_DL_HEAD;

static inline void dl_init(struct _dl_head* d_head)
{
    d_head->prev = d_head; 
	d_head->next = d_head;
}


static inline void dl_add(struct _dl_head*  new_item, struct _dl_head* head)
{
	head->next->prev = new_item;
    new_item->next = head->next;
    new_item->prev = head;
    head->next = new_item;
}

static inline void dl_add_tail(struct _dl_head* new_item, struct _dl_head* head)
{
    head->prev->next = new_item;
    new_item->prev = head->prev;
    new_item->next = head;
    head->prev = new_item;
}

static inline int dl_empty(struct _dl_head* head)
{
    if (head->next == head)
    {
        assert(head->prev = head);
        return 1;
    }

    return 0;
}

// should not remove head!!!!
static inline void dl_remove(struct _dl_head* del_item, struct _dl_head* head)
{
    if (del_item == head) 
        return;
    
    del_item->prev->next = del_item->next; 
    del_item->next->prev = del_item->prev;
}

static inline unsigned int dl_count (const struct _dl_head* head)
{
	unsigned int count = 0;
    
    struct _dl_head* pos;
    dl_for_each(pos, head)
        ++ count;

	return count;
}

#if 0
static inline int dlist_u_test(void)
{
    typedef struct _test_struct 
    {
        struct _dl_head list;
        int data;
    } ST;

    struct _dl_head* pos;
    struct _test_struct* p_tst;
    struct _test_struct* n;
	int tmp = 0;
	
	struct _dl_head dl_head;
    dl_init(&dl_head);
    st_d_print("CONT: %u\n", dl_count(&dl_head));

    st_d_print("ITEM:");
    dl_for_each_entry(p_tst, &dl_head, list)
        fprintf(stderr, "%d\t", p_tst->data); 
    fprintf(stderr, "\n");

    ST st1; st1.data = 12;
    ST st2; st2.data = 14;
    ST st3; st3.data = 16;
    ST st4; st4.data = 18;
    ST st5; st5.data = 20;
    ST st6; st6.data = 22;
    dl_add(&st1.list, &dl_head);
    dl_add(&st2.list, &dl_head);
    dl_add_tail(&st3.list, &dl_head);
    dl_add_tail(&st4.list, &dl_head);
    dl_add_tail(&st5.list, &dl_head);

    st_d_print("CONT: %u\n", dl_count(&dl_head));

    st_d_print("ITEM:");
    dl_for_each_entry(p_tst, &dl_head, list)
        fprintf(stderr, "%d\t", p_tst->data); 
    fprintf(stderr, "\n");

    dl_remove(&st2.list, &dl_head);
    st_d_print("ITEM:");
    dl_for_each_entry(p_tst, &dl_head, list)
        fprintf(stderr, "%d\t", p_tst->data); 
    fprintf(stderr, "\n");

    dl_remove(&dl_head, &dl_head);
    st_d_print("ITEM:");
    dl_for_each_entry(p_tst, &dl_head, list)
        fprintf(stderr, "%d\t", p_tst->data); 
    fprintf(stderr, "\n");

    dl_for_each_entry_safe(p_tst, n, &dl_head, list)
    {
        if (p_tst->data == 12) 
            dl_remove(&p_tst->list, &dl_head);
        if (p_tst->data == 20)
            dl_remove(&p_tst->list, &dl_head);
    }


    st_d_print("CONT: %u\n", dl_count(&dl_head));

    st_d_print("ITEM:");
    dl_for_each_entry(p_tst, &dl_head, list)
        fprintf(stderr, "%d\t", p_tst->data); 
    fprintf(stderr, "\n");

    dl_add_tail(&st6.list, &dl_head);
    dl_for_each_entry_safe_reverse(p_tst, n, &dl_head, list)
    {
        if (p_tst->data == 22) 
            dl_remove(&p_tst->list, &dl_head);
    }
    st_d_print("CONT: %u\n", dl_count(&dl_head));

    dl_for_each_entry_safe_reverse(p_tst, n, &dl_head, list)
    {
        dl_remove(&p_tst->list, &dl_head);
    }

    st_d_print("CONT: %u\n", dl_count(&dl_head));

	fprintf(stderr,"dlist unit test PASS!");

    return 0;
}

#endif


#ifdef __cplusplus 
}
#endif //__cplusplus 

#endif //ST_DLIST_H