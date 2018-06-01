#include "WINDEF.H"
#include "st_others.h"
#include "st_interface.h"

#include <sys/timeb.h>
#include <pthread.h>

/**
 * 主要用于提供一些在Windows下同步操作的Linux实现 
 * 包括 线程内部的  CriticalSection 
 * 进程间的 Mutex 
 *        Event 
 */

/*****************************
 * Windows 临界区，只用于同一个进程的线程同步
 */
typedef pthread_mutex_t CRITICAL_SECTION;
typedef CRITICAL_SECTION *LPCRITICAL_SECTION;

void InitializeCriticalSection(LPCRITICAL_SECTION section)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr); 
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);  //可递归
    pthread_mutex_init(section, &attr);
    pthread_mutexattr_destroy(&attr);
}

void EnterCriticalSection(LPCRITICAL_SECTION section)
{
    pthread_mutex_lock(section);
}

void LeaveCriticalSection(LPCRITICAL_SECTION section)
{
    pthread_mutex_unlock(section);
}

void DeleteCriticalSection(LPCRITICAL_SECTION section)
{
	pthread_mutex_destroy(section);
}


/*****************************
 * Windows Mutex互斥锁 
 * 可以跨进程 
 */

/**
 * 由于自己封装了结构，方便调用，所以有fork亲属关系的进程，不再
 * 让其继承mutex结构，需要创建使用命名Mutex
 */
HANDLE CreateMutex( void* lpMutexAttributes,
                BOOL bInitialOwner, const char* lpName)
{
    P_ST_WINSYNC_T p_mutex = NULL;

    p_mutex = (P_ST_WINSYNC_T)malloc(sizeof(ST_WINSYNC_T));
    if (!p_mutex)
        return NULL;

    memset(p_mutex, 0, sizeof(ST_WINSYNC_T));
    p_mutex->type = SYNC_MUTEX;

    if ( lpName) // Inter-Process
    {
        strncpy(p_mutex->sync_name, lpName, PATH_MAX);
        sem_unlink(lpName); // Just for safe
        //bInitialOwner
        p_mutex->p_sem = sem_open(p_mutex->sync_name, O_CREAT | O_EXCL, 0644, bInitialOwner); 
        if (p_mutex->p_sem == SEM_FAILED)
        {
            st_d_print("Create Mutex: %s failed!\n", p_mutex->sync_name);
            free(p_mutex);
            return NULL;
        }
    }
    else        // Intra-Process
    {
        p_mutex->sync_name[0] = '\0';
        if( sem_init((sem_t *)(&p_mutex->sem), 1/*pshared for forked dismiss*/, 1) != 0)
        {
            st_print("semaphore initilization");
            return NULL;
        }
    }

    return p_mutex;
}

HANDLE OpenMutex( DWORD dwDesiredAccess,
                BOOL bInheritHandle, const char* lpName)
{
    P_ST_WINSYNC_T p_mutex = NULL;

    if ( !lpName)
    {
        st_print("You may use this function for inter-process, please check!\n");
        return NULL;
    }

    p_mutex = (P_ST_WINSYNC_T)malloc(sizeof(ST_WINSYNC_T));
    if (!p_mutex)
        return NULL;

    memset(p_mutex, 0, sizeof(ST_WINSYNC_T));
    p_mutex->type = SYNC_MUTEX;

    strncpy(p_mutex->sync_name, lpName, PATH_MAX);
    // initial value => 0
    p_mutex->p_sem = sem_open(p_mutex->sync_name, 0, 0644, 0);
    if (p_mutex->p_sem == SEM_FAILED)
    {
        st_d_print("Open Mutex: %s failed!\n", p_mutex->sync_name);
        free(p_mutex);
        return NULL;
    }

    return p_mutex;
}

