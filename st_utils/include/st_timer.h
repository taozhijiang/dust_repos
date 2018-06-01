#ifndef __ST_TIMER_H
#define __ST_TIMER_H

#ifdef __cplusplus 
extern "C" {
#endif //__cplusplus 


// Timer
#include <sys/timerfd.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>


typedef struct _st_timer_srv 
{
    pthread_t   pid;
    int         max_timers;
    SLIST_HEAD  timer_objs;
    pthread_mutex_t mutex;
    int         event_fd;
    struct epoll_event* p_events;
} ST_TIMER_SRV, *P_ST_TIMER_SRV;

typedef struct _st_timer_obj {
    SLIST_HEAD list;
    char    timer_name[NAME_MAX];
    int     timer_fd;
    time_t  m_sec;
    int     repeatible;
    void*   (* handler)(struct _st_timer_obj* pt_obj);
    void*   data;
} ST_TIMER_OBJ, *P_ST_TIMER_OBJ;

P_ST_TIMER_SRV st_create_timer_service(int max_timers);
P_ST_TIMER_OBJ st_add_timer(P_ST_TIMER_SRV pt_srv,
                          const char* t_name, time_t msec, int repeatible, 
                          void* (*handler)(P_ST_TIMER_OBJ pt_obj), void* data);
P_ST_TIMER_SRV st_remove_timer(P_ST_TIMER_SRV pt_srv,
                          const char* t_name);
void st_destroy_timers(P_ST_TIMER_SRV pt_srv);
void st_utils_timer_test(void);



#ifdef __cplusplus 
}
#endif //__cplusplus 


#endif //__ST_TIMER_H
