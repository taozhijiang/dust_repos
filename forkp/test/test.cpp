#include <iostream>
#include <unistd.h>


int main(int argc, char* argv[])
{
    
	std::cerr << "Start test exec process !" << std::endl;
	std::size_t tick = 0;
	
	for(;;) {
		std::cerr << "exec print " << tick ++ << std::endl;
		::sleep(2);
	}

    return 0;
}

