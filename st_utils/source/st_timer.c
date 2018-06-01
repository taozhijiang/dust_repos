#include "st_others.h"
#include "st_slist.h"
#include "st_timer.h"

/** 
 * 使用epoll timerfd实现定时器 
 * 传统的signal方式只能实现一个定时器，而posix 
 * timer由于也是用信号实现，会对现有的其他基于信号的函数产生干扰
 */

static void st_timer_cancel_func(void* data)
{
    P_ST_TIMER_SRV pt_srv = (P_ST_TIMER_SRV)data;
    st_print("Thread st_timer_service was canceled(%lu)!\n", 
             pt_srv->pid);
}

static void* st_timer_service(void *data)
{
    P_ST_TIMER_SRV pt_srv = (P_ST_TIMER_SRV)data;
    uint64_t dumy_value;
    P_ST_TIMER_OBJ p_node = NULL;

    int ready = 0, e_i = 0;

    st_print("st_timer_service running...\n");
    pthread_cleanup_push(st_timer_cancel_func, pt_srv);

    for ( ; ; )
    {
        ready = epoll_wait(pt_srv->event_fd, pt_srv->p_events, 
                           pt_srv->max_timers, -1); 
        for (e_i = 0; e_i < ready; e_i++)
        {
            // NEED TO BE DUMMY READ
            read(pt_srv->p_events[e_i].data.fd, &dumy_value, 8);
            //调用处理函数，由于处理函数时间不知道，所以，嘿嘿。。。
            slist_for_each_entry(p_node, &pt_srv->timer_objs, list)
            {
                if (p_node->timer_fd == pt_srv->p_events[e_i].data.fd)
                {
                    if (p_node->handler)
                    {
                        (*p_node->handler)(p_node);
                    }
                    else
                    {
                        SYS_ABORT("!!!!! NULL HANDLER FOR %s<%d>", 
                                 p_node->timer_name, p_node->timer_fd); 
                    }
                }
            }
        }
    }

    pthread_cleanup_pop(0);
    st_print("st_timer_service terminated...\n");
    return NULL;
}


P_ST_TIMER_SRV st_create_timer_service(int max_timers)
{
    P_ST_TIMER_SRV pt_srv = 
        (P_ST_TIMER_SRV)malloc(sizeof(ST_TIMER_SRV));

    RET_NULL_IF_TRUE(!pt_srv);
    memset(pt_srv, 0, sizeof(ST_TIMER_SRV));
    pt_srv->max_timers = max_timers;
    pthread_mutex_init(&pt_srv->mutex, NULL); 
    pt_srv->event_fd = epoll_create (max_timers);
    pt_srv->p_events = 
        (struct epoll_event*)calloc (max_timers, sizeof(struct epoll_event));  
    if ( !pt_srv->p_events)
    {
        free(pt_srv);
        return NULL;
    }
    memset(pt_srv->p_events, 0, max_timers*sizeof(struct epoll_event));
    // NULL for timer_objs

    if(pthread_create(&(pt_srv->pid), NULL,
                          st_timer_service, pt_srv) != 0) 
        goto failed;
    
    return pt_srv;

failed:
    if (pt_srv && pt_srv->p_events)
    {
        free(pt_srv->p_events);
        pthread_mutex_destroy(&pt_srv->mutex);
    }
    close(pt_srv->event_fd);
    if (pt_srv)
        free(pt_srv);
    return NULL; 
}

