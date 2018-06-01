#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/util.h>

#include "st_others.h"

/**
 * This program aim on the server side of libevent
 */

void timer_cb (evutil_socket_t fd, short what, void *arg);
void sigusr1_cb (evutil_socket_t fd, short what, void *arg);
void bufferevent_cb(struct bufferevent *bev, short events, void *ptr);
void bufferread_cb(struct bufferevent *bev, void *ptr);
void bufferwrite_cb(struct bufferevent *bev, void *ptr);
static void
accept_conn_cb(struct evconnlistener *listener,
    evutil_socket_t fd, struct sockaddr *address, int socklen,
    void *ctx);
static void
accept_error_cb(struct evconnlistener *listener, void *ctx);


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

    st_d_print("Current Using Method: %s", event_base_get_method(base)); // epoll

    // priority 0, and priority 1 ->　lower
    // 0: listen  1: read/write 2: other
    event_base_priority_init(base, 3);

    /**
     * 建立Listen侦听套接字
     */
    struct evconnlistener *listener;
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(0);
    sin.sin_port = htons(8080); /* Port 8080 */

    listener = evconnlistener_new_bind(base, accept_conn_cb, NULL,
            LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1/*backlog 无限制*/,
            (struct sockaddr*)&sin, sizeof(sin));
    if (!listener) {
            perror("Couldn't create listener");
            return 1;
    }
    evconnlistener_set_error_cb(listener, accept_error_cb);
   

    /**
     * 建立一个信号侦听 
     */
    struct event *ev_signal;
    //evsignal_new(base, signum, cb, arg)为简洁版本
    ev_signal = event_new(base, SIGUSR1, EV_SIGNAL|EV_PERSIST, sigusr1_cb, base);
    event_priority_set(ev_signal, 2);
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
    event_priority_set(ev_timer, 2);
    event_add(ev_timer, &one_sec);

    //其实，上面的signal和timer事件可以公用一个event，然后在处理的callback中，使用what来区分到底是
    //由于哪个事件activate了

    //event_base_dispatch(base); //进入事件循环直到没有pending的事件就返回
	//EVLOOP_ONCE　阻塞直到有event激活，执行回调函数后返回
	//EVLOOP_NONBLOCK 非阻塞类型，立即检查event激活，如果有运行最高优先级的那一类，完毕后退出循环


    event_base_loop(base, 0);

    event_free(ev_timer);
    event_free(ev_signal);

    evconnlistener_free(listener);
    // this function does not deallocate any of the events that are currently
    // associated with the event_base, or close any of their sockets, or free any of their pointers.
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

//events
// initialized  pending active  persistent 回调函数执行之后，non-pending

// 使用kill -SIGUSR1 PID　发送信号
void sigusr1_cb(evutil_socket_t fd, short what, void *arg)
{
    struct event_base *base = (struct event_base *)arg;

    st_d_print("Signal Hander Called for SIGUSR1!");

    FILE* fp = fopen("event_dump.txt","w");
    // DUMP events
    //user@gentoo ~ % cat Dropbox/ReadTheCode/demoproj/event_dump.txt
//Inserted events:
//  0x604550 [fd 6] Read Persist
//  0x6040a8 [fd 5] Read Persist
//  0x6046f0 [fd 10] Signal Persist
//  0x604cb0 [fd 8] Read Persist

    event_base_dump_events(base, fp);
    fflush(fp);
    close(fp);

    return;
}

void bufferevent_cb(struct bufferevent *bev, short events, void *ptr)
{
    struct event_base *base = bufferevent_get_base(bev);
    int loop_terminate = 0;

    //只有使用bufferevent_socket_connect进行的连接才会得到CONNECTED的事件
    if (events & BEV_EVENT_CONNECTED) {
        st_d_print("Got BEV_EVENT_CONNECTED event! ");
    } else if (events & BEV_EVENT_ERROR) {
        st_d_print("Got BEV_EVENT_ERROR event! ");
        loop_terminate = 1;
    } else if (events & BEV_EVENT_EOF) {
        st_d_print("Got BEV_EVENT_EOF event! ");
        bufferevent_free(bev);
    } else if (events & BEV_EVENT_TIMEOUT) {
        st_d_print("Got BEV_EVENT_TIMEOUT event! ");
    } else if (events & BEV_EVENT_READING) {
        st_d_print("Got BEV_EVENT_READING event! ");
    } else if (events & BEV_EVENT_WRITING) {
        st_d_print("Got BEV_EVENT_WRITING event! ");
    }

    if (loop_terminate)
    {
        bufferevent_free(bev);
        event_base_loopexit(base, NULL);
    }
}

// remove method
//bufferevent_read bufferevent_read_buffer
void bufferread_cb(struct bufferevent *bev, void *ptr)
{
    char *msg = "SERVER MESSAGE: WOSHINICOL 桃子大人";
    char buf[1024];
    int n;
    struct evbuffer *input = bufferevent_get_input(bev);
    struct evbuffer *output = bufferevent_get_output(bev);

    while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0) 
    {
        fwrite("BUFFERREAD_CB:", 1, strlen("BUFFERREAD_CB:"), stderr);
        fwrite(buf, 1, n, stderr);
    }

    fprintf(stderr, "READ DONE!\n");
    //bufferevent_write(bev, msg, strlen(msg));
    evbuffer_add(output, msg, strlen(msg));
}

static void
accept_conn_cb(struct evconnlistener *listener,
    evutil_socket_t fd, struct sockaddr *address, int socklen,
    void *ctx)
{ 
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

    st_d_print("Detect new connect ...");

    getnameinfo (address, socklen,
               hbuf, sizeof(hbuf),sbuf, sizeof(sbuf),
               NI_NUMERICHOST | NI_NUMERICSERV);
    st_print("Welcome (host=%s, port=%s)\n", hbuf, sbuf);
    /* We got a new connection! Set up a bufferevent for it. */
    struct event_base *base = evconnlistener_get_base(listener);
    struct bufferevent *bev = bufferevent_socket_new(
            base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_priority_set(bev, 2);

    /**
     * 对于服务端，一般都是阻塞在读，而如果要写，一般在read_cb中写回就可以了
     */
    bufferevent_setcb(bev, bufferread_cb, NULL, bufferevent_cb, NULL);
    bufferevent_enable(bev, EV_READ|EV_WRITE);

    st_d_print("Allocate and attach new bufferevent for new connectino...");

}

static void
accept_error_cb(struct evconnlistener *listener, void *ctx)
{
    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();

    st_d_print( "Got an error %d (%s) on the listener. "
            "Shutting down.\n", err, evutil_socket_error_to_string(err));
    event_base_loopexit(base, NULL);
}
