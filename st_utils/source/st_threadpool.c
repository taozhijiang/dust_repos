#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>

#include "st_threadpool.h"

static const int MAX_THREADS = 100;

P_ST_THREAD_MANAGE st_threadpool_init(P_ST_THREAD_MANAGE p_manage, int cnt)
{
    int i = 0;

    if (!p_manage || cnt < 0)
        return NULL;

    cnt = (cnt > MAX_THREADS ? MAX_THREADS : cnt );
    P_ST_THREAD p_thread = NULL;
    p_manage->thread_total = cnt;

    memset(p_manage, 0, sizeof(ST_THREAD_MANAGE));
    pthread_mutex_init(&(p_manage->tk_mutex), NULL);
    pthread_cond_init(&(p_manage->tk_cond), NULL);

    //创建管理统计线程
    if(pthread_create(&(p_manage->manage_pid), NULL,
                          threadpool_manage, p_manage) != 0)
        return NULL;

    for (i = 0; i<cnt; i++)
    {
        p_thread = (P_ST_THREAD)malloc(sizeof(ST_THREAD));
        if (!p_thread)
            goto fail_proc;
        memset(p_thread, 0, sizeof(ST_THREAD));
        p_thread->status = THREAD_SPARE;
        if(pthread_create(&(p_thread->pid), NULL,
                          threadpool_run, p_manage) != 0)
            goto fail_proc;

        slist_add(&p_thread->list, &p_manage->threads);
        p_manage->thread_running += 1;

    }

    return p_manage;

fail_proc:
    st_threadpool_destroy(p_manage);
    return NULL;
}

void st_threadpool_destroy(P_ST_THREAD_MANAGE p_manage)
{
    P_SLIST_HEAD pos = NULL;
    P_ST_THREAD  pth = NULL;
    P_ST_TASK    ptk = NULL;

    if (!p_manage)
        return;

    if (!slist_empty(&p_manage->threads))
    {
        slist_for_each_entry(pth, &p_manage->threads, list)
        {
            if (pth->pid)
            {
                pthread_cancel(pth->pid);
                pthread_join(pth->pid, NULL); // only this time was really canceled
                free(pth);
            }
        }
    }

    if (!slist_empty(&p_manage->tasks))
    {
        slist_for_each_entry(ptk, &p_manage->tasks, list)
        {
           free(ptk);
        }
    }

    if (p_manage->manage_pid)
    {
        pthread_cancel(p_manage->manage_pid);
    }


    pthread_mutex_destroy(&p_manage->tk_mutex);
    pthread_cond_destroy(&p_manage->tk_cond);

}

void st_threadpool_statistic(P_ST_THREAD_MANAGE p_manage)
{
    P_SLIST_HEAD pos = NULL;
    P_ST_THREAD  pth = NULL;
    P_ST_TASK    ptk = NULL;
    time_t now;
    time(&now);

    st_print("\n");
    st_print("======================\n");
    st_print("General Thread Status:\n");
    st_print("SPARE:%d\t RUNNING:%d\t DEAD:%d\n",
             p_manage->thread_spare, p_manage->thread_running, p_manage->thread_dead);

    st_print("======================\n");
    st_print("Detail Thread Status:\n");
    slist_for_each(pos, &p_manage->threads)
    {
        pth = list_entry(pos, ST_THREAD, list);
        st_print("TID[%lu], status:%s, time:%fSec\n", pth->pid,
                 THREAD_STATUS_STR[pth->status], difftime(now, pth->start));
    }

    st_print("======================\n");
    st_print("General Task Status:\n");
    st_print("PENDING:%d\n", slist_count(&p_manage->tasks));
    st_print("\nTIME:%s\n",ctime(&now));

    return;
}

void st_threadpool_refresh(P_ST_THREAD_MANAGE p_manage)
{
    P_SLIST_HEAD pos = NULL;
    P_ST_THREAD  pth = NULL;

    int spare = 0;
    int running = 0;
    int dead = 0;


    slist_for_each(pos, &p_manage->threads)
    {
        pth = list_entry(pos, ST_THREAD, list);

        //检测线程是否存活
        if(pthread_kill(pth->pid, 0))
            pth->status = THREAD_DEAD;


        if (pth->status == THREAD_SPARE)
            ++spare;
        if (pth->status == THREAD_RUNNING)
            ++running;
        if (pth->status == THREAD_DEAD)
            ++dead;
    }

    p_manage->thread_spare = spare;
    p_manage->thread_running = running;
    p_manage->thread_dead = dead;

    return;
}

