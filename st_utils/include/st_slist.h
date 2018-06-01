#ifndef __ST_SLIST_H
#define __ST_SLIST_H

#ifdef __cplusplus 
extern "C" {
#endif //__cplusplus 

#include <stdio.h>
#include <stdlib.h>

#include "st_others.h"

//根据list_head简化得到的单向NULL结尾链表
typedef struct _slist_head {
	struct _slist_head *next;
} SLIST_HEAD, *P_SLIST_HEAD;

static inline void slist_init(P_SLIST_HEAD head)
{
	head->next = NULL;
}

// replaced of container_of
#define container_of(ptr, type, member) \
((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))  

#define list_entry(ptr, type, member) \
((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))  

#define slist_for_each(pos, head) \
	for (pos = (head)->next; pos != NULL; pos = pos->next)  

// 可以删除元素，安全链表
#define slist_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos && ({ n = pos->next; 1; }); pos = n)  

#define slist_for_each_entry(pos, head, member)\
	for (pos = list_entry((head)->next, typeof(*pos), member);  \
        &pos->member != NULL;								\
         pos = list_entry(pos->member.next, typeof(*pos), member))


#define byte_offset(base, offset) \
            ((char *)(base) + (offset))

static inline void slist_add(P_SLIST_HEAD new_item, P_SLIST_HEAD head)
{
	P_SLIST_HEAD ptr =  head;
	while(ptr->next)
		ptr = ptr->next;
	ptr->next = new_item;
	new_item->next = NULL;

	//new_item->next = head->next;
	//head->next = new_item;
}

static inline int slist_empty(const P_SLIST_HEAD head)
{
	return head->next == NULL;
}

static inline void slist_remove(P_SLIST_HEAD del_item, P_SLIST_HEAD head)
{
    P_SLIST_HEAD pos = NULL;

    if (!head || !del_item || slist_empty(head))
        return;

    if ( head->next == del_item)
    {
        head->next = head->next->next;
        return;
    }

    slist_for_each(pos, head)
    {
        if ( pos->next == del_item)
        {
            pos->next = pos->next->next;
            return;
        }
    }
}

static inline unsigned int slist_count (const P_SLIST_HEAD head)
{
	int count = 0;
	P_SLIST_HEAD pos = NULL;

	if(!head)
		return -1;

	slist_for_each(pos, head)
		count ++;

	return count;
}

// 先进先出的队列
static inline P_SLIST_HEAD slist_fetch(P_SLIST_HEAD head)
{
    P_SLIST_HEAD ret = NULL;

    if (!head || slist_empty(head))
    {
        //cout << "EMPTY_LIST" << endl;
        return NULL;   
    }
    
    ret = head->next;
    head->next = head->next->next;  //may also be null

    return ret;
}

// 返回链表最后一个LIST_HEAD
static inline P_SLIST_HEAD slist_last(P_SLIST_HEAD head)
{
    P_SLIST_HEAD ret = head;

    if (!head || slist_empty(head))
    {
        return NULL;   
    }
    
    while ( ret->next->next != NULL) 
        ret = ret->next;

    return ret->next;
}

static inline int slist_u_test(void)
{
	typedef struct _test_struct 
	{
		SLIST_HEAD list;
		int data;
	} TEST_STRUCT, *P_TEST_STRUCT;

	int tmp = 0;
	
	SLIST_HEAD test_head = {0};
	P_SLIST_HEAD p_test_head = &test_head;
	P_SLIST_HEAD pos = NULL; 

	tmp = slist_empty(p_test_head);
	if(!tmp)
	{
		fprintf(stderr,"slist_empty test FAILED!\n");
		return 0;
	}
	else
		fprintf(stderr,"slist_empty test PASS!\n");

	tmp = slist_count(p_test_head);
	if(tmp != 0 )
	{
		fprintf(stderr,"slist_count test FAILED!\n");
		return 0;
	}

	int i = 1, data;
	for(i = 1; i <= 10; i++)  
	{   
		data = (11 - i) * 10;  //data 

		P_TEST_STRUCT p_node = (P_TEST_STRUCT)malloc(sizeof(TEST_STRUCT));
		p_node->data = data;
		slist_add(&p_node->list, p_test_head);

		tmp = slist_count(p_test_head);
		if(tmp != i)
		{
			fprintf(stderr,"slist_count test FAIL!\n");
			return 0;
		}
	}   
	
	fprintf(stderr,"slist_count test PASS!\n");
    
	i = 10;
	slist_for_each(pos, p_test_head)  
	{   
		P_TEST_STRUCT p_node = list_entry(pos, TEST_STRUCT, list);   
		tmp = i-- * 10;
		if(tmp != p_node->data)
		{
            fprintf(stderr,"list_entry test FAIL!\n");
            fprintf(stderr,"%d <-> %d \n", tmp, p_node->data);
            return 0;
		}
	}   

    fprintf(stderr,"list_entry test PASS!\n");

    P_TEST_STRUCT p_node = NULL;

    slist_for_each_entry(p_node, p_test_head, list)
        st_print("%d\n", p_node->data);

	//pos = slist_fetch(p_test_head);
    //pos = slist_fetch(p_test_head);
    pos = slist_fetch(p_test_head);
    p_node = list_entry(pos, TEST_STRUCT, list);
    if (p_node->data != 100)
    {
        fprintf(stderr,"list_fetch test FAIL!\n");
        fprintf(stderr,"%d \n", p_node->data);
        return 0;
    }
    free(p_node);

    //测试，不关心内存泄漏
    pos = slist_fetch(p_test_head); //90
    pos = slist_fetch(p_test_head); //80
    pos = slist_fetch(p_test_head); //70
    pos = slist_fetch(p_test_head); //60
    p_node = list_entry(pos, TEST_STRUCT, list);
    if (p_node->data != 60)
    {
        fprintf(stderr,"list_fetch test FAIL!\n");
        fprintf(stderr,"%d \n", p_node->data);
        return 0;
    }
    free(p_node);

    fprintf(stderr,"STAGE-PA\n");
    slist_for_each_entry(p_node, p_test_head, list)
        st_print("%d\n", p_node->data);

    slist_for_each_entry(p_node, p_test_head, list)
    {
        if(p_node->data == 20 || p_node->data == 50)
        {
            slist_remove(&p_node->list, p_test_head);
        }
    }

    fprintf(stderr,"STAGE-PB\n");
    slist_for_each_entry(p_node, p_test_head, list)
        st_print("%d\n", p_node->data);

    //pos = slist_fetch(p_test_head); //50
    pos = slist_fetch(p_test_head); //40
    pos = slist_fetch(p_test_head); //30
    //pos = slist_fetch(p_test_head); //20
    pos = slist_fetch(p_test_head); //10
    p_node = list_entry(pos, TEST_STRUCT, list);
    if (p_node->data != 10)
    {
        fprintf(stderr,"list_fetch test FAIL!\n");
        fprintf(stderr,"%d \n", p_node->data);
        return 0;
    }
    free(p_node);

    slist_for_each_entry(p_node, p_test_head, list)
        st_print("%d\n", p_node->data);

    if (slist_fetch(p_test_head) != NULL)
    {
        fprintf(stderr,"list_fetch test FAIL!\n");
        return 0;
    }
	fprintf(stderr,"list_fetch test PASS!\n");

	fprintf(stderr,"slist unit test PASS!");
}



#ifdef __cplusplus 
}
#endif //__cplusplus 

#endif //ST_SLIST_H