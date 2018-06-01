/**
 * 使用epoll进行大流量服务器设计的实现
 */

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <string.h>

#include <errno.h>
#include <stdio.h>
#include "st_interface.h"

int st_buildsocket(int listen_cnt, int port)
{
    int lsocket;

    lsocket = socket(AF_INET, SOCK_STREAM, 0);
    if (lsocket == -1)
    {
        st_print("Create Socket Error!\n");
        return -1;
    }

	int flag = 1;
	if( setsockopt(lsocket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1 )
	{
		st_print("setsockopt Error!\n");
        return -1;
	}

	struct sockaddr_in svraddr;
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(port);
	svraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(lsocket, (struct sockaddr *)&svraddr, sizeof(struct sockaddr_in)))
	{
		st_print("Socket Bind Error!\n");
        close(lsocket);
		return -1;
	}


	if(listen(lsocket, listen_cnt))
	{
		st_print("Socket Set Listen Error!\n");
        close(lsocket);
		return -1;
	}

	return lsocket;
}

int st_make_nonblock(int lsocket)
{
    int flags = 0;

    flags = fcntl (lsocket, F_GETFL, 0);
	flags |= O_NONBLOCK;
    fcntl (lsocket, F_SETFL, flags);

    return 0;
}

int st_add_new_event(int accepted_socket, P_EPOLL_STRUCT p_epoll)
{
    struct epoll_event new_event;

    int flags = 0;
    if( ! p_epoll)
        return -1;

    st_make_nonblock(accepted_socket);

    new_event.data.fd = accepted_socket;
    new_event.events = EPOLLIN | EPOLLET;

    return epoll_ctl (p_epoll->event_fd, EPOLL_CTL_ADD, accepted_socket, &new_event);
}

P_EPOLL_STRUCT st_make_events(int lsocket, size_t maxepoll_size)
{
    P_EPOLL_STRUCT p_epoll = 
        (P_EPOLL_STRUCT)malloc(sizeof(EPOLL_STRUCT));

    if (! p_epoll)
        return NULL;

    p_epoll->p_events = 
        (struct epoll_event*)calloc (maxepoll_size, sizeof(struct epoll_event));

    if (! p_epoll->p_events)
    {
        free(p_epoll);
        return NULL;
    }
	
    memset(p_epoll->p_events, 0, maxepoll_size*sizeof(struct epoll_event));
    p_epoll->max_events = maxepoll_size;
	p_epoll->event_fd = epoll_create (maxepoll_size);
	p_epoll->event.data.fd = lsocket;
    p_epoll->event.events = EPOLLIN | EPOLLET;

    if(epoll_ctl(p_epoll->event_fd, EPOLL_CTL_ADD, lsocket, &p_epoll->event))
    {
        free(p_epoll->p_events);
        free(p_epoll);
        return NULL;
    }

    return p_epoll;
}

