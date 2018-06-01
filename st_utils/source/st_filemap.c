/**
 * 主要用户模拟windows下面的有名和匿名内存映射共享
 */

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/mman.h>

#include "st_others.h"
#include "st_interface.h"
    

static const char* ST_MEMMAP_PREFIX = "/tmp/ST_MEMMAP/";

void* st_memmap_create(const char* filename, const char* share_name, size_t max_size)
{
    char path_buf[PATH_MAX];
    P_ST_MEMMAP_T p_token = NULL;
    mode_t	mode = 0;
    int	    flag = 0;
    FILE*   fp = NULL;
    char    rw_buf[512];

    if (!share_name)
        return NULL;

    if ( access(ST_MEMMAP_PREFIX, W_OK) != 0)
        mkdir(ST_MEMMAP_PREFIX, 0777);

    snprintf(path_buf, PATH_MAX, "%s/%s", ST_MEMMAP_PREFIX, share_name);

    if ( access(path_buf, W_OK) == 0)
    {
        st_d_print("Created %s has already exists, anyway delete it first!\n",
                   share_name);
        remove(path_buf);
    }

    p_token = (P_ST_MEMMAP_T)malloc(sizeof(ST_MEMMAP_T));
    if (! p_token)
    {
        remove(path_buf);
        return NULL;
    }

    memset(p_token, 0, sizeof(ST_MEMMAP_T));
    p_token->fd = -1;
    p_token->size = max_size;
    strncpy(p_token->mapname, share_name, PATH_MAX);

    if (filename)
    {
        strncpy(p_token->filename, filename, PATH_MAX);
        mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;  //同用户同组应该够了
        flag = O_CREAT | O_RDWR | O_SYNC;

        if((p_token->fd = open(p_token->filename, flag, mode)) < 0)
        {
            st_d_print("Open file failed:%s\n", p_token->filename);
            goto failed;
        }
    }
    else
    {
        strncpy(p_token->filename, ST_MEMMAP_PREFIX, PATH_MAX);
        strncat(p_token->filename, share_name, PATH_MAX);
        strncat(p_token->filename, "_FILE", PATH_MAX);

        remove(p_token->filename);
        mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
        flag = O_CREAT | O_TRUNC | O_RDWR | O_SYNC;
        
        if((p_token->fd = open(p_token->filename, flag, mode)) < 0)
        {
            st_d_print("Open file failed:%s\n", p_token->filename);
            goto failed;
        }

        //Just test memmap size OK
        int ch = '\0';
        lseek(p_token->fd, p_token->size - 1, SEEK_SET);
        write(p_token->fd, &ch, sizeof(char));
        if(lseek(p_token->fd, 0, SEEK_END) < p_token->size)
        {	
            st_print("LSEEK test failed:%lu\n", p_token->size);
            goto failed;
        }
    }

    // Just for safe
    lseek(p_token->fd, 0, SEEK_SET);

    p_token->location = mmap( NULL, p_token->size,
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED, p_token->fd, 0);
    if ((void *)p_token->location == (void *)MAP_FAILED)
    {
        st_print("Call mmap failed:%s\n", p_token->filename);
        goto failed;
    }
    
    //Store the address in mapfile for referenec
    strncpy(path_buf, ST_MEMMAP_PREFIX, PATH_MAX);
    strncat(path_buf, p_token->mapname, PATH_MAX);

    if((fp = fopen(path_buf, "w")))
    {
        snprintf(rw_buf, sizeof(rw_buf), "ADDR:%p,SIZE:%lu,FILE:%s\n", p_token->location, 
                 p_token->size, p_token->filename);
        fwrite(rw_buf, strlen(rw_buf)+1, 1, fp);
        fclose(fp);
    }

    st_d_print("ShareName:%s,\t FilePath:%s,\t Location:%p,\t MapSize:%lu\n", 
               p_token->mapname, p_token->filename, p_token->location, p_token->size);

    return p_token;
     
failed:
    st_memmap_destroy(p_token);
    return NULL;
}

