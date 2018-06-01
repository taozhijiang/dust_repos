#include <iostream>
#include <vector>
#include <list>
#include <algorithm>

#ifndef _SORT_HPP_
#define _SORT_HPP_

class Sort {
public:
    // 拷贝待排数据
    void sort_it(const std::string msg, const std::vector<int>& src, std::vector<int>& store) 
    {
        store = src;
        show_it(msg + " Before", store);
        do_sort(store);
        show_it(msg + " After", store);
    }

    virtual void do_sort(std::vector<int>& store) = 0;

    void show_it(const std::string& msg, const std::vector<int>& vec) 
    {
        std::cout << "[" << msg << "]\t";
        for (auto& i: vec)
            std::cout << i << ", ";
        std::cout << std::endl;
    }

    Sort() = default;
    virtual ~Sort() = default;
};

/**
 * 冒泡排序
 */
class BubbleSort: public Sort {
public:
    virtual void do_sort(std::vector<int>& store) override
    {
        const size_t sz = store.size();
        for (size_t i=0; i<sz; ++i)
            for (size_t j=i+1; j<sz; ++j)
                if (store[i] > store[j])
                    std::swap(store[i], store[j]);
    }
};

class BubbleSort2: public Sort {
public:
    virtual void do_sort(std::vector<int>& store) override
    {
        const size_t sz = store.size();
        for (size_t i=0; i<sz; ++i)
            for (size_t j=0; j<sz-1; ++j)
                if (store[j] > store[j+1])
                    std::swap(store[j], store[j+1]);
    }
};

/**
 * 直接插入排序
 */
class InsertSort: public Sort {
public:
    virtual void do_sort(std::vector<int>& store) override
    {
        std::list<int> tmp;
        auto it = tmp.begin();
        for (auto& i: store)
        {
            for (it = tmp.begin(); it!=tmp.end() && *it<i; ++it)
                continue;
            tmp.insert(it, i);
        }
        store.assign(tmp.cbegin(), tmp.cend());
    }
};

// 二分插入排序(折半查找插入点，此处需要随机访问容器)
// 模拟原始的array，不用标准容器
class InsertSort2: public Sort {
public:
    virtual void do_sort(std::vector<int>& store) override
    {
        const size_t sz = store.size();
        for (size_t i=1; i<sz; ++i)
        {
            int elem = store[i];
            int left = 0;
            int right = i-1;

            while (left <= right) //当left==right，也需要判断mid前还是后
            {
                ssize_t mid = (left + right) / 2; //向0取整
                if (elem > store[mid])
                    left = mid + 1; //必须+1 -1，否则相邻序死循环
                else
                    right = mid - 1;
            }

            for (size_t j=i; j>left; --j)
                store[j] = store[j-1];
            store[left] = elem;
        }
    }
};



/**
 * 希尔排序
 */
class ShellSort: public Sort {
public:
    virtual void do_sort(std::vector<int>& store) override
    {
        const size_t sz = store.size();
        size_t gap = sz >> 1;

        while (gap)
        {
            //所有间隔gap的元素为一组，第一个元素不排序，所以跳过gap
            for (size_t i=gap; i<sz; ++i) 
            {
                int elem = store[i];
                int j = i;

                while ( j>=gap && elem < store[j-gap] )
                {
                    store[j] = store[j-gap]; //移动gap
                    j -= gap;
                }
                store[j] = elem;
            }

            gap >>= 1;
        }
    }
};


/**
 * 选择排序
 */
class SelectSort: public Sort {
public:
    virtual void do_sort(std::vector<int>& store) override
    {
        const size_t sz = store.size();
        size_t index = 0;

        for (size_t i=0; i<sz; ++i)
        {
            index = i;
            for (size_t j=i+1; j<sz; ++j)
                if (store[j] < store[index])
                    index = j;
            
            if (i != index)
                std::swap(store[i], store[index]);
        }
    }
};


/**
 * 堆排序 
 *  
 * 完全二叉树，树中任意非叶节点的关键字均不小于其左右孩子节点的关键字(大顶堆) 
 * 对于父节点i，其左右孩子为2*i+1, 2*i+2 
 * 对于子节点i，其父节点为(i-1)/2 
 */
