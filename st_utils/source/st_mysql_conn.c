#include "st_mysql_conn.h"
#include "st_others.h"

#include <string.h>

// RET_YES OK
// RET_NO  FAILED
RET_T mysql_exec_sql(MYSQL *mysql,const char *create_definition)
{
    if (!mysql || !create_definition || !strlen(create_definition))
        return RET_NO;

    if(mysql_real_query(mysql, create_definition, strlen(create_definition)) == 0)
        return RET_YES;
    else
        return RET_NO;
}

// RET_YES if exist
// RET_NO  not exist
RET_T mysql_check_exist_r(MYSQL *mysql,const char *create_definition)
{
    MYSQL_RES *result;
    RET_T ret = RET_NO;

    if(mysql_exec_sql(mysql, create_definition) == RET_NO)
    {
        st_d_print("exec sql failed:%s", create_definition);  
        st_d_print(mysql_error(mysql)); 
        return RET_NO;
    }

    result = mysql_store_result(mysql);
    if (!result)
    {
        st_d_print("mysql_store_result failed!");  
        st_d_print(mysql_error(mysql)); 
        return RET_NO;
    }

    if(mysql_affected_rows(mysql) != 0)
        ret = RET_YES;

    mysql_free_result(result);

    return ret;
}

/*
 * RET_YES  success, and num stored in result_store 
 * RET_NO   failed 
 */
RET_T mysql_fetch_num(MYSQL *mysql,const char *create_definition,
    unsigned long* result_store)
{
    MYSQL_RES *result;
    MYSQL_ROW row;
    RET_T ret = RET_NO;

    if(mysql_exec_sql(mysql, create_definition) != RET_YES)
    {
        st_d_print("exec sql failed:%s", create_definition);  
        st_d_print(mysql_error(mysql)); 
        return RET_NO;
    }

    result = mysql_store_result(mysql);
    if (!result)
    {
        st_d_print("mysql_store_result failed!");  
        st_d_print(mysql_error(mysql)); 
        return RET_NO;
    }

    if( (row = mysql_fetch_row(result)) )
    {
        *result_store = atoi(row[0]);
        ret = RET_YES;
    }

    mysql_free_result(result);
    return ret;
}


/*
 * RET_YES  success, and string copied in result_store 
 * RET_NO   failed 
 */
RET_T mysql_fetch_string(MYSQL *mysql,const char *create_definition,
    char* result_store)
{
    MYSQL_RES *result;
    MYSQL_ROW row;
    RET_T ret = RET_NO;

    if(mysql_exec_sql(mysql, create_definition) != RET_YES)
    {
        st_d_print("exec sql failed:%s\n", create_definition);  
        st_d_print(mysql_error(mysql)); 
        return RET_NO;
    }

    result = mysql_store_result(mysql);
    if (!result)
    {
        st_d_print("mysql_store_result failed!");  
        st_d_print(mysql_error(mysql)); 
        return RET_NO;
    }

    if( (row = mysql_fetch_row(result)) )
    {
        st_d_print(row[0]);
        strcpy(result_store, row[0]);
        ret = RET_YES;
    }

    mysql_free_result(result);
    return ret;
}

//失败返回NULL
P_MYSQL_CONNS mysql_conns_init(unsigned int conn_num, P_MYSQL_OPTION pdata)
{
    P_MYSQL_CONNS p_conns = NULL;
    int i = 0;

    if (!pdata || !conn_num )
        return NULL;

    int pipefd[2];
    if(pipe(pipefd) != 0)
    {
        st_d_error("Create communicate pipe failed!");
        return NULL;
    }

    p_conns = (P_MYSQL_CONNS)malloc(sizeof(MYSQL_CONNS));
    if (!p_conns)
        return NULL;

    p_conns->len = conn_num;
    p_conns->conns  = (P_MYSQL_CONN)calloc(sizeof(MYSQL_CONN), conn_num);
    if (!p_conns->conns)
    {
        free(p_conns);
        return NULL;
    }

    pthread_mutex_init(&p_conns->mutex, NULL);
    p_conns->pipe_read = pipefd[0];
    p_conns->pipe_write = pipefd[1];
    P_MYSQL_CONN conns = p_conns->conns;

    for (i=0; i<conn_num; ++i)
    {
        if(mysql_init(&conns[i].mysql)==NULL)
        {
            st_d_print("Failed to initate MySQL connection[%d]!", i);
            goto failed;
        }
   
        conns[i].free = 1;
        write(p_conns->pipe_write, "Q", 1);
            
        if ( mysql_real_connect(&conns[i].mysql,
            pdata->hostname,pdata->username,
            pdata->password,pdata->database,0,NULL,0) == NULL)
        {
            st_d_print("Connect to database failed[%d]!", i);
            mysql_error(&conns[i].mysql);
            goto failed;
        }

        // setup the characterset
        if (!mysql_set_character_set(&conns[i].mysql, "utf8"))
        {
            st_d_print("MySQL client character set: %s",
                   mysql_character_set_name(&conns[i].mysql));
        }
    }

    return p_conns;

failed:
    pthread_mutex_destroy(&p_conns->mutex);
    free(p_conns->conns);
    free(p_conns);
    return NULL;
}

