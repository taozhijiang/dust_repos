/****************************************************************************
*                                                                           *
* windef.h -- Basic Windows Type Definitions                                *
*                                                                           *
* Copyright (c) Microsoft Corporation. All rights reserved.                 *
*                                                                           *
****************************************************************************/


#ifndef _WINDEF_
#define _WINDEF_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * BASETYPES is defined in ntdef.h if these types are already defined
 */

#ifndef BASETYPES
#define BASETYPES
typedef unsigned long ULONG;
typedef ULONG *PULONG;
typedef unsigned short USHORT;
typedef USHORT *PUSHORT;
typedef unsigned char UCHAR;
typedef UCHAR *PUCHAR;
typedef char *PSZ;
#endif  /* !BASETYPES */

#define MAX_PATH          260

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

#undef far
#undef near
#undef pascal

#define far
#define near
#if (!defined(_MAC)) && ((_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED))
#define pascal __stdcall
#else
#define pascal
#endif

#if defined(DOSWIN32) || defined(_MAC)
#define cdecl _cdecl
#ifndef CDECL
#define CDECL _cdecl
#endif
#else
#define cdecl
#ifndef CDECL
#define CDECL
#endif
#endif

#ifdef _MAC
#define CALLBACK    PASCAL
#define WINAPI      CDECL
#define WINAPIV     CDECL
#define APIENTRY    WINAPI
#define APIPRIVATE  CDECL
#ifdef _68K_
#define PASCAL      __pascal
#else
#define PASCAL
#endif
#elif (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#define WINAPIV     __cdecl
#define APIENTRY    WINAPI
#define APIPRIVATE  __stdcall
#define PASCAL      __stdcall
#else
#define CALLBACK
#define WINAPI
#define WINAPIV
#define APIENTRY    WINAPI
#define APIPRIVATE
#define PASCAL      pascal
#endif

#ifdef _M_CEE_PURE
#define WINAPI_INLINE  __clrcall
#else
#define WINAPI_INLINE  WINAPI
#endif

#ifndef CONST
#define CONST               const
#endif

typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef float               FLOAT;
typedef FLOAT               *PFLOAT;
typedef BOOL           		*PBOOL;
typedef BOOL                *LPBOOL;
typedef BYTE                *PBYTE;
typedef BYTE                *LPBYTE;
typedef int                 *PINT;
typedef int                 *LPINT;
typedef WORD                *PWORD;
typedef WORD                *LPWORD;
typedef long                *LPLONG;
typedef DWORD               *PDWORD;
typedef DWORD               *LPDWORD;
typedef void                *LPVOID;
typedef const void          *LPCVOID;

typedef const char *LPCSTR, *PCSTR;

typedef unsigned long ULONG_PTR, *PULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

typedef int                 INT;
typedef unsigned int        UINT;
typedef unsigned int        *PUINT;

#include <limits.h>
#include <stdlib.h>
#define INFINITE 	ULONG_MAX

#ifndef NOMINMAX

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#endif  /* NOMINMAX */

#define MAKEWORD(a, b)      ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#define LOWORD(l)           ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l)           ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w)           ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w)           ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))

typedef int SOCKET;
typedef void *HANDLE;


#include <time.h>
static inline DWORD GetTickCount(void)
{
	time_t t = time(NULL);
	
	if(t != (time_t) -1)
		return t/1000;
	
	return 0;
}

#include <string.h>
#include <strings.h>

static inline char *strtok_s(
   char *strToken,
   const char *strDelimit,
   char **context
)
{
	return strtok_r(strToken, strDelimit, context);
}

static inline int stricmp(const char *s1, const char *s2)
{
	return strcasecmp(s1, s2);
}

static inline int strnicmp(const char *s1, const char *s2, size_t n)
{
	return strncasecmp(s1, s2, n);
}


#define INVALID_HANDLE_VALUE ((HANDLE)-1)

#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)

#define WM_USER       0x0400

#define ZeroMemory(Destination,Length) memset((Destination),0,(Length))

#include <pthread.h>
#ifdef WIN32
#else
typedef pthread_mutex_t CRITICAL_SECTION;
typedef CRITICAL_SECTION *LPCRITICAL_SECTION;

void InitializeCriticalSection(CRITICAL_SECTION *section);
void EnterCriticalSection(CRITICAL_SECTION *section);
void LeaveCriticalSection(CRITICAL_SECTION *section);
void DeleteCriticalSection(CRITICAL_SECTION *section);

HANDLE CreateMutex( void* lpMutexAttributes,
                BOOL bInitialOwner, const char* lpName);
DWORD  WaitForSingleObject_Mutex(HANDLE hHandle,
                DWORD dwMilliseconds);
BOOL  ReleaseMutex(HANDLE hMutex);
BOOL  CloseHandle_Mutex( HANDLE hObject);


// 因为涉及到了进程间通信，所以这里的Event事件不能使用
// pthread的条件变量来做，建议用信号量来实现

HANDLE CreateEvent(void* lpEventAttributes,
                BOOL bManualReset,BOOL bInitialState,
                const char* lpName);
BOOL ResetEvent( HANDLE hEvent);
BOOL SetEvent( HANDLE hEvent);
DWORD WaitForSingleObject_Event(HANDLE hHandle,
                DWORD dwMilliseconds);
BOOL WINAPI CloseHandle_Event(HANDLE hObject);

#endif	

#include <unistd.h>
static inline int closesocket( SOCKET s)
{
	close(s);
	return 0;
}

#include <errno.h>
static inline DWORD GetLastError(void)
{
	return errno;
}

static inline int WSAGetLastError(void)
{
	return errno;
}

void Sleep( DWORD dwMilliseconds);

BOOL get_workdir( char* store);

#ifdef WIN32
#include <process.h>
#else
#include <pthread.h>
#endif

typedef struct timeval TIMEVAL;
typedef struct timeval *PTIMEVAL;

#define  WAIT_TIMEOUT  -1

#define WSAEFAULT               EFAULT
#define WSANOTINITIALISED       10093
#define WSAENETDOWN             ENETDOWN
#define WSAEFAULT               EFAULT
#define WSAEINPROGRESS          10036
#define WSAEINVAL               EINVAL
#define WSAENETRESET            ENETRESET
#define WSAENOPROTOOPT          ENOPROTOOPT
#define WSAENOTCONN             ENOTCONN
#define WSAENOTSOCK             ENOTSOCK

#ifdef __cplusplus
}
#endif

#endif /* _WINDEF_ */