P_ST_TIMER_OBJ st_add_timer(P_ST_TIMER_SRV pt_srv,
                          const char* t_name, time_t msec, int repeatible, 
                          void* (*handler)(P_ST_TIMER_OBJ pt_obj), void* data)
{
    struct epoll_event new_event;
    P_ST_TIMER_OBJ pt_obj = NULL;
    struct itimerspec itv;
    struct timespec   now;
    int    ret = 0;

    if (!t_name || msec == 0 || !handler)
        return NULL;

    if (slist_count(&pt_srv->timer_objs) >= pt_srv->max_timers)
    {
        st_print("MAX TIMER(%d), UNABLE TO ADD!\n", pt_srv->max_timers);
        return NULL;
    }

    pt_obj = (P_ST_TIMER_OBJ)malloc(sizeof(ST_TIMER_OBJ));
    RET_NULL_IF_TRUE(!pt_obj);

    memset(pt_obj, 0, sizeof(ST_TIMER_OBJ));
    strncpy(pt_obj->timer_name, t_name, NAME_MAX);
    pt_obj->handler = handler;
    pt_obj->data = data; 
    pt_obj->repeatible = repeatible;
    pt_obj->m_sec = msec;
    pt_obj->timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

    GOTO_IF_TRUE((pt_obj->timer_fd == -1), failed);

    new_event.data.fd = pt_obj->timer_fd;
    new_event.events = EPOLLIN | EPOLLET;;

     // add timer
    clock_gettime(CLOCK_MONOTONIC, &now);
    memset(&itv, 0, sizeof(struct itimerspec));
    itv.it_value.tv_sec = now.tv_sec + msec/1000;
    itv.it_value.tv_nsec = now.tv_nsec + (msec%1000)*1000*1000;
    if (itv.it_value.tv_nsec >= 1000*1000*1000)
    {
        ++itv.it_value.tv_sec;
        itv.it_value.tv_nsec -= 1000*1000*1000;
    }
    if (repeatible)
    {
        itv.it_interval.tv_sec = msec / 1000; 
        itv.it_interval.tv_nsec = (msec%1000)*1000*1000;
    }


    pthread_mutex_lock(&pt_srv->mutex);
    epoll_ctl (pt_srv->event_fd, EPOLL_CTL_ADD, 
               pt_obj->timer_fd, &new_event);
   
    ret = timerfd_settime(pt_obj->timer_fd, TFD_TIMER_ABSTIME, &itv, NULL);
    pthread_mutex_unlock(&pt_srv->mutex);

    GOTO_IF_TRUE((ret == -1), failed);
    slist_add(&pt_obj->list, &pt_srv->timer_objs);

    st_print("ADDING TIMER %s:%lu ms OK!\n", pt_obj->timer_name,
             pt_obj->m_sec);

    return pt_obj;

failed:
    if (pt_obj)
        free(pt_obj);

    return NULL;
}

P_ST_TIMER_SRV st_remove_timer(P_ST_TIMER_SRV pt_srv,
                          const char* t_name)
{
    P_ST_TIMER_OBJ pt_obj = NULL;
    P_ST_TIMER_OBJ p_node = NULL;

    if (!pt_srv || !t_name || !strlen(t_name))
        return NULL;

    slist_for_each_entry(p_node, &pt_srv->timer_objs, list)
    {
        if ( !strncmp(p_node->timer_name, t_name, strlen(t_name)))
            break;
    }

    if (p_node)
    {
        pthread_mutex_lock(&pt_srv->mutex);
        close(p_node->timer_fd); //when close, will auto rm from events
        slist_remove(&p_node->list, &pt_srv->timer_objs);
        free(p_node);
        pthread_mutex_unlock(&pt_srv->mutex);
        st_print("TIMER %s removed!\n", t_name);
        return pt_srv;
    }
    else
    {
        st_print("!!!TIMER %s not found!\n", t_name);
        return NULL;
    }
}

void st_destroy_timers(P_ST_TIMER_SRV pt_srv)
{
    if (!pt_srv)
        return;

    P_ST_TIMER_OBJ p_node = NULL;
    slist_for_each_entry(p_node, &pt_srv->timer_objs, list)
    {
        close(p_node->timer_fd); //when close, will auto rm from events
        slist_remove(&p_node->list, &pt_srv->timer_objs);
        free(p_node);
    }
    close(pt_srv->event_fd);
    pthread_mutex_destroy(&pt_srv->mutex);
    pthread_cancel(pt_srv->pid);
    pthread_join(pt_srv->pid, NULL);
    free(pt_srv);

    return;
}

static void* st_timer_test_handler(P_ST_TIMER_OBJ pt_obj)
{
    if (! pt_obj->data)
        st_print("%s Handler with data == NULL \n",
                 pt_obj->timer_name); 
    else
        st_print("%s Handler with data == %s \n",
                 pt_obj->timer_name, (char *)pt_obj->data);

    return NULL;
}

///// TEST
void st_utils_timer_test(void)
{
    P_ST_TIMER_SRV pt_srv = 
        st_create_timer_service(3);

    st_add_timer(pt_srv, "TEST_TIMER1", 1000, 1, st_timer_test_handler, "T1");
    st_add_timer(pt_srv, "TEST_TIMER2", 2000, 1, st_timer_test_handler, NULL);
    st_add_timer(pt_srv, "TEST_TIMER3", 1020, 1, st_timer_test_handler, "桃子大人");
    st_add_timer(pt_srv, "TEST_TIMER4", 3000, 1, st_timer_test_handler, NULL);
    sleep(7);
    st_remove_timer(pt_srv, "AAAAA");
    st_remove_timer(pt_srv, "TEST_TIMER1");
    st_print("CND:%d\n", slist_count(&pt_srv->timer_objs));
    st_remove_timer(pt_srv, "TEST_TIMER3");
    st_print("CND:%d\n", slist_count(&pt_srv->timer_objs));
    sleep(10);
    st_destroy_timers(pt_srv);
    while (1)
    {
        sleep(1);
    }
}
