#include <iostream>
#include <mutex>

#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;

#include "lockless.hpp"
	
const size_t max_times = 10000000L;
volatile size_t current = 0L;
volatile size_t should_protect = 0L;

Lockless<int> ll = Lockless<int>(200);

int main(int argc, char* argv[])
{
	boost::thread_group t_threads;
	std::cout << "Start freelock test:" <<	std::endl;
		
	ptime start(second_clock::local_time());
	
	
	for(size_t i=0; i<16; ++i)
		t_threads.create_thread(
            [&]{
            std::cout << "producer tid: " <<boost::this_thread::get_id() << std::endl;
            while(true) {
				ll.push(should_protect);
				++ should_protect;		
                	              	
                if(++ current > max_times)
                	break; 
            }
        });
		
	for(size_t i=0; i<16; ++i)
		t_threads.create_thread(
            [&]{
            std::cout << "customer tid: " <<boost::this_thread::get_id() << std::endl;
            size_t dummy;
            
            while(true) {
            	          	      		
            	dummy = ll.pop();
            	-- should_protect;	
                	
                if(++ current > max_times)
                	break; 
            }
        });
        
    t_threads.join_all();
    
    ptime end(second_clock::local_time());
    time_duration diff = end - start;
    
    std::cout << "with max_times: " << max_times << std::endl;	
    std::cout << "cost time: " << to_simple_string(diff) << std::endl;
    	
}