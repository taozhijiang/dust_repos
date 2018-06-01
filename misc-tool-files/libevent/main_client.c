#include <stdio.h>
#include <signal.h>
#include <string.h>

#include <sys/socket.h>

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/util.h>

#include "st_others.h"

void timer_cb (evutil_socket_t fd, short what, void *arg);
void sigusr1_cb (evutil_socket_t fd, short what, void *arg);
void bufferevent_cb(struct bufferevent *bev, short events, void *ptr);
void bufferread_cb(struct bufferevent *bev, void *ptr);
void bufferwrite_cb(struct bufferevent *bev, void *ptr);

int main(int argc, char* argv[])
{
    struct event_config *cfg;
    struct event_base *base;


    /*带配置产生event_base对象*/
    cfg = event_config_new();
    event_config_avoid_method(cfg, "select");   //避免使用select
    event_config_require_features(cfg, EV_FEATURE_ET);  //使用边沿触发类型
    //event_config_set_flag(cfg, EVENT_BASE_FLAG_PRECISE_TIMER);
    //event_base_new(void); 为根据系统选择最快最合适的类型
    base = event_base_new_with_config(cfg);
    event_config_free(cfg);

    st_d_print("Current Using: %s", event_base_get_method(base)); // epoll

    // priority 0, and priority 1 ->　lower
    event_base_priority_init(base, 2);

    /**
     * 建立一个套接字侦听 
     */
    struct bufferevent *bev;
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    inet_aton("192.168.1.161", &sin.sin_addr.s_addr);
    sin.sin_port = htons(8080); /* Port 8080 */

    //这里既可以显式传递socket，也可以传递-1然后面bufferevent_socket_connect自动创建
    //int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    //evutil_make_listen_socket_reuseable(sockfd);
    //evutil_make_socket_nonblocking(sockfd);
    //bev = bufferevent_socket_new(base, sockfd, BEV_OPT_CLOSE_ON_FREE);
    bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    //这里将ev_base作为参数，注意read/write/event所有事件回调函数都是用同一个参数
    //发现bufferevent_get_base这个函数接口，是不是这个就不用传递了
    bufferevent_setcb(bev, bufferread_cb, bufferwrite_cb, bufferevent_cb, base); 
    bufferevent_enable(bev, EV_READ|EV_WRITE); 

    // 如果在建立bufferevent_socket_new的时候没有传递socket，那么这里的连接函数
    // 会在底层创建一个新的socket
    if (bufferevent_socket_connect(bev, (struct sockaddr *)&sin, sizeof(sin)) < 0) 
    {
        /* Error starting connection */
        bufferevent_free(bev);
        return -1;
    }

    /**
     * 建立一个信号侦听 
     */
    struct event *ev_signal;
    //evsignal_new(base, signum, cb, arg)为简洁版本
    ev_signal = event_new(base, SIGUSR1, EV_SIGNAL|EV_PERSIST, sigusr1_cb, NULL);
    //将事件变为pending
    event_add(ev_signal, NULL);


	/**
     * 建立一个定时事件
     */
    struct event *ev_timer;
	struct timeval one_sec = { 1, 0 }; //1s
    int n_calls = 0;
    //evtimer_new(base, callback, arg)为简洁版本
    //EV_TIMEOUT的参数实际是可被忽略的，不传递也可以
	ev_timer = event_new(base, -1, EV_PERSIST, timer_cb, &one_sec);
    event_add(ev_timer, &one_sec);

    //event_base_dispatch(base); //进入事件循环直到没有pending的事件就返回
	//EVLOOP_ONCE　阻塞直到有event激活，执行回调函数后返回
	//EVLOOP_NONBLOCK 非阻塞类型，立即检查event激活，如果有运行最高优先级的那一类，完毕后退出循环
    event_base_loop(base, 0);

    event_free(ev_timer);
    event_free(ev_signal);
    event_base_free(base);

    st_d_print("Program terminated!");
    return 0;
}


void timer_cb(evutil_socket_t fd, short what, void *arg)
{
    int* i_ptr = (int *)arg;

    st_d_print("cb_func called %d times so far.", *i_ptr);
    *i_ptr += 1;
}

// 使用kill -SIGUSR1 PID　发送信号
void sigusr1_cb(evutil_socket_t fd, short what, void *arg)
{
    st_d_print("Signal Hander Called!");
}

void bufferevent_cb(struct bufferevent *bev, short events, void *ptr)
{
    struct event_base *base = ptr;

    //只有使用bufferevent_socket_connect进行的连接才会得到CONNECTED的事件
    if (events & BEV_EVENT_CONNECTED) {
        st_d_print("Got BEV_EVENT_CONNECTED event! ");
    } else if (events & BEV_EVENT_ERROR) {
        st_d_print("Got BEV_EVENT_ERROR event! ");
        bufferevent_free(bev);
        event_base_loopexit(base, NULL);
    } else if (events & BEV_EVENT_READING) {
        st_d_print("Got BEV_EVENT_READING event! ");
    } else if (events & BEV_EVENT_WRITING) {
        st_d_print("Got BEV_EVENT_WRITING event! ");
    }
}

// remove method
//bufferevent_read bufferevent_read_buffer
void bufferread_cb(struct bufferevent *bev, void *ptr)
{
    struct evbuffer *input = bufferevent_get_input(bev);
}

// add method
//bufferevent_write bufferevent_write_buffer
void bufferwrite_cb(struct bufferevent *bev, void *ptr)
{
    struct evbuffer *output = bufferevent_get_output(bev);
}

