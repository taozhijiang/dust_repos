#ifndef __ST_OTHERS_H
#define __ST_OTHERS_H

#define DEBUG

#ifdef __cplusplus 
extern "C" {
#endif //__cplusplus 

#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>

#include <linux/kernel.h>

typedef unsigned long ulong;
typedef unsigned int  uint;
typedef unsigned char uchar;
typedef unsigned short ushort;

#include <sys/types.h>
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({                      \
             const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
             (type *)( (char *)__mptr - offsetof(type,member) );})

#ifdef FALSE
#undef FALSE
#endif
#define FALSE                           (1 == 0)

#ifdef TRUE
#undef TRUE
#endif
#define TRUE                            (1 == 1)

#ifndef NULL
#define NULL ((void *)0)
#endif

/**
 * 统一一下函数的返回类型，不要自己都搞晕了 
 *  
 * 如果函数的返回值只是表示成功或者失败，就用下面的类型 
 */
typedef enum RET_TYPE {
    RET_YES = 1,
    RET_NO  = 0,
} RET_T;


#ifdef DEBUG
#define st_d_print(...) \
	do{ fprintf( stderr,"DEBUG:%s|%s<%d>:",__FILE__, __FUNCTION__,__LINE__); \
		fprintf( stderr , __VA_ARGS__ ); \
	}while(0)
#define st_print(...) fprintf( stderr , __VA_ARGS__ )
#else
#define st_d_print(...) \
	do {} while(0)
#define st_print(...) fprintf( stderr , __VA_ARGS__ )
#endif

#define st_d_error(...) \
	do{ perror("ERROR:");	\
		fprintf( stderr,"DEBUG:%s|%s<%d>:",__FILE__, __FUNCTION__,__LINE__); \
		fprintf( stderr , __VA_ARGS__ ); \
	}while(0)

typedef struct _st_small_obj {
    char    data[2048];
    size_t  len;
} ST_SMALL_OBJ, *P_ST_SMALL_OBJ;

typedef struct _st_small_pobj {
    char*   data;
    size_t  len;
} ST_SMALL_POBJ, *P_ST_SMALL_POBJ;

#define WAIT_FOR_ENTER  fprintf( stderr, "Press ENTER\n" );getchar()


#define EXIT_IF_TRUE(x) if (x)                                  \
    do {                                                        \
            fprintf(stderr, "!!!%s:%d ASSERT '%s' IS TRUE\n",   \
            __FILE__, __LINE__, #x);                            \
            SYS_ABORT(#x);                                      \
    }while(0)  

#define RET_NULL_IF_TRUE(x) if (x)                              \
    do {                                                        \
            fprintf(stderr, "!!!%s:%d ASSERT '%s' IS TRUE\n",   \
            __FILE__, __LINE__, #x);                            \
            return NULL;                                        \
    }while(0)  

#define GOTO_IF_TRUE(x, flag) if (x)                            \
    do {                                                        \
            fprintf(stderr, "!!!%s:%d ASSERT '%s' IS TRUE\n",   \
            __FILE__, __LINE__, #x);                            \
            goto flag;                                          \
    }while(0) 

static inline void backtrace_info(int param)
{
	int j, nptrs;
#define BT_SIZE 100
	char **strings;
    void *buffer[BT_SIZE];
	
	nptrs = backtrace(buffer, BT_SIZE);
	fprintf(stderr, "backtrace() returned %d addresses\n", nptrs);
	
	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL) 
	{
		perror("backtrace_symbols");
		exit(EXIT_FAILURE);
    }

    for (j = 0; j < nptrs; j++)
        fprintf(stderr, "%s\n", strings[j]);

    free(strings);
}

#define SYS_ABORT(...)  \
    do{ fprintf( stderr,"!!!DIE:%s|%s<%d>\n",__FILE__, __FUNCTION__,__LINE__); \
		fprintf( stderr , __VA_ARGS__ ); \
        backtrace_info(0); abort();    \
    }while(0)



#ifdef __cplusplus 
}
#endif //__cplusplus 

#endif  //__ST_OTHERS_H
