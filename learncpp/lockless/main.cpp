#include <iostream>
#include "lockless.hpp"

int main(int argc, char* argv[])
{
	Lockless<int> ll = Lockless<int>(8);
	
	for(size_t i=0; i<8; ++i)
		ll.push(i+100);
	
	std::cout <<"PUSH OK" << std::endl;
	
	for(size_t i=0; i<8; ++i)
		std::cout << ll.pop() << std::endl;
		
	std::cout << "Hi!" << std::endl;
	return 0;
}