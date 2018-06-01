#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

// netstat -n | awk '/^tcp/ {++S[$NF]} END {for(a in S) print a, S[a]}'

int main(int argc, char *argv[])
{
    int sockfd;
    int n;
    struct sockaddr_in serv_addr;
    char buffer[512];
	unsigned long socket_cnt = 0;
 
	while(socket_cnt < 1020)
	{
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
		{
			fprintf(stderr,"opening socket error! \n");
			exit(-1);
		}
	
		bzero(&serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(7899);
		serv_addr.sin_addr.s_addr = inet_addr("192.168.200.128");
	 
		if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		{
			fprintf(stderr,"connecting error! \n");
			exit(-1);
		}
		
		++ socket_cnt;
		fprintf(stderr,"Current Connectiong Number:%lu\n", socket_cnt);
		
		sprintf(buffer,"The Welcome Message from %d connectiong!£¡\n", sockfd);
		n = write(sockfd, buffer, strlen(buffer));
		if (n < 0)
			fprintf(stderr,"error writing to socket! \n");
        else
            fprintf(stderr,"Sending:%s OK!\n", buffer);
		usleep(500*000);
	}
	while(1)
		sleep(1);

 
	return 0;
}
