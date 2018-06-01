
////  
// 废弃，因为用信号的定时器会影响到其它程序中的sleep等
// 基于信号的函数被异常唤醒

#include "st_others.h"

#include <time.h>
#include <sys/time.h> 
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <linux/limits.h>

#define SIG_ST_TIMER (SIGRTMIN+5)   /*自定义信号*/

typedef struct _st_timer_obj {
    char    timer_name[NAME_MAX];
    timer_t tt;
    time_t  m_sec;
    int     repeatible;
    void    (*handler)(int sig, siginfo_t *si, void *uc);
    union {
        int     data;
        void*   p_data;
    };
} ST_TIMER_OBJ, *P_ST_TIMER_OBJ;

// 其它小的工具类函数
// REMEBER:at most one signal is queued to the  process  for  a  given timer
P_ST_TIMER_OBJ st_create_timer(P_ST_TIMER_OBJ p_timer)
{
    struct itimerspec itv;
    struct timespec   tm_val;
    struct sigevent   sev;
    sigset_t mask;
    struct sigaction  sa;

    if (!p_timer || !p_timer->handler || p_timer->m_sec == 0)
        return NULL;

    // 建立Timer信号与处理函数的关联
    sa.sa_flags = SA_SIGINFO;   //sa_sigaction
    sa.sa_sigaction = p_timer->handler;  
    sigemptyset(&sa.sa_mask);
    RET_NULL_IF_TRUE( sigaction(SIG_ST_TIMER, &sa, NULL) == -1 );

    //同时先Block住该信号处理
    sigemptyset(&mask);
    sigaddset(&mask, SIG_ST_TIMER);
    RET_NULL_IF_TRUE( sigprocmask(SIG_SETMASK, &mask, NULL) == -1 );

    // Create Timer
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIG_ST_TIMER;
    sev.sigev_value.sival_ptr = (void *)p_timer;    //sival_int sival_ptr union
    RET_NULL_IF_TRUE ( timer_create(CLOCK_REALTIME, &sev, &p_timer->tt) == -1 );

    tm_val.tv_sec = p_timer->m_sec / 1000; 
    tm_val.tv_nsec = (p_timer->m_sec % 1000)*1000*1000;
    memset(&itv, 0 , sizeof(struct itimerspec));
    itv.it_value = tm_val;  //fist time
    if (p_timer->repeatible)                //repeatible
        itv.it_interval = tm_val;

    RET_NULL_IF_TRUE( timer_settime(p_timer->tt, 0, &itv, NULL) == -1 );

    // Currently, Timer signal still been blocked!

    return p_timer;
}

int st_start_timer(timer_t tt)
{
    sigset_t mask;

    sigemptyset(&mask);
    sigaddset(&mask, SIG_ST_TIMER);
    return sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

int st_delete_timer(P_ST_TIMER_OBJ p_timer)
{
    if (!p_timer)
        return -1;

    return timer_delete(p_timer->tt); 
}

// uc user_context unused
static void test_handler(int sig, siginfo_t *si, void *uc)
{
    P_ST_TIMER_OBJ p_timer = 
        (P_ST_TIMER_OBJ)si->si_value.sival_ptr;
    int* num = (int *)p_timer->p_data;

    st_print("SIGNAL:%x\n",sig);
    if( strncmp("TEST_TIMER", p_timer->timer_name, strlen("TEST_TIMER")) )
        st_d_print("ERROR TIMER!\n");
    else
        st_d_print("JUST TEST OUTPUT of(%x:%s)!!!! with %d\n", 
                   p_timer->tt, p_timer->timer_name, *num);

    return;
}

void st_utils_test(void)
{

    ST_TIMER_OBJ timer;
    P_ST_TIMER_OBJ p_timer = &timer;
    int i = 2 ;

    memset(p_timer, 0, sizeof(ST_TIMER_OBJ));
    p_timer->m_sec = 2000;
    p_timer->handler = test_handler;
    p_timer->repeatible = 1;
    p_timer->p_data = &i;
    strncpy(p_timer->timer_name, "TEST_TIMER", NAME_MAX);


    st_create_timer(p_timer);
    st_print("Create Timer:%x\n", p_timer->tt);
    st_start_timer(p_timer->tt);

    while (i <= 10)
    {
        sleep(10);   // 15/2 = 7 timer action
        i++ ;
    }

    st_delete_timer(p_timer);

    while (1)
    {
        ;
    }
}