/** 
 * return RET_YES
 */
RET_T mysql_conns_destroy(P_MYSQL_CONNS p_conns)
{
    int i = 0;
    if (!p_conns)
        return 0;

    for (i=0; i<p_conns->len; ++i)
        mysql_close(&p_conns->conns[i].mysql);

    close(p_conns->pipe_read);
    close(p_conns->pipe_write);
    free(p_conns->conns);
    free(p_conns);

    return RET_YES;
}

P_MYSQL_CONN mysql_get_conn(P_MYSQL_CONNS p_conns)
{
    int i = 0;
    P_MYSQL_CONN ret_conn = NULL;
    char read_buf;

    if (!p_conns || !p_conns->len)
        return NULL;

retry:    
    read(p_conns->pipe_read, &read_buf, 1);    
    pthread_mutex_lock(&p_conns->mutex);
    for (i=0; i<p_conns->len; ++i)
    {
        if (p_conns->conns[i].free)
        {
            ret_conn = &p_conns->conns[i];
            p_conns->conns[i].free = 0;
            break;
        }
    }

    if (!ret_conn)
    {
        st_d_error("Block read returned, but no available conns!");
        pthread_mutex_unlock(&p_conns->mutex);
        goto retry;
    }

    pthread_mutex_unlock(&p_conns->mutex);

    return ret_conn;
}

void mysql_free_conn(P_MYSQL_CONNS p_conns, P_MYSQL_CONN p_conn)
{
    int i = 0;

    if (!p_conns || !p_conns->len)
        return;

    if (!p_conn)
        return;

    pthread_mutex_lock(&p_conns->mutex);
    for (i=0; i<p_conns->len; ++i)
    {
        if (&p_conns->conns[i] == p_conn)
        {
            p_conns->conns[i].free = 1;
            write(p_conns->pipe_write, "Q", 1);
            break;
        }
    }
    pthread_mutex_unlock(&p_conns->mutex);

    return;
}


int mysql_conns_capacity_r(P_MYSQL_CONNS p_conns)
{
    int i = 0;
    int free_c = 0;
    int total_c = 0;

    if (!p_conns || !p_conns->len)
        return 0;


    pthread_mutex_lock(&p_conns->mutex);
    for (i=0; i<p_conns->len; ++i)
    {
        if (p_conns->conns[i].free == 1)
            ++ free_c;
        ++ total_c;
    }
    pthread_mutex_unlock(&p_conns->mutex);

    st_d_print("MySQL Conns free:%d, total:%d", free_c, total_c);

    return free_c;
}



int st_mysql_conn_test(void)
{

    MYSQL_OPTION mysql_data;
    strncpy(mysql_data.hostname,"192.168.122.1",128);
    strncpy(mysql_data.username,"v5kf",128);
    strncpy(mysql_data.password,"v5kf",128);
    strncpy(mysql_data.database,"v5kf",128);

    P_MYSQL_CONN p_conn[5];

    P_MYSQL_CONNS g_p_conns = mysql_conns_init(10, &mysql_data);
    if (!g_p_conns)
    {
        st_d_print("Failed to initate MySQL connection pool!");
        exit(-1);
    }

    st_d_print("Database prepared OK!\n");

    if( mysql_conns_capacity_r(g_p_conns) != 10)
        goto FAILED;

    p_conn[0] = mysql_get_conn(g_p_conns);
    p_conn[1] = mysql_get_conn(g_p_conns);
    p_conn[2] = mysql_get_conn(g_p_conns);
    p_conn[3] = mysql_get_conn(g_p_conns);

    if( mysql_conns_capacity_r(g_p_conns) != 6)
        goto FAILED;


    mysql_free_conn(g_p_conns, p_conn[2]);
    mysql_free_conn(g_p_conns, p_conn[0]);

    if( mysql_conns_capacity_r(g_p_conns) != 8)
        goto FAILED;

    mysql_conns_destroy(g_p_conns);
    if( mysql_conns_capacity_r(g_p_conns) != 0)
        goto FAILED;

    fprintf(stderr,"mysql_conn unit test PASS!\n");
    return 1;

FAILED:

    fprintf(stderr,"mysql_conn unit test FAILED!\n");
    return 0;
}
