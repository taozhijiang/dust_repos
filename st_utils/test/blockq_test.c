#include "st_blockqueue.h"

int test0(void)
{
    struct blockq queue;

    if (! st_blockq_init(&queue, false))
    {
        exit(-1);
    }

    st_blockq_display(&queue);

    return 0;
}

int test1(void)
{
    struct blockq queue;

    if (! st_blockq_init(&queue, false))
    {
        exit(-1);
    }

    struct blockq_node *node1 = NULL;
    struct blockq_node *node2 = NULL;
    struct blockq_node *node3 = NULL;
    struct blockq_node *node4 = NULL;
    struct blockq_node *node5 = NULL;
    struct blockq_node *node6 = NULL;


    node1 = st_blockq_request(&queue);
    node1->data = "DDD";

    node2 = st_blockq_request(&queue);
    node2->data = "TTTT";

    st_blockq_display(&queue);
    st_blockq_push(&queue, node1);
    st_blockq_display(&queue);
    st_blockq_push(&queue, node2);
    st_blockq_display(&queue);

    node3 = st_blockq_pop(&queue);

    st_blockq_display(&queue);
    node4 = st_blockq_pop(&queue);

    assert(node3 == node1);
    assert(node4 == node2);

    st_blockq_display(&queue);
    fprintf(stderr, "XXXX");

    st_blockq_free(&queue, node1);
    st_blockq_free(&queue, node2);

    printf("OK!\n");

    return 0;
}

int test2(void)
{
    struct blockq_node* nodes[1024];
    int i = 0;
    struct blockq q;

    if (! st_blockq_init(&q, true))
    {
        exit(-1);
    }

    char format[24];
    for (i=0; i<1024; ++i)
    {
        sprintf(format, "DA<%d>DA", i);
        nodes[i] = st_blockq_request(&q);
        nodes[i]->data = (void *)strdup(format); 
        nodes[i]->len = strlen(format) + 1;
        st_blockq_push(&q, nodes[i]);
    }

    assert(q.count == 1024);
    st_blockq_display(&q);

    struct blockq_node* ptr;
    for (i=0; i<100; ++i)
    {
        ptr = st_blockq_pop(&q);
        st_blockq_free(&q, ptr);
    }

    assert(q.count == 924);
    st_blockq_display(&q);

    printf("OK!\n");
    return 0;
}

int test3(void)
{
    struct blockq q;

    if (! st_blockq_init(&q, true))
    {
        exit(-1);
    }

    struct blockq_node* ptr;
    ptr = st_blockq_pop(&q); 

    assert(1);
    fprintf(stderr, "NEVER SEE THIS!\n");

    return;
}

int main(int argc, char* argv[])
{
    test0();
    test1();
    test2();
    test3();

    return 0;
}