static struct timespec* 
st_make_timespec(struct timespec* p_ts, time_t dwMilliseconds)
{
    struct timeb    tb;
    time_t sec, millisec;

    if (!p_ts)
        return NULL;
    
    sec      = dwMilliseconds / 1000;
    millisec = dwMilliseconds % 1000;

    ftime( &tb );
    tb.time     += sec;
    tb.millitm  += millisec;

    tb.time     += (tb.millitm / 1000);
    tb.millitm  =  tb.millitm % 1000;

    p_ts->tv_sec   = tb.time;
    p_ts->tv_nsec  = tb.millitm * 1000000 ;    //ns

    return p_ts;
}

DWORD  WaitForSingleObject(HANDLE hHandle,
                DWORD dwMilliseconds)
{
    P_ST_WINSYNC_T p_sync = (P_ST_WINSYNC_T)hHandle;

    struct timespec	ts;

    if (! p_sync)
    {
        st_d_print("Invalid Argument!\n");
        return 0;
    }

    if ( dwMilliseconds == INFINITE)
    {
        if (p_sync->type == SYNC_MUTEX)
        {
            if (strlen(p_sync->sync_name))  //inter-process
            {
                return sem_wait((sem_t *)(p_sync->p_sem));
            }
            else    // intra-process
            {
                return sem_wait((sem_t *)(&p_sync->sem));
            }
        }
        if (p_sync->type == SYNC_EVENT)
        {
            if (strlen(p_sync->sync_name))  //inter-process
            {
                return sem_wait((sem_t *)(p_sync->p_sem));
            }
            else    // intra-process
            {
                return sem_wait((sem_t *)(&p_sync->sem));
            }
        }
    }
    else
    {
        st_make_timespec(&ts, dwMilliseconds);

        if (p_sync->type == SYNC_MUTEX)
        {
            if (strlen(p_sync->sync_name))  //inter-process
            {
                if(sem_timedwait((sem_t *)(p_sync->p_sem), &ts))
                {
                    if ( errno == ETIMEDOUT )
                        return ETIMEDOUT;
                    else
                        return -1;
                }
            }
            else    // intra-process
            {
                if(sem_timedwait((sem_t *)(&p_sync->sem), &ts))
                {
                    if ( errno == ETIMEDOUT )
                        return ETIMEDOUT;
                    else
                        return -1;
                }
            }
        }
        if (p_sync->type == SYNC_EVENT)
        {
            if (strlen(p_sync->sync_name))  //inter-process
            {
                if(sem_timedwait((sem_t *)(p_sync->p_sem), &ts))
                {
                    if ( errno == ETIMEDOUT )
                        return ETIMEDOUT;
                    else
                        return -1;
                }
            }
            else    // intra-process
            {
                if(sem_timedwait((sem_t *)(&p_sync->sem), &ts))
                {
                    if ( errno == ETIMEDOUT )
                        return ETIMEDOUT;
                    else
                        return -1;
                }
            }
        }
    }

    return 0;
}

BOOL  ReleaseMutex(HANDLE hMutex)
{
    P_ST_WINSYNC_T p_mutex = (P_ST_WINSYNC_T)hMutex;
    if (!p_mutex || p_mutex->type != SYNC_MUTEX)
    {
        st_d_print("Invalid Argument!\n");
        return FALSE;
    }

    if (p_mutex->type == SYNC_MUTEX)
    {
        if (strlen(p_mutex->sync_name))  //inter-process
        {
            sem_post((sem_t *)(p_mutex->p_sem));
        }
        else    // intra-process
        {
            sem_post((sem_t *)(&p_mutex->sem));
        }
    }
    else
    {
        SYS_ABORT("WIN SYNC error For Mutex with %d\n", p_mutex->type);
    }

    return TRUE;
}

