#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <pthread.h>
#include <cstdlib>

//#include "jnky.inc"
#include "swxa.inc"

volatile bool start = false;
volatile bool stop  = false;

time_t            start_time = 0;
volatile uint64_t count = 0;

static void sign_test() {
	
	std::stringstream ss;
	ss << "sign message with: " << ::random() << ", and " << ::random() << "include";
	std::string msg = ss.str();
	
	std::string sign = RawSignMsg(msg);
	if(sign.empty()) {
		fprintf(stderr, "sign error\n");
		::abort();
	}
}

static void sign_verify_test() {
	
	std::stringstream ss;
	ss << "sign message with: " << ::random() << ", and " << ::random() << "include";
	std::string msg = ss.str();
	
	std::string sign = RawSignMsg(msg);
	if(sign.empty()) {
		fprintf(stderr, "sign error\n");
		::abort();
	}
	
	if(RawVerifyMsg(msg, sign) != 0) {
		fprintf(stderr, "sign error\n");
		::abort();
	}
}

void* perf_run(void* x_void_ptr) {
	
    while(!start)
        ::usleep(1);

    while(!stop) {
        //sign_test();
		sign_verify_test();
        ++ count;
    }
}

int main(int argc, char* argv[]) {
	
	if (!init())
		::abort();

	pthread_t tids[5];
	
	for(int i=0; i<sizeof(tids)/sizeof(tids[0]); ++i) {
		pthread_create(&tids[i], NULL, perf_run, NULL);
		std::cout << "starting" << tids[i] << std::endl;
	}

    ::sleep(2);
    start_time = ::time(NULL);
    start = true;

    int ch = getchar();
    stop = true;
    time_t stop_time = ::time(NULL);

    uint64_t count_per_sec = count / ( stop_time - start_time);
    fprintf(stderr, "count %ld, time: %ld, perf: %ld tps\n", count, stop_time - start_time, count_per_sec);

	for(int i=0; i<sizeof(tids)/sizeof(tids[0]); ++i) {
		pthread_join(tids[i], NULL);
		std::cout << "joining" << tids[i] << std::endl;
	}

    return 0;
}
