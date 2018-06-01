#ifndef _BB_TREE_H
#define _RB_TREE_H

#include <cstddef>
#include <sys/types.h>

#include <iostream>
#include <memory>
#include <cassert>

class RBNode;
class RBTree;
using SmartNodePtr = std::shared_ptr<RBNode>;

enum Color { BLACK = false, RED = true};
class RBNode {
private:
    int data_;
    SmartNodePtr left_;
    SmartNodePtr right_;
    SmartNodePtr parent_;
    enum Color color_;

public:
    explicit RBNode(int data):
    data_(data), color_(RED) {}

    ~RBNode() {
        std::cout << "Destructing:" << data_
        << ", Color:" << (color_ == BLACK ? "Black" : "Red ") << std::endl;
    }

    SmartNodePtr get_grandparent()
    {
        if (!parent_)
            return SmartNodePtr();
        return parent_->parent_;
    }

    SmartNodePtr get_uncle()
    {
        if (!get_grandparent())
            return SmartNodePtr();

        if (get_grandparent()->left_ == parent_)
            return get_grandparent()->right_;
        else
            return get_grandparent()->left_;
    }

    SmartNodePtr get_sibling()
    {
        if (!parent_)
            return SmartNodePtr();

        if (parent_->left_.get() == this)
            return parent_->right_;
        else
            return parent_->left_;
    }

    friend class RBTree;
};


class RBTree {
public:
    SmartNodePtr root_;
    SmartNodePtr nill_; // shared by all
    RBTree()
    {
        nill_ = std::make_shared<RBNode>(-1);
        nill_->color_ = BLACK;
        nill_->left_ = nill_->right_ = nill_->parent_ = SmartNodePtr();
    }

    void remove(int data)
    {
        if (! root_)
            return;

        return remove(root_, data);
    }

    void remove(SmartNodePtr node, int data)
    {
        if (data < node->data_)
        {
            if (node->left_ == nill_) //未找到
                return;
            return remove(node->left_, data);
        }
        else if (data > node->data_)
        {
            if (node->right_ == nill_)
                return;
            return remove(node->right_, data);
        }
        else
        {
            if (node->right_ == nill_)
                return remove(node);

            SmartNodePtr tmp = find_right_min(node->right_);
            std::swap(node->data_, tmp->data_);
            return remove(tmp);
        }
    }

    SmartNodePtr find_right_min(SmartNodePtr node)
    {
        assert(node && node != nill_);

        if (node->left_ == nill_)
            return node;
        return find_right_min(node->left_);
    }

    // 这里的remove节点，已经是确认要删除的节点了(都至多只有一个非nill_孩子的情况)
    // 比如交换后得到的右子树最小节点
    void remove(SmartNodePtr node)
    {
        // 可能的非空子树，当然也可能node不包含非nill_儿子
        SmartNodePtr child = node->left_ == nill_ ? node->right_ : node->left_;

        // 只有一个根节点而且没有非nill_孩子的时候，直接重新初始化根节点为空
        if (! node->parent_
            && node->left_ == nill_ && node->right_ == nill_ )
        {
            node.reset();
            root_.reset();
            return;
        }

        // 否则说明有非nill_的根节点要删除，直接将非nill_孩子顶替上来成为根节点
        if (! node->parent_)
        {
            node.reset();
            child->parent_ = SmartNodePtr();
            root_ = child;
            root_->color_ = BLACK;
            return;
        }

        // 使用单节点的孩子来顶替父节点的位置，父节点的引用计数自动降低
        if (node->parent_->left_ == node)
            node->parent_->left_ = child;
        else
            node->parent_->right_ = child;
        child->parent_ = node->parent_;

        // 如果原先被是删除的红色节点，表明被删除节点的父亲和孩子都是黑色的，
        // 最主要的是不影响RB树的规则，可以直接返回
        if (node->color_ == RED)
            return;

        // node->color_ == BLACK

         // 用孩子顶替后，如果孩子是红色，就把孩子重新作色为黑色，
         // 这样原来通过黑色删除节点的路线现在都由孩子去作为黑色节点顶替，
         // 原先二叉树的性质没有被破坏，否则就是double-black了
        if (child->color_ == RED)
            child->color_ = BLACK;
        else
            do_post_remove(child);
    }

