#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <thread>
#include <cstring>
#include <atomic>

#include <vector>
#include <time.h>

#include <signal.h>

using std::string;

void usage() {
	std::cout << std::endl;
	std::cout << " This is http client benchmark tools " << std::endl;
	std::cout << " ./http_client_test -h 127.0.0.1 -p 7599 -c 30 -P -d \"data\" " << std::endl;
	std::cout << std::endl;
	
	return;
}

struct configure_t {
	struct sockaddr_in addr;
	std::size_t thread_num;
	bool   		post_methord;
	const char* data;
	time_t      start_tm;
	
	std::vector<std::size_t> success;
	std::vector<std::size_t> failed;
};

struct configure_t configure;
// 1 start   2 end
std::atomic<int> running_flag = ATOMIC_VAR_INIT(0); 

struct header
{
  std::string name;
  std::string value;
};
std::string header_name_value_separator_str = ": ";
std::string header_crlf_str = "\r\n";

std::string reply_generate(const char* data, size_t len){
	
	std::vector<header> headers(6);

	// reply fixed header
	headers[0].name = "Server";
	headers[0].value = "http_test/1.0";
	headers[1].name = "Date";
	time_t now = time(NULL);
	headers[1].value = ctime(&now);
	headers[2].name = "Content-Length";
	headers[2].value = std::to_string(static_cast<long long int>(len));
	headers[3].name = "Content-Type";
	headers[3].value = "text/html";
	headers[4].name = "Connection";
	headers[4].value = "keep-alive";
	headers[5].name = "Access-Control-Allow-Origin";
	headers[5].value = "*";

	string str = "HTTP/1.1 200 OK\r\n";
	for (size_t i=0; i< headers.size(); ++i)
	{
		str += headers[i].name;
		str += header_name_value_separator_str;
		str += headers[i].value;
		str += header_crlf_str;
	}

	str += header_crlf_str;
	str += data;

	return str;
}

void sig_handler(int) {
	
	running_flag = 2;
	std::size_t use_time = time(NULL) - configure.start_tm;
	
	std::size_t succ_cnt = 0;
	std::size_t fail_cnt = 0;
	for(std::size_t i=0; i<configure.success.size(); ++i) {
		succ_cnt += configure.success[i];
		fail_cnt += configure.failed[i];
	}
	
	std::cout << "USE TIME: " << use_time << "sec" << std::endl;
	std::cout << "SUCCESS: " << succ_cnt << std::endl;
	std::cout << "FAILURE: " << fail_cnt << std::endl;
	std::cout << "SUCCESS PERCENT: " << (100.0 * succ_cnt)/(succ_cnt + fail_cnt) << "%" << std::endl;
	std::cout << "SUCCESS TRANS RATE: " << double(succ_cnt)/use_time << " trans/sec" << std::endl;
	std::cout << "TOTAL TRANS RATE: " <<  double(succ_cnt + fail_cnt)/use_time << " trans/sec" << std::endl;
	
	exit(EXIT_SUCCESS);
}

void runTask(int id) {
	std::cout << "TID:" << id << " ";
	
	int sockfd;
	ssize_t n;
	
	std::string str = reply_generate(configure.data, strlen(configure.data));
	char buffer[1024];
	strcpy(buffer, str.c_str());
	std::size_t buffer_len = strlen(buffer);
	
	while(true) {
		
		if(running_flag == 0)
			continue;
		if(running_flag == 2)
			break;
		
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
		{
			std::cout << "opening socket error!" << std::endl;
			exit(EXIT_FAILURE);
		}
	 
		if (connect(sockfd, (struct sockaddr *) &configure.addr, sizeof(struct sockaddr_in)) < 0)
		{
			std::cout << "connecting error!" << std::endl;
			exit(EXIT_FAILURE);
		}
		
		n = write(sockfd, buffer, buffer_len);
		
		if (n < 0)
			++ configure.failed[id];
		else
			++ configure.success[id];
		
		close(sockfd);
	}
	
	std::cout << "Task Stopping ID:" << id << std::endl;
	
}


// ./http_client_test -h 127.0.0.1 -p 2335 -c 30 -P -d "data"
int main(int argc, char *argv[]) {
	
	int opt_g = 0;
	
	memset(&configure, 0, sizeof(struct configure_t));
	configure.addr.sin_family = AF_INET;
	configure.addr.sin_port = htons(7599);
	configure.addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	configure.thread_num =7;
	configure.post_methord = true;
	configure.data = "HTTP TEST INFO";
	
    while( (opt_g = getopt(argc, argv, "h:p:c:Pd:H")) != -1 )
    {
        switch(opt_g)
        {
			case 'h':
				configure.addr.sin_addr.s_addr = inet_addr(optarg);
				break;
			case 'p':
				configure.addr.sin_port = atoi(optarg);
				break;
            case 'c':
                configure.thread_num = atoi(optarg);
				if(!configure.thread_num){
					usage();
					exit(EXIT_SUCCESS);
				}
                break;
            case 'P':
                configure.post_methord = true;
                break;
            case 'd':
                configure.data = optarg;
                break;
            case 'H':
            default:
                usage();
                exit(EXIT_SUCCESS);
        }
    }

	std::vector<std::thread> threads;
	configure.success.resize(configure.thread_num);
	configure.failed.resize(configure.thread_num);
	
	for(std::size_t i=0; i<configure.thread_num; ++i) {
		threads.emplace_back(std::thread(runTask, i));
	}
	
	::signal(SIGINT, sig_handler);
	configure.start_tm = time(NULL);
	running_flag = 1;
	
	for(auto& it: threads)
		it.join();
	
	::sleep(1);
	
	return 0;
}
	