int st_threadpool_push_task(P_ST_THREAD_MANAGE p_manage, void *(*start_routine) (void *), void *arg)
{
    if (!p_manage || !start_routine)
        return 0;

    P_ST_TASK p_tk = (P_ST_TASK)malloc(sizeof(ST_TASK));
    if (! p_tk)
        return 0;
    p_tk->handler = start_routine;
    p_tk->arg = arg;
    p_tk->list.next = NULL;

    pthread_mutex_lock(&p_manage->tk_mutex);
    slist_add(&p_tk->list, &p_manage->tasks);
    //如果等待的任务太多，而且有睡眠线程，就尝试唤醒。
    if (p_manage->thread_spare > 0 &&
        slist_count(&p_manage->tasks) > (int)(p_manage->thread_total * 0.5))
    {
        if (slist_count(&p_manage->tasks) > (int)(p_manage->thread_total * 0.2))
            pthread_cond_signal(&p_manage->tk_cond);
        else
            pthread_cond_signal(&p_manage->tk_cond);
    }
    pthread_mutex_unlock(&p_manage->tk_mutex);

    return 1;
}

int st_threadpool_pop_task(P_ST_THREAD_MANAGE p_manage, P_ST_TASK p_task_store)
{
    P_SLIST_HEAD ptr = NULL;
    P_ST_TASK p_tk = NULL;

    if (!p_manage || !p_task_store || slist_empty(&p_manage->tasks))
        return 0;

    pthread_mutex_lock(&p_manage->tk_mutex);
    ptr = slist_fetch(&p_manage->tasks);
    pthread_mutex_unlock(&p_manage->tk_mutex);

    if (ptr)
    {
        p_tk = list_entry(ptr, ST_TASK, list);
        p_task_store->handler = p_tk->handler;
        p_task_store->arg = p_tk->arg;
        p_task_store->list.next = NULL;
        free(p_tk);
        return 1;
    }

    return 0;
}

static void* threadpool_manage(void *data)
{
    P_ST_THREAD_MANAGE p_manage =
        (P_ST_THREAD_MANAGE)data;

    for ( ; ;)
    {
        sleep(2);
        st_threadpool_refresh(p_manage);
        st_threadpool_statistic(p_manage);
    }
}

static void* threadpool_run(void *data)
{
    ST_TASK task;
    P_ST_THREAD_MANAGE p_manage = (P_ST_THREAD_MANAGE)data;
    P_ST_THREAD p_me = NULL;
    P_ST_THREAD ptr = NULL;

retry:

    p_me = NULL;

    slist_for_each_entry(ptr, &p_manage->threads, list)
    {
        if (ptr->pid == pthread_self())
        {
            p_me = ptr;
            break;
        }
    }

    if (!p_me)
    {
        st_print("PID FIND ERROR!\n");
        sleep(5);
        goto retry;
    }

    for ( ; ;)
    {
        while (st_threadpool_pop_task(p_manage, &task))
        {
            time(&p_me->start);
            p_me->status = THREAD_RUNNING;
            (*(task.handler))(task.arg);
        }

        pthread_mutex_lock(&p_manage->tk_mutex);
        //即使被惊醒了，也会跳过这个循环上去再次检测
        p_me->status = THREAD_SPARE;
        pthread_cond_wait(&p_manage->tk_cond, &p_manage->tk_mutex);
        st_print("Thread Waitup:%lu\n", pthread_self());
        pthread_mutex_unlock(&p_manage->tk_mutex);
    }
}


static void* test_func(void* data)
{
    static int i = 0;
    int num = *(int *)data;
    //st_print("Function Called with %d under %ul \n", num, pthread_self());

    i ++;

    // 活动检测测试
    if (i > 30)
       pthread_exit(NULL);

    sleep(2);
}

int st_threadpool_test(void)
{
    ST_THREAD_MANAGE st_manage;
    P_ST_THREAD_MANAGE p_manage = &st_manage;

    if ( !st_threadpool_init(p_manage, 10))
    {
        st_print("threadpool初始化失败!\n");
        return 0;
    }

    int num = 1;
    int i   = 0;

    for (i = 0; i < 20000; i++)
    {
        num += i;
        st_threadpool_push_task(p_manage,test_func,&num);
        num += 3;
        st_threadpool_push_task(p_manage,test_func,&num);
        num += 5;
        st_threadpool_push_task(p_manage,test_func,&num);
        num += 7;
        st_threadpool_push_task(p_manage,test_func,&num);
        num += i;
        st_threadpool_push_task(p_manage,test_func,&num);
        num += 3;
        st_threadpool_push_task(p_manage,test_func,&num);
        num += 5;
        st_threadpool_push_task(p_manage,test_func,&num);
        num += 7;
        st_threadpool_push_task(p_manage,test_func,&num);
        num += i;
        st_threadpool_push_task(p_manage,test_func,&num);
        num += 3;
        st_threadpool_push_task(p_manage,test_func,&num);
        num += 5;
        st_threadpool_push_task(p_manage,test_func,&num);
        num += 7;
        st_threadpool_push_task(p_manage,test_func,&num);
        sleep(random()%7);
    }
}

