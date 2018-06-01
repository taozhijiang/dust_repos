#ifndef _AVL_TREE_H
#define _AVL_TREE_H

#include <cstddef>
#include <sys/types.h>

#include <iostream>
#include <memory>
#include <cassert>

class AvlNode;
class AvlTree;
using SmartAvlNodePtr = std::shared_ptr<AvlNode>;

class AvlNode {
private:
    int data_;
    SmartAvlNodePtr left_;
    SmartAvlNodePtr right_;
    // 从叶子向根节点计数的高度，从1开始计数
    size_t height_;  // Store height instead of traditional balance factor

public:
    explicit AvlNode(int data):
    data_(data), height_(1) {}

    ~AvlNode() { std::cout << "Destructing:" << data_ << std::endl; }

    friend class AvlTree;
};


class AvlTree {
public:
    SmartAvlNodePtr root_;
    AvlTree() = default;

    size_t get_height(SmartAvlNodePtr node) 
    { 
        return node ? node->height_ : 0; 
    }

    ssize_t get_bal_factor(SmartAvlNodePtr node) 
    { 
        return get_height(node->right_) - get_height(node->left_);
    }

    void fix_height(SmartAvlNodePtr node)
    {
        size_t high = get_height(node->left_) > get_height(node->right_) ?
                        get_height(node->left_) : get_height(node->right_);
        node->height_ = high + 1;
    }

    // 约定旋转后都是返回旋转后的跟节点
    SmartAvlNodePtr rotate_left(SmartAvlNodePtr node)
    {
        SmartAvlNodePtr tmp_root = node->right_;
        node->right_ = tmp_root->left_;
        tmp_root->left_ = node;
        
        fix_height(node);
        fix_height(tmp_root);
        return tmp_root; 
    }

    SmartAvlNodePtr rotate_right(SmartAvlNodePtr node)
    {
        SmartAvlNodePtr tmp_root = node->left_;
        node->left_ = tmp_root->right_;
        tmp_root->right_ = node;

        fix_height(node);
        fix_height(tmp_root);
        return tmp_root;
    }

    SmartAvlNodePtr rebalance(SmartAvlNodePtr node)
    {
        fix_height(node);

        int bal_factor = get_bal_factor(node);

        if (bal_factor > 1) 
        {
            if (get_bal_factor(node->right_) < 0) //rotate right first
                node->right_ = rotate_right(node->right_);

            return rotate_left(node);
        }
        else if (bal_factor < -1)
        {
            if (get_bal_factor(node->left_) > 0) //rotate right first
                node->left_ = rotate_left(node->left_);

            return rotate_right(node);
        }

        return node;
    }

    SmartAvlNodePtr insert(SmartAvlNodePtr node, int data)
    {
        if (!node)
        {
            node = std::make_shared<AvlNode>(data);
            return node;
        }

        assert(data != node->data_);

        if (data < node->data_) 
        {
            node->left_ = insert(node->left_, data);
            node = rebalance(node);
        }
        else if (data > node->data_)
        {
            node->right_ = insert(node->right_, data);
            node = rebalance(node);
        }

        return node;
    }


    /**
     * replace with largest from left subtree, or smallest from 
     * right subtree 
     *  
     * Because of recursive method, this work should break into 2 
     * methods 
     */
    SmartAvlNodePtr find_right_min(SmartAvlNodePtr node) 
    {
        return node->left_ ? find_right_min(node->left_) : node;
    }

    SmartAvlNodePtr remove_right_min(SmartAvlNodePtr node) 
    {
        if (!node->left_)
            return node->right_;

        node->left_ = remove_right_min(node->left_);
        return rebalance(node);
    }

    SmartAvlNodePtr remove(SmartAvlNodePtr node, int data)
    {
        assert(node);

        if (data < node->data_)
            node->left_ = remove(node->left_, data); 
        else if (data > node->data_)
            node->right_ = remove(node->right_, data);
        else
        {
            // if no right subtree, just return left subtree to replace it
            if (!node->right_)
                return node->left_;

            SmartAvlNodePtr repl = find_right_min(node->right_);

            // using repl replace node's place with its own data
            repl->right_ = remove_right_min(node->right_);
            repl->left_  = node->left_;
            return rebalance(repl);
        }
        return rebalance(node);
    }

    void display(SmartAvlNodePtr node, int level)
    {
        if (!node)
            return;
        
        display(node->right_, level + 1);
        std::cout << std::endl;

        if (node == root_)
            std::cout << "Root -> ";

        for (size_t i = 0; i < level && node != root_; i++)
            std::cout << "        " ;

        std::cout << node->data_;
        display(node->left_, level + 1);
    }

    void pre_order(SmartAvlNodePtr node)
    {
        if (!node) 
            return;

        std::cout<<node->data_<<", ";
        pre_order(node->left_);
        pre_order(node->right_);
    }

    void in_order(SmartAvlNodePtr node)
    {
        if (!node) 
            return;

        in_order(node->left_);
        std::cout<<node->data_<<", ";
        in_order(node->right_);
    }

    void post_order(SmartAvlNodePtr node)
    {
        if (!node) 
            return;

        post_order(node->left_);
        post_order(node->right_);
        std::cout<<node->data_<<", ";
    }
};

#endif //_AVL_TREE_H
