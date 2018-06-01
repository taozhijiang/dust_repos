#include "avl_tree.hpp"
#include <assert.h>


int main(int argc, char* argv[])
{
    std::cout << "AVL Tree Test" << std::endl;

    AvlTree avltree;
    avltree.root_ = avltree.insert(avltree.root_, 8);
    avltree.root_ = avltree.insert(avltree.root_, 5);
    avltree.root_ = avltree.insert(avltree.root_, 4);
    avltree.root_ = avltree.insert(avltree.root_, 11);
    avltree.root_ = avltree.insert(avltree.root_, 15);
    avltree.root_ = avltree.insert(avltree.root_, 3);
    avltree.root_ = avltree.insert(avltree.root_, 6); 
    avltree.root_ = avltree.insert(avltree.root_, 2);

    avltree.display(avltree.root_, 1);

    avltree.pre_order(avltree.root_); std::cout << std::endl;
    avltree.in_order(avltree.root_); std::cout << std::endl;
    avltree.post_order(avltree.root_); std::cout << std::endl;

    avltree.root_ = avltree.remove(avltree.root_, 5);
    avltree.root_ = avltree.remove(avltree.root_, 8);

    avltree.display(avltree.root_, 1);

    return 0;
}
