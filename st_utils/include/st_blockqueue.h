#ifndef _ST_BLOCKQUEUE_H_
#define _ST_BLOCKQUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <assert.h>

typedef volatile unsigned int atomic_uint_t;

struct blockq {

    struct blockq_node* head;
    struct blockq_node* tail;
    atomic_uint_t count;        //其实不原子计数也行
    pthread_mutex_t lock;

    struct blockq_node* free;   //空闲的struct blockq_node，做缓存
    pthread_mutex_t f_lock;

    bool free_dat;   //调用st_blockq_free的时候是否释放数据

    int pipe_read;   //add tail
    int pipe_write;  //pop front
};

enum node_stat { node_free = 1, node_use = 2 };

struct blockq_node {
    struct blockq_node* next;
    enum node_stat stat;
    void* data;
    size_t len;
};


static inline bool st_blockq_init(struct blockq* p, bool free_dat)
{
    if (!p)
        return false;

    memset(p, 0, sizeof(struct blockq));

    pthread_mutex_init(&p->lock, NULL);
    p->head = p->tail = NULL;

    pthread_mutex_init(&p->f_lock, NULL);
    p->free = NULL;

    p->count = 0;

    p->free_dat = free_dat;

    int fds[2];
    if (pipe(fds)) {
        fprintf(stderr, "Can't create notify pipe");
        abort();
    }

    p->pipe_read = fds[0];
    p->pipe_write = fds[1];

    return true;
}

static inline void st_blockq_push(struct blockq* p, struct blockq_node* node)
{
    if (!p || !node)
        return;
    
    pthread_mutex_lock(&p->lock);

    node->next = NULL;

    assert(node->stat == node_use);

    if (p->head == NULL)
    {
        assert(p->head == p->tail);
        p->head = p->tail = node;
    }
    else
    {
        p->tail->next = node;
        p->tail = node;
    }

    __sync_fetch_and_add(&p->count, 1);
    pthread_mutex_unlock(&p->lock);

    write(p->pipe_write, "Q", 1);

    return;
}

static inline struct blockq_node* st_blockq_pop(struct blockq* p)
{
    char buf[1];
    struct blockq_node* ret = NULL;

    if (!p)
        return NULL;

    if (read(p->pipe_read, buf, 1) != 1)
    {
        fprintf(stderr, "Read from pipe error!\n");
        return NULL;
    }

    pthread_mutex_lock(&p->lock);

    ret = p->head;
    assert(ret);
    assert(ret->stat == node_use);

    if (p->tail == p->head)
    {
        assert(p->tail->next == NULL);
        assert(p->head->next == NULL);
        p->tail = p->head->next;
    }

    p->head = p->head->next;
    __sync_fetch_and_sub(&p->count, 1);
    pthread_mutex_unlock(&p->lock);

    return ret;
}

static inline void st_blockq_display(struct blockq* p)
{
    struct blockq_node* ptr = NULL;
    if (!p)
        return;

    pthread_mutex_lock(&p->lock);
    ptr = p->head;

    while (ptr)
    {
        fprintf(stderr, "数据：%s\n", (char *)ptr->data);
        assert(ptr->stat == node_use);

        if (ptr->next == NULL)
            assert(ptr == p->tail);

        ptr = ptr->next;
    }

    pthread_mutex_lock(&p->f_lock);
    size_t free_cnt = 0;
    ptr = p->free;
    while (ptr)
    {
        assert(ptr->stat == node_free);
        ++ free_cnt;
        ptr = ptr->next;
    }
    pthread_mutex_unlock(&p->f_lock);

    fprintf(stderr, "BlockQ 使用中：%u\n", p->count);
    fprintf(stderr, "BLockQ 当前空闲：%lu\n", free_cnt);
    pthread_mutex_unlock(&p->lock);

    return;
}


static inline struct blockq_node* st_blockq_request(struct blockq* p)
{
    struct blockq_node* ret = NULL;

    if (!p)
        return NULL;

    pthread_mutex_lock(&p->f_lock);

    if (!p->free)
    {
        fprintf(stderr, "Allocate 20 items!!!\n");
        struct blockq_node* ptr = (struct blockq_node*)
                calloc(20, sizeof(struct blockq_node));   //一次性多分配点

        int i = 0;
        for (i=0; i<20; ++i)
        {
            ptr[i].next = p->free;
            ptr[i].stat = node_free;
            p->free = &ptr[i];
        }
    }

    ret = p->free;
    ret->stat = node_use;
    p->free = p->free->next;

    pthread_mutex_unlock(&p->f_lock);

    return ret;
}

static inline void st_blockq_free(struct blockq* p, struct blockq_node* node)
{

    if (!p || !node)
        return;
 
    pthread_mutex_lock(&p->f_lock);

    if (!p->free)
    {
        node->next = NULL;
        p->free = node;
    }
    else
    {
        node->next = p->free;
        p->free = node;
    }

    node->stat = node_free;
    if (p->free_dat) 
    {
        free(node->data);
        node->data = NULL;
        node->len = 0;
    }

    pthread_mutex_unlock(&p->f_lock);

    return;
}

static inline bool st_blockq_init(struct blockq* p, bool free_dat);

static inline void st_blockq_push(struct blockq* p, struct blockq_node* node);
static inline struct blockq_node* st_blockq_pop(struct blockq* p);

static inline struct blockq_node* st_blockq_request(struct blockq* p);
static inline void st_blockq_free(struct blockq* p, struct blockq_node* node);

static inline void st_blockq_display(struct blockq* p);

#ifdef __cplusplus
}
#endif //__cplusplus


#endif //_ST_BLOCKQUEUE_H_
