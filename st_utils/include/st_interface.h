#ifndef __ST_INTERFACE_H
#define __ST_INTERFACE_H

#ifdef __cplusplus 
extern "C" { 
#endif //__cplusplus

#include "st_others.h"
/**
 * For st_epoll
 */
#include <sys/epoll.h>
#include "WINDEF.H"
#include "st_threadpool.h"
#include <linux/limits.h>

typedef struct __epoll_event 
{
    struct epoll_event event;
    int listen_socket;
    int max_events;
    int event_fd;
    struct epoll_event* p_events;
} EPOLL_STRUCT, *P_EPOLL_STRUCT;

void st_epoll_test(void);
void st_event_loop(P_EPOLL_STRUCT p_epoll, P_ST_THREAD_MANAGE p_manage, void* handler(void* data));
P_EPOLL_STRUCT st_make_events(int lsocket, size_t maxepoll_size);
int st_make_nonblock(int lsocket);
int st_add_new_event(int accepted_socket, P_EPOLL_STRUCT p_epoll);
int st_buildsocket(int listen_cnt, int port);


/**
 * For st_memmap
 */

#include <limits.h>

typedef struct _st_memmap_t
{    	
    void*   location;   //映射得到的内存地址
    int	    fd;	        //文件操作句柄
    size_t	size;	    //最大映射大小
    char    filename[PATH_MAX]; //映射到物理磁盘文件
    char    mapname[PATH_MAX];  //map的名字，例如 RPCShare等
} ST_MEMMAP_T, * P_ST_MEMMAP_T;

void* st_memmap_create(const char* filename, const char* share_name, size_t max_size);
void* st_memmap_open(const char* share_name, int fixaddr, int writable);
void st_memmap_close(P_ST_MEMMAP_T p_token);
void st_memmap_destroy(P_ST_MEMMAP_T p_token);
void st_memmap_test(void);


/**
 *  For Shm
 */
#include <sys/types.h>
#include <sys/ipc.h>
	   
typedef struct _st_shm_t
{    	
    void*   location;   //映射得到的内存地址
	int		fd;
    size_t	size;	    //最大映射大小  PAGE_SIZE  SHMMAX  
    char    shmname[NAME_MAX];  //map的名字，例如 RPCShare等
} ST_SHM_T, * P_ST_SHM_T;

void* st_shm_create(const char* share_name, size_t max_size);
void* st_shm_open(const char* share_name, int fixaddr, int writable);
void st_shm_close(P_ST_SHM_T p_token);
void st_shm_destroy(P_ST_SHM_T p_token);
void st_shm_test(void);


/**
 *  For Win Sync
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

// CriticalSection
typedef pthread_mutex_t CRITICAL_SECTION;
typedef CRITICAL_SECTION *LPCRITICAL_SECTION;
void InitializeCriticalSection(LPCRITICAL_SECTION section);
void EnterCriticalSection(LPCRITICAL_SECTION section);
void LeaveCriticalSection(LPCRITICAL_SECTION section);
void DeleteCriticalSection(LPCRITICAL_SECTION section);


// Mutex

//为了实现Wait Close 接口的统一，以及将来同步方式的扩展
enum SYNC_TYPE 
{
    SYNC_MUTEX,
    SYNC_EVENT,
};

typedef struct _st_winsync_t
{
    char    sync_name[PATH_MAX];   //strlen==0 if Intra-process
    enum    SYNC_TYPE type;
    union
    {
        sem_t   sem;      //unamed mutex
        sem_t*  p_sem;    //named mutex
    };
    void*   extra;
} ST_WINSYNC_T, * P_ST_WINSYNC_T;

BOOL  st_winsync_destroy(P_ST_WINSYNC_T p_sync);

HANDLE CreateMutex( void* lpMutexAttributes,
                BOOL bInitialOwner, const char* lpName);
HANDLE OpenMutex( DWORD dwDesiredAccess,
                BOOL bInheritHandle, const char* lpName);
//如果超时，返回ETIMEDOUT
DWORD  WaitForSingleObject(HANDLE hHandle,
                DWORD dwMilliseconds);
BOOL  ReleaseMutex(HANDLE hMutex);
BOOL  CloseHandle( HANDLE hObject);


// Event
HANDLE CreateEvent(void* lpEventAttributes,
                BOOL bManualReset,BOOL bInitialState,
                const char* lpName);
HANDLE WINAPI OpenEvent( DWORD dwDesiredAccess,
                         BOOL bInheritHandle, const char* lpName );
BOOL ResetEvent( HANDLE hEvent);
// 如果已经有事件了，就返回EBUSY，不实际再发送
BOOL SetEvent( HANDLE hEvent);


void Sleep(DWORD dwMilliseconds);
BOOL get_workdir( char* store);

#ifdef __cplusplus 
}
#endif //__cplusplus

#endif