BOOL  CloseHandle( HANDLE hObject)
{
    P_ST_WINSYNC_T p_sync = (P_ST_WINSYNC_T)hObject;

    if (!p_sync)
        return FALSE;

    if (p_sync->type == SYNC_MUTEX)
    {
        if (strlen(p_sync->sync_name))  //inter-process
        {
            sem_close((sem_t *)(p_sync->p_sem));
            sem_unlink(p_sync->sync_name);
        }
        else    // intra-process
        {
            sem_close((sem_t *)(&p_sync->sem));   //where to call sem_destroy???
        }
    }

    if (p_sync->type == SYNC_EVENT)
    {
        if (strlen(p_sync->sync_name))  //inter-process
        {
            sem_close((sem_t *)(p_sync->p_sem));
            sem_unlink(p_sync->sync_name);
        }
        else    // intra-process
        {
            sem_close((sem_t *)(&p_sync->sem));   //where to call sem_destroy???
        }
    }

    return TRUE;
}

/**
 * Sometimes Needed for Clean Up
 * You should know WHAT THIS DO!
 */
BOOL  st_winsync_destroy(P_ST_WINSYNC_T p_sync)
{
    if (!p_sync)
        return FALSE;

    if (p_sync->type == SYNC_MUTEX)
    {
        if (strlen(p_sync->sync_name))  //inter-process
        {
            /*Nothing*/;
        }
        else    // intra-process
        {
            sem_destroy((sem_t *)(&p_sync->sem)); 
        }
    }

    return TRUE;
}


/**************** 
 * Windows事件通知，可以跨进程 
 *  
 * Setting an event that is already set has no effect. (NOT IMPELTE)
 *  
 * Using Mutex simulate producer & comsumer
 */
HANDLE CreateEvent(void* lpEventAttributes,
                BOOL bManualReset,BOOL bInitialState,
                const char* lpName)
{
    P_ST_WINSYNC_T p_event = NULL;

    p_event = (P_ST_WINSYNC_T)CreateMutex(lpEventAttributes,
                0, lpName);

    if (!p_event)
        return NULL;

    p_event->type = SYNC_EVENT;

    if ( bInitialState )
        SetEvent(p_event);

    return p_event; 
}

HANDLE WINAPI OpenEvent( DWORD dwDesiredAccess,
                         BOOL bInheritHandle, const char* lpName )
{
    P_ST_WINSYNC_T p_event = NULL;

    p_event = (P_ST_WINSYNC_T)OpenMutex(dwDesiredAccess,
                bInheritHandle, lpName);

    if (!p_event)
        return NULL;

    p_event->type = SYNC_EVENT;

    return p_event; 
}


BOOL ResetEvent( HANDLE hEvent)
{

    st_d_print("Not Implemented!!!!\n");

    return FALSE;
}

BOOL SetEvent( HANDLE hEvent)
{
    P_ST_WINSYNC_T p_event = (P_ST_WINSYNC_T)hEvent;
    int ret = 0;

    if (p_event->type == SYNC_EVENT)
    {
        //如果上次的事件还没有被消费掉，就忽略本次的发送
        //是否要重发，可以调用程序根据结果来判断

        if (strlen(p_event->sync_name))  //inter-process
        {
            sem_getvalue((sem_t *)(p_event->p_sem), &ret);
            if (ret > 0)
            {
                st_d_print("Producer Fast, Dismiss it!\n");
                return EBUSY;
            }
            return sem_post((sem_t *)(p_event->p_sem)); 
        }
        else    // intra-process
        {
            sem_getvalue((sem_t *)(&p_event->sem), &ret);
            if (ret > 0)
            {
                st_d_print("Producer Fast, Dismiss it!\n");
                return EBUSY;
            }
            return sem_post((sem_t *)(&p_event->sem));
        }
    }
    else
    {
        SYS_ABORT("WIN SYNC error For Event with %d\n", p_event->type);
    }

	return TRUE;
}


void Sleep(DWORD dwMilliseconds)
{
	if(dwMilliseconds >= 1000 && !(dwMilliseconds % 1000))
		sleep(dwMilliseconds/1000);
	else
		usleep(dwMilliseconds*1000);
	
	return;
}


