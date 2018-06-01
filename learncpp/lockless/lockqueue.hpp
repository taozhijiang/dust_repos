#ifndef _LOCKQUEUE_HPP_
#define _LOCKQUEUE_HPP_

#include <memory>
#include <vector>
#include <condition_variable>
#include <mutex>


template <typename T>
class LockQueue
{
typedef volatile size_t atomic_t;

public:
	explicit LockQueue(size_t sz):
		cond_empty_(),
		cond_overflow_(),
		mtx_(),
		data_()
	{
		++ sz;  //head tail会空出一个空位
		size_t align_size = 1;
		if(sz < 2)
			abort();
			
		while(align_size < sz)
			align_size <<= 1;
			
		count_ = align_size;
		mask_  = align_size - 1;
		data_.resize(count_);
		
		head_ =  0;
		tail_ =  0;
		
		std::cout << "count: " << count_ << ", mask: "  << mask_ << std::endl; 
			
	}
	
	void push(const T& item)
	{
		std::unique_lock<std::mutex> lock(mtx_);
		cond_overflow_.wait(lock, [this]() {
					return (next(head_) != tail_) ;
				});
			
		data_[head_] = item;
		head_ = next(head_);	
		cond_empty_.notify_one();
		return;
	}
	
	T pop()
	{
		T t;
		std::unique_lock<std::mutex> lock(mtx_);
		cond_empty_.wait(lock, [this]() {
					return (tail_ != head_) ;
				});

		t = data_[tail_];
		tail_ = next(tail_);
		cond_overflow_.notify_one();
		
		return t;
	}

	~LockQueue() {}

private:
	atomic_t next(const atomic_t& curr)
	{
		return ((curr + 1) & mask_);
	}
	
	std::vector<T> data_;
	size_t		   count_;
	size_t         mask_;
	
	atomic_t head_;
	atomic_t tail_;	
	
	std::condition_variable	cond_empty_;
	std::condition_variable	cond_overflow_;
	std::mutex				mtx_;
};

#endif  // _LOCKQUEUE_HPP_