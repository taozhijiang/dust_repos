#include "rb_tree.hpp"
#include <assert.h>


int main(int argc, char* argv[])
{
    std::cout << "RB Tree Test" << std::endl;
    RBTree rbtree;
#if 0
    rbtree.insert(30);
    rbtree.insert(20);
    rbtree.insert(40);
    rbtree.insert(11);
    rbtree.insert(62);
    rbtree.insert(71);
    rbtree.insert(65);
    rbtree.insert(78);
    rbtree.insert(64);
#else
    rbtree.insert(3);
    rbtree.insert(7);
    rbtree.insert(18);
    rbtree.insert(10);
    rbtree.insert(23);
    rbtree.insert(8);
    rbtree.insert(11);
    rbtree.insert(26);
#endif
    rbtree.display(rbtree.root_, 1);

    rbtree.remove(23);
    rbtree.remove(10);
    rbtree.display(rbtree.root_, 1);


    rbtree.remove(7);
    rbtree.remove(3);
    rbtree.display(rbtree.root_, 1);

    rbtree.remove(8);
    rbtree.remove(11);
    rbtree.remove(26);
    rbtree.remove(18);

    return 0;
}