BOOL get_workdir( char* store)
{
    if(!store)
        return FALSE;
        
    int cnt = readlink("/proc/self/exe", store, PATH_MAX);
    if(cnt == -1)
        return FALSE;
        
    char* ptr = store + strlen(store);
    while(ptr > store && *ptr != '/')
        ptr --;
    
    if(ptr < store)
        return FALSE;
     
     *(ptr+1) = '\0';
        
    return TRUE;
}



/**
 * TEST
 */


#include <sys/wait.h>

void* mutex_thread(void* data)
{
    int i = 0, nloop = 10;
    int ret = 0;

    P_ST_WINSYNC_T p_mutex = (P_ST_WINSYNC_T)data;
    P_ST_MEMMAP_T p_token = (P_ST_MEMMAP_T)p_mutex->extra;

    for (i = 0; i < nloop; i++) 
    {
        ret = WaitForSingleObject(p_mutex, 2000); //2s
        if (ret == ETIMEDOUT)
        {
            st_print("THREAD WAIT TIMEDOUT\n");
            continue;
        }
        st_print("G_LOCK\n"); 
        write(p_token->fd, "GGGGG", 5);
        usleep(200*1000);
        write(p_token->fd, "GGGGG", 5);
        st_print("G_UNLOCK\n");
        ReleaseMutex(p_mutex);
        usleep(100*1000);
    }

    return NULL;
}

void st_mutex_test_intra(void)
{
    P_ST_MEMMAP_T p_token = NULL;
    p_token = st_memmap_create(NULL, "RPC_SHARE", 4096);
    char* ptr = p_token->location;
    *ptr = '\0';
    char buf[512];

    if (!p_token)
        return;

    P_ST_WINSYNC_T p_mutex = (P_ST_WINSYNC_T)CreateMutex(NULL, 0, "RPC_MUTEX");
    
    int i = 0, nloop = 10;
    lseek(p_token->fd, 0, SEEK_SET);


    if (fork() == 0) 
    { /* child process*/
        //虽然系统保证sem_t是共享位置，但是由于其它部分没有存放到共享地址，所以父子进程不能
        //共享匿名Mutex...
        p_mutex = (P_ST_WINSYNC_T)OpenMutex(0, 0, "RPC_MUTEX");

        pthread_t pid;
        p_mutex->extra = p_token;
        pthread_create(&pid, NULL,mutex_thread, p_mutex);

        for (i = 0; i < nloop; i++) 
        {
            WaitForSingleObject(p_mutex, INFINITE);
            st_print("C_LOCK\n");
            write(p_token->fd, "CCCCC", 5);
            usleep(500*1000);
            write(p_token->fd, "CCCCC", 5);
            st_print("C_UNLOCK\n");
            ReleaseMutex(p_mutex);
            usleep(400*1000);
        }

        pthread_join(pid, NULL);

        CloseHandle(p_mutex);
        st_memmap_close(p_token);

        exit(0);
    }

    /* back to parent process */
    for (i = 0; i < nloop; i++) 
    {
        WaitForSingleObject(p_mutex, INFINITE);
        st_print("P_LOCK\n");
        write(p_token->fd, "PPPPP", 5);
        usleep(400*1000);
        write(p_token->fd, "PPPPP", 5);
        st_print("P_UNLOCK\n");
        ReleaseMutex(p_mutex);
        usleep(500*1000);
    }

    wait(NULL);
    
    lseek(p_token->fd, 0, SEEK_SET);
    int sz = read(p_token->fd, buf, 512);
    buf[sz] = '\0';
    st_print("INFO(%lu):%s\n",strlen(buf),buf);

    st_memmap_close(p_token);
    st_memmap_destroy(p_token);
    CloseHandle(p_mutex);
    st_winsync_destroy(p_mutex);

}


