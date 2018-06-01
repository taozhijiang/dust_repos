#ifndef _ST_MYSQL_CONN_POOL_H
#define _ST_MYSQL_CONN_POOL_H

// For Database
#include <mysql/mysql.h>  
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "st_others.h"

#ifdef __cplusplus 
extern "C" {
#endif //__cplusplus 


typedef struct _mysql_option
{
    char hostname[128];
    char username[128];
    char password[128];
    char database[128];
} MYSQL_OPTION, *P_MYSQL_OPTION;


typedef struct  _mysql_conn 
{
    MYSQL mysql;
    unsigned free;  // =0 绌洪棽杩炴帴
} MYSQL_CONN, *P_MYSQL_CONN;

typedef struct  _mysql_conns 
{
    pthread_mutex_t mutex;
    int len;
    int pipe_read; //需要取用链接的时候，读取pipe
    int pipe_write; //释放链接的时候，写入pipe
    P_MYSQL_CONN conns;
} MYSQL_CONNS, *P_MYSQL_CONNS;

P_MYSQL_CONN mysql_get_conn(P_MYSQL_CONNS p_conns);
void mysql_free_conn(P_MYSQL_CONNS p_conns, P_MYSQL_CONN p_conn);
P_MYSQL_CONNS mysql_conns_init(unsigned int conn_num, P_MYSQL_OPTION pdata);
RET_T mysql_conns_destroy(P_MYSQL_CONNS p_conns);


RET_T mysql_fetch_string(MYSQL *mysql,const char *create_definition,
    char* result_store);
RET_T mysql_fetch_num(MYSQL *mysql,const char *create_definition,    
    unsigned long* result_store);

RET_T mysql_check_exist_r(MYSQL *mysql,const char *create_definition);
RET_T mysql_exec_sql(MYSQL *mysql,const char *create_definition);

int mysql_conns_capacity_r(P_MYSQL_CONNS p_conns);
int st_mysql_conn_test(void);



static inline char* mysql_scape_str(char* from, MYSQL * mysql)
{
    char str_buff[2048];

    mysql_real_escape_string(mysql, str_buff, from, strlen(from));
    strcpy(from, str_buff);

    return from;
}



#ifdef __cplusplus 
}
#endif //__cplusplus 

#endif //_ST_MYSQL_CONN_POOL_H