    void do_post_remove(SmartNodePtr node)
    {

        // Precondition:
        // 此处的node是顶替上来的child节点了，原先的节点已经被删除了，此处主要做
        // 调整方面的工作
        // node->color_ == BLACK
        // 
        // Node Parent GrandParent Sibling Silbing_l Silbing_right
        // 
        // 下面的旋转和作色，不仅要看局部树到各个叶子的黑色节点数目是否相同，还要
        // 整体看调整后整体的数目有没有增多或者减少

        if (! node->parent_)
        {
            node->color_ = BLACK;
            return;
        }

        // 维基百科的case分类比较的乱，这里重新整理一下

        // 1. 如果Silbing是红色
        // 那么Parent肯定是黑色，重绘Parent成红，Silbling成黑色；
        // 如果N是左儿子，则进行左旋操作，否则进行右旋操作
        // 
        // 经过这一步，N有了一个黑色的兄弟和一个红色的父亲，但是N和Silbing原先的儿子
        // 现在的兄弟黑色节点的数目肯定不一样，所以还需要继续处理
        if (node->get_sibling()->color_ == RED)
        {
            // S是红色，所以node->parent_肯定是黑色
            assert(node->parent_->color_ == BLACK);

            node->parent_->color_ = RED;
            node->get_sibling()->color_ = BLACK;

            if (node == node->parent_->left_)
                rotate_left(node->get_sibling());
            else
                rotate_right(node->get_sibling());
        }

        // 2. 如果Sibling原先就是黑色的或者上面处理成黑色的了
        // 2.a 如果Silbing的两个孩子都是黑色的
        if ( node->get_sibling()->left_->color_ == BLACK
            && node->get_sibling()->right_->color_ == BLACK)
        {
            // X和Silbing都是黑色，所以N和Silbing的儿子黑色节点一样
            assert(node->get_sibling()->color_ == BLACK);

            // S和S的左右儿子都是黑色，但是N的父亲是红色
            // 此时反转S和parent_的颜色，删除的X的黑色由Parent补偿回来了，而Sibling没有改变
            // 已经是平衡状态了
            if (node->parent_->color_ == RED)
            {
                node->get_sibling()->color_ = RED;
                node->parent_->color_ = BLACK;
                return;
            }
            // 修改Silbing为红色，虽然当前路子数是平衡的，但是整个子树减少了一个黑色节点，
            // 为了和其他树平衡，需要递归处理父节点
            else
            {
                node->get_sibling()->color_ = RED;
                do_post_remove(node->parent_);
            }
        }
        // 2.b 如果Silbing的两个孩子至少有一个是红色
        else
        {
            // 上面经过多次旋转，此时Parent的颜色不定了
            // 因为Sibling的孩子有红色，所以此时Sibling一定是黑色
            assert(node->get_sibling()->color_ == BLACK);

            // 此时Sibling都是黑色的，这里其实跟刚开始插入的情况比较类似，
            // 当Sibling->r_child和Parnet-Sibling-r_child不在一条线上面，需要先旋转
            // 成一条线，然后做颜色的修正，否则就跳过这个步骤   
            if (node == node->parent_->left_
                && node->get_sibling()->left_->color_ == RED
                && node->get_sibling()->right_->color_ == BLACK)
            {
                // 经过这个旋转后子树是满足二叉树性质的，但是node和新的Sibling不平衡，
                // 这个操作不会涉及到node和Parent，而且这个不平衡会fall through到下面处理
                node->get_sibling()->color_ = RED;
                node->get_sibling()->left_->color_ = BLACK;
                rotate_right(node->get_sibling()->left_);
            }
            else if (node == node->parent_->right_
                && node->get_sibling()->left_->color_ == BLACK
                && node->get_sibling()->right_->color_ == RED)
            {
                node->get_sibling()->color_ = RED;
                node->get_sibling()->right_->color_ = BLACK;
                rotate_right(node->get_sibling()->right_);
            }

            // 上面经过处理也是到达这个步骤，此时Sibling是黑色，且依次挂了红色、黑色一条线的右
            // 子树或者左子树，通过Parent进行旋转，让原来的Sibling代替Parent的颜色，同时修改
            // 原先Parent(成黑色)和Sibling孩子(成黑色)
            // 由于原先的Parent和现在的Sibling颜色是不确定的，无非做两种情况讨论：
            // (1) Parent原先是黑色的
            // (2) Parent原先是红色的
            // 看图都可以分析出，修改之后这条子树到子叶的黑色节点都是2，满足条件
            node->get_sibling()->color_ = node->parent_->color_;
            node->parent_->color_ = BLACK;
            if (node == node->parent_->left_)
            {
                node->get_sibling()->right_->color_ = BLACK;
                rotate_left(node->get_sibling());
            }
            else
            {
                node->get_sibling()->left_->color_ = BLACK;
                rotate_right(node->get_sibling());
            }
        }
    }

    void insert(int data)
    {
        if (! root_)
        {
            root_ = std::make_shared<RBNode>(data);
            root_->color_ = BLACK;
            root_->parent_ = SmartNodePtr();
            root_->left_ = root_->right_ = nill_;
        }
        else
            insert(root_, data);
    }