void st_event_comsumer_test(void)
{
    P_ST_MEMMAP_T p_token = NULL;
    p_token = st_memmap_create(NULL, "RPC_SHARE", 4096);
    char* ptr = p_token->location;
    *ptr = '\0';
    char buf[512];

    if (!p_token)
        return;

    P_ST_WINSYNC_T p_event = (P_ST_WINSYNC_T)CreateEvent(NULL, 0, 0, "RPC_EVENT");
    
    int i = 0, nloop = 100;
    lseek(p_token->fd, 0, SEEK_SET);

    /* back to parent process */
    for (i = 0; i < nloop; i++) 
    {
        WaitForSingleObject(p_event, INFINITE);
        memset(buf, 0, sizeof(buf));
        lseek(p_token->fd, 0, SEEK_SET);
        read(p_token->fd, buf, sizeof(buf));
        st_print("COMSUMER GET:%s\n", buf);
        usleep(300);
    }
    
    st_memmap_close(p_token);
    st_memmap_destroy(p_token);
    CloseHandle(p_event);
    st_winsync_destroy(p_event);
}

void st_event_producer_test(void)
{

    P_ST_MEMMAP_T p_token = NULL;
    p_token = st_memmap_open("RPC_SHARE", 1, 1);
    char* ptr = p_token->location;
    *ptr = '\0';
    char buf[512];

    if (!p_token)
        return;

    P_ST_WINSYNC_T p_event = (P_ST_WINSYNC_T)OpenEvent(0, 0, "RPC_EVENT");
    
    int i = 0, nloop = 100;

    WAIT_FOR_ENTER;

    /* back to parent process */
    for (i = 0; i < nloop; i++) 
    {
        snprintf(buf, sizeof(buf), "THIS IS MESSAGE: ID-%d", i);
        lseek(p_token->fd, 0, SEEK_SET);
        write(p_token->fd, buf, strlen(buf)+1);
        st_print("POSTING:%s\n", buf);
        SetEvent(p_event);
    }

    st_memmap_close(p_token);
    CloseHandle(p_event);
    st_winsync_destroy(p_event);
}


void* event_thread(void* data)
{
    int i = 0, nloop = 20;
    int ret = 0;
    char buf[512];

    P_ST_WINSYNC_T p_event = (P_ST_WINSYNC_T)data;
    P_ST_MEMMAP_T p_token = (P_ST_MEMMAP_T)p_event->extra;

    for (i = 0; i < nloop; i++) 
    {
        snprintf(buf, sizeof(buf), "THIS IS MESSAGE: ID-%d", i);
        lseek(p_token->fd, 0, SEEK_SET);
        write(p_token->fd, buf, strlen(buf)+1);
        st_print("POSTING:%s\n", buf);
        SetEvent(p_event);
    }

    return NULL;
}


void st_event_thread_test(void)
{
    P_ST_MEMMAP_T p_token = NULL;
    p_token = st_memmap_create(NULL, "RPC_SHARE", 4096);
    char* ptr = p_token->location;
    *ptr = '\0';
    char buf[512];
    int ret = 0;

    if (!p_token)
        return;

    P_ST_WINSYNC_T p_event = (P_ST_WINSYNC_T)CreateEvent(NULL, 0, 0, "RPC_EVENT3");
    
    pthread_t pid;
    p_event->extra = p_token;
    pthread_create(&pid, NULL,event_thread, p_event);

    int i = 0, nloop = 100;
    lseek(p_token->fd, 0, SEEK_SET);

    /* back to parent process */
    for (i = 0; i < nloop; i++) 
    {
        ret = WaitForSingleObject(p_event, 1000); //1s
        if (ret == ETIMEDOUT)
        {
            st_print("WAIT TIME OUT!\n");
            continue;
        }
        memset(buf, 0, sizeof(buf)); 
        lseek(p_token->fd, 0, SEEK_SET);
        read(p_token->fd, buf, sizeof(buf));
        st_print("COMSUMER GET:%s\n", buf);
        usleep(3000);
    }
    
    st_memmap_close(p_token);
    st_memmap_destroy(p_token);
    CloseHandle(p_event);
    st_winsync_destroy(p_event);
}