class HeapSort: public Sort {
public:
    void buildHeap(std::vector<int>& store, size_t curr/*父*/, size_t last/*尾，包含*/)
    {
        size_t child = 2*curr + 1; //左孩
        int elem = store[curr];

        while (child <= last)
        {
            //两个儿子中较大的
            if (child<last && store[child]<store[child+1]) 
                ++child;

            if (elem >= store[child]) 
                break;

            // 元素交换，同时递归到子节点，另外一个儿子不用管了
            store[curr] = store[child];
            curr = child;
            child = 2*curr + 1;    
        }

        store[curr] = elem;
    }

    virtual void do_sort(std::vector<int>& store) override
    {
        const size_t sz = store.size();

        // 首先建立堆
        for (int i=((sz-1)-1)/2; i>=0; --i)
            buildHeap(store, i, sz-1);

        for (int i=sz-1; i>0; --i)
        {
            std::swap(store[0], store[i]);
            buildHeap(store, 0, i-1);
        }

    }
};

/**
 * 鸡尾酒排序，Shaker排序，双向冒泡排序
 */
class ShakerSort: public Sort {
public:
    virtual void do_sort(std::vector<int>& store) override
    {
        const size_t sz = store.size();

        size_t left=0, right=sz-1;
        size_t i=0;

        while (left<right)
        {
            //第一遍，从左到右
            for (i=left; i<right; ++i)
            {
                if (store[i] > store[i+1])
                    std::swap(store[i], store[i+1]);
            }
            -- right;

            for (i=right; i>left; --i)
            {
                if (store[i] < store[i-1])
                    std::swap(store[i], store[i-1]);
            }
            ++ left;
        }
    }
};


/**
 * Quick排序 
 * 数据分区、递归求解 
 */
class QuickSort: public Sort {
public:
    void do_quick_sort(std::vector<int>& store, size_t left, size_t right)
    {
        size_t i=left, j=right;
        int pivot = store[i];

        while (i<j)
        {
            while (i<j && store[j] > pivot)
                --j;
            if (i<j)
                std::swap(store[i], store[j]); //pivot == store[j]

            while (i<j && store[i] < pivot)
                ++i;
            if (i<j)
                std::swap(store[i], store[j]); //pivot == store[i]
        }

        if (left != i)
            do_quick_sort(store, left, i - 1);

        if (right != j)
            do_quick_sort(store, j + 1, right);
    }

    virtual void do_sort(std::vector<int>& store) override
    {
        const size_t sz = store.size();
        if (sz <= 1)
            return;

        do_quick_sort(store, 0, sz-1);
    }
};


/**
 * 归并排序
 */
class MergeSort: public Sort {
public:
    void do_merge_sort(std::vector<int>& store,
                        size_t beg, size_t last)
    {
        if (beg < last)
        {
            size_t mid = (beg + last) / 2;
            do_merge_sort(store, beg, mid);
            do_merge_sort(store, mid+1, last);
            do_merge_merge(store, beg, last, mid);
        }
    }

    void do_merge_merge(std::vector<int>& store, 
                        size_t beg, size_t last, size_t mid/*included in first*/)
    {
        size_t index_1 = beg, index_2 = mid+1;
        size_t index_s = 0;
        std::vector<int> tmp_vec(last - beg + 1);

        while (index_1 <= mid || index_2 <= last )
        {
            if (index_1 > mid)
            {
                while (index_2 <= last)
                    tmp_vec[index_s ++] = store[index_2++];
            }
            else if (index_2 > last)
            {
                while (index_1 <= mid)
                    tmp_vec[index_s ++] = store[index_1++];
            }
            else
            {
                if (store[index_1] < store[index_2])
                    tmp_vec[index_s ++] = store[index_1++];
                else
                    tmp_vec[index_s ++] = store[index_2++];
            }
        }

        for (size_t i=0; i<tmp_vec.size(); ++i)
            store[beg+i] = tmp_vec[i];
    }

    virtual void do_sort(std::vector<int>& store) override
    {
        const size_t sz = store.size();
        do_merge_sort(store, 0, sz-1);
    }
};


/**
 * 归并排序
 */
class CountingSort: public Sort {
public:
    virtual void do_sort(std::vector<int>& store) override
    {
        auto it = std::max_element(store.cbegin(), store.cend());
        int max_item = *it;
        std::vector<int> result(store.size());

        std::vector<int> bucket(max_item + 1);
        for (auto& elem: store)
            bucket[elem] ++;

        // 关键，调整计数针对索引
        for (size_t i=1; i<bucket.size(); ++i)
            bucket[i] += bucket[i-1];

        //得到元素位置，保留结果
        for (auto& elem: store)
            result[bucket[elem] - 1] = elem;

        store = result;
    }
};


#endif