void st_event_loop(P_EPOLL_STRUCT p_epoll, P_ST_THREAD_MANAGE p_manage, void* handler(void* data))
{
    if (!p_epoll)
        return;
    struct epoll_event* p_events = p_epoll->p_events;
    int listen_socket = p_epoll->event.data.fd;

    int e_i = 0;
    int ready = 0;

    for ( ; ; )
    {
        ready = epoll_wait(p_epoll->event_fd, p_events, p_epoll->max_events, -1); 
        for (e_i = 0; e_i < ready; e_i++)
        {
			if( (p_epoll->p_events[e_i].events & EPOLLERR) )			
            {
                st_d_print("Epoll Error!\n");
                close(p_epoll->p_events[e_i].data.fd);
                continue;
            }
			
			if ( ( p_epoll->p_events[e_i].events & EPOLLHUP ) ||
				 ( p_epoll->p_events[e_i].events & EPOLLRDHUP) )
			{
				st_d_print("Remote Disconnected!\n");
                close(p_epoll->p_events[e_i].data.fd);
				continue;
			}
            
            if (listen_socket == p_events[e_i].data.fd)
            {
                 /* 新链接到了（可能会有多个连接同时到来） */
                while (1)
                {
                    struct sockaddr in_addr;
                    socklen_t in_len;
                    int infd;
                    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

                    in_len = sizeof(struct sockaddr);
                    infd = accept (listen_socket, &in_addr, &in_len); //非阻塞的Socket
                    if (infd == -1)
                        break;

                    int sk = getnameinfo (&in_addr, in_len,
                                       hbuf, sizeof(hbuf),
                                       sbuf, sizeof(sbuf),
                                       NI_NUMERICHOST | NI_NUMERICSERV);
                    if (sk == 0)
                    {
                        st_print("Accept NEW Connect:[%d} "
                                 "(host=%s, port=%s)\n", infd, hbuf, sbuf);
                    }

                    if (st_add_new_event(infd, p_epoll))
                    {
                        st_print("Add socket:%d to event error！\n", infd);
                        break;
                    }

                    continue;       //可能有多个连接
                }
            }
            else
            {
                //有侦听的套接字发送数据的请求，添加你的处理
                if (p_manage && handler)
                {
                    st_threadpool_push_task(p_manage, handler, (void *)p_events[e_i].data.fd);  //传值调用简单些！
                }
#if 0
                while (1)
                {
                    ssize_t count;
                    char buf[512];

                    count = read (p_events[e_i].data.fd, buf, sizeof(buf));
                    if (count == -1)
                    {
                        /* If errno == EAGAIN, that means we have read all
                         data. So go back to the main loop. */
                        if (errno != EAGAIN)
                        {
                            perror ("read");
                            done = 1;
                        }

                        break;
                    }
                    else if (count == 0)
                    {
                      /* End of file. The remote has closed the
                         connection. */
                      done = 1;
                      break;
                    }

                    /* Write the buffer to standard output */
                    int s = write (1, buf, count);
                    if (s == -1)
                    {
                        perror ("write");
                        abort ();
                    }
                }

                if (done)
                {
                    printf ("Closed connection on descriptor %d\n",
                          p_events[e_i].data.fd);

                    /* Closing the descriptor will make epoll remove it
                     from the set of descriptors which are monitored. */
                    close (p_events[e_i].data.fd);
                }
#endif
            }
        }
    }
}




static void* response_func(void* data)
{
    ssize_t count;
	ssize_t nBytes;
    char buf[512];
    int done = 0;

    int socket = (int)data;
    //st_print("Function Called with %d under %ul \n", num, pthread_self());
    
	nBytes = 0;
	while (TRUE)
	{
		//接收数据
        count = recv (socket, buf + nBytes, 512, 0);
            
        if (count < 0)
        {   
            //数据读完了
            if ( count == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                // 由于此处的套接字已经是非阻塞的了，怎么处理后面再定
                break;
            }
            else
            {
                st_print("RECV ERROR for socket:%d\n", socket); 
                return NULL;
            }
        }
        else if (count == 0)    //stream socket peer has performed an orderly shutdown
        {
            //对端已经关闭了，这里也关闭
			st_print("Peer Close the Connection, close it!\n");
			close(socket);	// this will reclaim socket	
			return NULL;
        } 
        else 
        {
            nBytes += count;
            continue; // to read
        }
    }
	
	// Data Ready!!!!

    return NULL;
}

void st_epoll_test(void)
{
    int ret = 0;
    int lsocket = 0;

    ST_THREAD_MANAGE st_manage;
    P_ST_THREAD_MANAGE p_manage = &st_manage;

    if ( !st_threadpool_init(p_manage, 5))
    {
        st_print("st_threadpool_init FAILED!\n");
        return;
    }

    P_EPOLL_STRUCT p_epoll = NULL;

    lsocket = st_buildsocket(10, 7899);
    st_make_nonblock(lsocket);

    if (lsocket == -1)
    {
        st_print("st_buildsocket FAILED!\n");
        return;
    }

    p_epoll = st_make_events(lsocket, 32);
    if (!p_epoll)
    {
        st_print("st_make_events FAILED!\n");
        return;
    }

    st_event_loop(p_epoll, p_manage, response_func);

}