void* st_memmap_open(const char* share_name, int fixaddr, int writable)
{
    char path_buf[PATH_MAX];
    P_ST_MEMMAP_T p_token = NULL;
    mode_t	mode = 0;
    int	    flag = 0;
    int map_flag = 0;
    FILE*     fp = NULL;
    char rw_buf[512];

    if (!share_name)
        return NULL;

    p_token = (P_ST_MEMMAP_T)malloc(sizeof(ST_MEMMAP_T));
    if (! p_token)
        return NULL;
    memset(p_token, 0, sizeof(ST_MEMMAP_T));

    p_token->fd = -1;
    snprintf(path_buf, PATH_MAX, "%s/%s", ST_MEMMAP_PREFIX, share_name);
    if((fp = fopen(path_buf, "r")))
    {
        rewind(fp);
        fscanf(fp,"ADDR:%p,SIZE:%lu,FILE:%s\n", &p_token->location,
               &p_token->size, p_token->filename);
        fclose(fp);
        st_d_print("MAPINFO ADDR:%p,SIZE:%lu,FILE:%s\n", p_token->location,
               p_token->size, p_token->filename);
    }

    mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
    if (writable)
    {
        flag = O_RDWR | O_SYNC; 
        map_flag = PROT_READ | PROT_WRITE;
    }
    else
    {
        flag = O_RDONLY | O_SYNC;
        map_flag = PROT_READ;
    }

    if((p_token->fd = open(p_token->filename, flag, mode)) < 0)
    {
        st_d_print("Open file failed:%s\n", p_token->filename);
        goto failed;
    }

    if (fixaddr)
        p_token->location = mmap(p_token->location, p_token->size,
                      map_flag,
                      MAP_SHARED | MAP_FIXED, p_token->fd, 0);
    else
        p_token->location = mmap(NULL, p_token->size,
                      map_flag,
                      MAP_SHARED, p_token->fd, 0);

    if ((void *)p_token->location == (void *)MAP_FAILED)
    {
        st_print("Call mmap failed:%s\n", p_token->filename);
        goto failed;
    }

    return p_token;

failed:
    st_memmap_close(p_token);

    return NULL;
}

void st_memmap_close(P_ST_MEMMAP_T p_token)
{
    if (!p_token)
        return;

    if (p_token->location != MAP_FAILED &&
        p_token->location != 0)
        munmap(p_token->location, p_token->size);

    if(p_token->fd != -1)
        close(p_token->fd);

    return; 
}

void st_memmap_destroy(P_ST_MEMMAP_T p_token)
{
	char path_buf[PATH_MAX];
	
    if ( ! p_token)
        return;

    st_memmap_close(p_token); 

    // Be very carefully!
    if (!strncmp( p_token->filename, ST_MEMMAP_PREFIX, strlen(ST_MEMMAP_PREFIX)))
    {
        st_print("Removing tmp map file:%s\n", p_token->filename);
        remove(p_token->filename);
    }
	
	strncpy(path_buf, ST_MEMMAP_PREFIX, PATH_MAX);
    strncat(path_buf, p_token->mapname, NAME_MAX);
	remove(path_buf);

    free(p_token);

    return;
}

struct mmap_struct 
{
    int  i;
    char buf[128];
    char c;
};

void st_memmap_test(void)
{
    P_ST_MEMMAP_T p_token = NULL;
    p_token = st_memmap_create(NULL, "RPC_SHARE", 4096);

    //st_memmap_open("RPC_SHARE", 0, 0);

    struct mmap_struct* p_st = NULL;
    if ( p_token)
    {
        p_st = (struct mmap_struct*) p_token->location;
        p_st->i = 0x77889;
        p_st->c = 'T';
        strcpy(p_st->buf, "TAOZHIJIANG TEST");

        st_print("TEST_DATA:\n");
        st_print("i:%xd\t c:%c\t str:%s\n", p_st->i, p_st->c, p_st->buf);
    }

    getchar();

    st_print("Rechecking data:\n");

    st_print("i:%x\t c:%c\t str:%s\n", p_st->i, p_st->c, p_st->buf);


    st_memmap_destroy(p_token);

}

void st_memmap_test2(void)
{
    P_ST_MEMMAP_T p_token = NULL;
    p_token = st_memmap_open("RPC_SHARE", 1, 1);

    struct mmap_struct* p_st = NULL;

    if ( p_token)
    {
        p_st = (struct mmap_struct*) p_token->location;

        st_print("CHECKING MAP DATA:\n");
        st_print("i:%x\t c:%c\t str:%s\n", p_st->i, p_st->c, p_st->buf);
    }

    st_print("Changing the data from another process!\n");
    p_st->i = 0xAA991;
    p_st->c = 'J';
    strcpy(p_st->buf, "NEW INFO HERE!");

    st_memmap_close(p_token);
}
