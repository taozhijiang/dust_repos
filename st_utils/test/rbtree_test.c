#include "rbtree.h"

#include "st_others.h"

#include <string.h>

struct mytype {
    struct rb_node node;
    char *keystring;    // sortable data
};


struct mytype *my_search(struct rb_root *root, char *string)
{
    struct rb_node *node = root->rb_node;

    while (node) 
    {
        struct mytype *data = container_of(node, struct mytype, node);
        int result;

        result = strcmp(string, data->keystring);

        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else
            return data;
    }

    return NULL;
}


int my_insert(struct rb_root *root, struct mytype *data)
{
    struct rb_node **new = &(root->rb_node), *parent = NULL;

    /* Figure out where to put new node */
    while (*new) 
    {
        struct mytype *this = container_of(*new, struct mytype, node);
        int result = strcmp(data->keystring, this->keystring);

        parent = *new;
        if (result < 0)
            new = &((*new)->rb_left);
        else if (result > 0)
            new = &((*new)->rb_right);
        else
            return FALSE;
    }

    /* Add new node and rebalance tree. */
    rb_link_node(&data->node, parent, new);
    rb_insert_color(&data->node, root);

    return TRUE;
}


void rb_erase(struct rb_node *victim, struct rb_root *tree);


int main(int argc, char* argv[])
{

    struct rb_root mytree = RB_ROOT;
    struct mytype* ptr = NULL;
    struct rb_node* node = NULL;

    struct mytype dat1, dat2, dat3, dat4, dat5;
    dat1.keystring = "桃子x号!";
    dat2.keystring = "桃子z号码!";
    dat3.keystring = "桃子y号!";
    dat4.keystring = "桃子w号码!";
    dat5.keystring = "桃子t号啊啊啊啊啊!";

    fprintf(stderr,"测试开始！\n");

    my_insert(&mytree, &dat1);
    my_insert(&mytree, &dat2);
    my_insert(&mytree, &dat3);

    for (node = rb_first(&mytree); node; node = rb_next(node))
    {
        ptr = rb_entry(node, struct mytype, node);
        fprintf(stderr,"%s\n", ptr->keystring);
    }

    ptr = my_search(&mytree, "桃子x号!");
    if (ptr)
        fprintf(stderr, "搜索到：%s\n", ptr->keystring);
    else
        goto FAIL;

    ptr = my_search(&mytree, "桃子码!");
    if (!ptr)
        fprintf(stderr, "搜索不存在key值！\n");
    else
        goto FAIL;

    my_insert(&mytree, &dat4);
    my_insert(&mytree, &dat5);

    for (node = rb_first(&mytree); node; node = rb_next(node))
    {
        ptr = rb_entry(node, struct mytype, node);
        fprintf(stderr,"%s\n", ptr->keystring);
    }

    ptr = my_search(&mytree, "桃子x号!");
    if (ptr)
        rb_erase(&(ptr->node), &mytree);
    else
        goto FAIL;

    if(my_search(&mytree, "桃子x号!"))
        goto FAIL;

    rb_erase(&(my_search(&mytree, "桃子w号码!")->node), &mytree);

    for (node = rb_first(&mytree); node; node = rb_next(node))
    {
        ptr = rb_entry(node, struct mytype, node);
        fprintf(stderr,"%s\n", ptr->keystring);
    }

PASS:
        fprintf(stderr, "测试成功！\n");
        return 0;

FAIL:
        fprintf(stderr, "测试失败！\n");
        return 1;
}