    void insert(SmartNodePtr node, int data)
    {
        assert(root_);
        assert(data != node->data_);

        if (data < node->data_)
        {
            if (node->left_ != nill_)
                insert(node->left_, data);
            else
            {
                SmartNodePtr tmp = std::make_shared<RBNode>(data);
                tmp->left_ = tmp->right_ = nill_;
                tmp->parent_ = node;
                node->left_ = tmp;
                do_post_insert(tmp);
            }
        }
        else
        {
            if (node->right_ != nill_)
                insert(node->right_, data);
            else
            {
                SmartNodePtr tmp = std::make_shared<RBNode>(data);
                tmp->left_ = tmp->right_ = nill_;
                tmp->parent_ = node;
                node->right_ = tmp;
                do_post_insert(tmp);
            }
        }
    }

    void do_post_insert(SmartNodePtr node)
    {
        assert(node != nill_);

        if (! node->parent_)
        {
            root_ = node;
            node->color_ = BLACK;
            return;
        }

        // 在黑节点下面插入红节点，总是平衡的
        if (node->parent_->color_ == BLACK)
            return;

        // case 父节点和叔父节点都是RED
        // 将它们两个重绘为黑色并重绘祖父节点G为红色
        // 在祖父节点G上递归地进行情形1的整个过程
        if (node->get_uncle()->color_ == RED ) //总是返回有效指针，即使是nill_
        {
            node->parent_->color_ = BLACK;
            node->get_uncle()->color_ = BLACK;
            node->get_grandparent()->color_ = RED;
            return do_post_insert(node->get_grandparent());
        }

        //
        //叔父节点U是黑色或缺少(叶子节点)
        assert(node->parent_->color_ == RED && node->get_uncle()->color_ == BLACK);
        assert(node->get_grandparent()->color_ == BLACK);

        if (     node == node->parent_->left_ && node->parent_ == node->get_grandparent()->left_)
        {
            node->parent_->color_ = BLACK;
            node->get_grandparent()->color_ = RED;

            rotate_right(node->parent_);
        }
        else if (node == node->parent_->left_ && node->parent_ == node->get_grandparent()->right_)
        {
            node->get_grandparent()->color_ = RED;
            node->color_ = BLACK;

            rotate_right(node);
            rotate_left(node);
        }
        else if (node == node->parent_->right_ && node->parent_ == node->get_grandparent()->right_)
        {
            node->get_grandparent()->color_ = RED;
            node->parent_->color_ = BLACK;

            rotate_left(node->parent_);
        }
        else if (node == node->parent_->right_ && node->parent_ == node->get_grandparent()->left_)
        {
            node->get_grandparent()->color_ = RED;
            node->color_ = BLACK;

            rotate_left(node);
            rotate_right(node);
        }

    }

    // 约定旋转后都是返回旋转后的跟节点
    void rotate_left(SmartNodePtr node)
    {
        if (! node->parent_)
        {
            root_ = node;
            node->color_ = BLACK;
            return;
        }

        SmartNodePtr gp = node->get_grandparent();
        SmartNodePtr fa = node->parent_;
        SmartNodePtr yp = node->left_;

        fa->right_ = yp;
        if(yp != nill_)
            yp->parent_ = fa;

        node->left_ = fa;
        fa->parent_ = node;

        if(root_ == fa)
            root_ = node;

        node->parent_ = gp;

        if(gp)
        {
            if(gp->left_ == fa)
                gp->left_ = node;
            else
                gp->right_ = node;
        }
    }

    void rotate_right(SmartNodePtr node)
    {
        if (! node->parent_)
        {
            root_ = node;
            node->color_ = BLACK;
            return;
        }

        SmartNodePtr gp = node->get_grandparent();
        SmartNodePtr fa = node->parent_;
        SmartNodePtr yp = node->left_;

        fa->left_ = yp;
        if(yp != nill_)
            yp->parent_ = fa;

        node->right_ = fa;
        fa->parent_ = node;

        if(root_ == fa)
            root_ = node;

        node->parent_ = gp;

        if(gp)
        {
            if(gp->left_ == fa)
                gp->left_ = node;
            else
                gp->right_ = node;
        }
    }


    void display(SmartNodePtr node, int level)
    {
        if (!node)
            return;

        display(node->right_, level + 1);
        std::cout << std::endl;

        if (node == root_)
            std::cout << "Root -> ";

        for (size_t i = 0; i < level && node != root_; i++)
            std::cout << "        " ;

        std::cout << node->data_ << "[" << (node->color_ == BLACK ? "B" : "R ") << "]";
        display(node->left_, level + 1);
    }

    void pre_order(SmartNodePtr node)
    {
        if (!node)
            return;

        std::cout<<node->data_<<", ";
        pre_order(node->left_);
        pre_order(node->right_);
    }

    void in_order(SmartNodePtr node)
    {
        if (!node)
            return;

        in_order(node->left_);
        std::cout<<node->data_<<", ";
        in_order(node->right_);
    }

    void post_order(SmartNodePtr node)
    {
        if (!node)
            return;

        post_order(node->left_);
        post_order(node->right_);
        std::cout<<node->data_<<", ";
    }

};

#endif //_RB_TREE_H
