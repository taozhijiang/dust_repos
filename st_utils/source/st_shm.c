/**
 * 因为filemap必须映射到物理文件，这会导致一些安全和
 * 信息泄露的风险，使用shm不会有这个问题
 *
 * 使用POSIX共享内存实现
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
    
static const char* ST_SHM_PREFIX = "/tmp/ST_SHM/";	

// POSIX要求的共享名字必须是  /sharename  < NAME_MAX
//
void* st_shm_create(const char* share_name, size_t max_size)
{
	char path_buf[PATH_MAX];
	P_ST_SHM_T p_token = NULL;
	FILE* fp = NULL;
	char    rw_buf[512];
	
    if (!share_name)
        return NULL;
	
    p_token = (P_ST_SHM_T)malloc(sizeof(ST_SHM_T));
    if (! p_token)
    {
        return NULL;
    }
	
	if ( access(ST_SHM_PREFIX, W_OK) != 0)
        mkdir(ST_SHM_PREFIX, 0777);

	// Additional
	shm_unlink(share_name);

    memset(p_token, 0, sizeof(ST_SHM_T));
	p_token->location = NULL;
    p_token->fd = -1;
    p_token->size = max_size;
	
	// For compatible
	if(share_name[0] != '/')
	{
		p_token->shmname[0] = '/';
		strncpy(p_token->shmname + 1, share_name, NAME_MAX);
	}
	else
	{
		strncpy(p_token->shmname, share_name, NAME_MAX);
	}

	p_token->fd = shm_open(p_token->shmname, O_RDWR|O_CREAT|O_EXCL|O_TRUNC, 
		S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	if( p_token->fd < 0)
	{
		perror("Create shm_open failed!");
		goto failed;
	}
    
    if( ftruncate(p_token->fd, max_size) == -1)
	{
		perror("ftruncate share mem failed!");
		goto failed;
	}

    p_token->location = mmap( NULL, p_token->size,
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED, p_token->fd, 0);
    if ((void *)p_token->location == (void *)MAP_FAILED)
    {
        st_print("Call mmap failed:%s\n", p_token->shmname);
        goto failed;
    }
		
	//Store the address in mapfile for referenec
    strncpy(path_buf, ST_SHM_PREFIX, PATH_MAX);
    strncat(path_buf, p_token->shmname, NAME_MAX);
	if ( access(ST_SHM_PREFIX, W_OK) == 0)
	{
		st_d_print("shm file exists, delete it anyway!\n");
		remove(path_buf);
	}
	
    if((fp = fopen(path_buf, "w")))
    {
        snprintf(rw_buf, sizeof(rw_buf), "ADDR:%p,SIZE:%lu\n", p_token->location, 
                 p_token->size);
        fwrite(rw_buf, strlen(rw_buf)+1, 1, fp);
        fclose(fp);
    }

    st_d_print("ShareName:%s,\t Location:%p,\t MapSize:%lu\n", 
               p_token->shmname, p_token->location, p_token->size);
		   
    return p_token;
     
failed:
    free(p_token);
    return NULL;
}

void* st_shm_open(const char* share_name, int fixaddr, int writable)
{
    P_ST_SHM_T p_token = NULL;
    mode_t	mode = 0;
    int	    flag = 0;
	int map_flag = 0;
	FILE*     fp = NULL;
    char rw_buf[512];
	char path_buf[PATH_MAX];

    mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
    if (writable)
    {
        flag = O_RDWR ; 
        map_flag = PROT_READ | PROT_WRITE;
    }
    else
    {
        flag = O_RDONLY;
        map_flag = PROT_READ;
    }
	
	p_token = (P_ST_SHM_T)malloc(sizeof(ST_SHM_T));
    if (! p_token)
        return NULL;
    
	memset(p_token, 0, sizeof(ST_SHM_T));
    p_token->location = NULL;
    p_token->fd = -1;
    p_token->size = 0;
    // For compatible
	if(share_name[0] != '/')
	{
		p_token->shmname[0] = '/';
		strncpy(p_token->shmname + 1, share_name, NAME_MAX);
	}
	else
	{
		strncpy(p_token->shmname, share_name, NAME_MAX);
	}
	
	p_token->fd = shm_open(p_token->shmname, flag, mode);
	if( p_token->fd < 0)
	{
		perror("Create shm_open failed!");
		goto failed;
	}
	
    snprintf(path_buf, PATH_MAX, "%s/%s", ST_SHM_PREFIX, share_name);
    if((fp = fopen(path_buf, "r")))
    {
        rewind(fp);
        fscanf(fp,"ADDR:%p,SIZE:%lu\n", &p_token->location,
               &p_token->size);
        fclose(fp);
        st_d_print("MAPINFO ADDR:%p,SIZE:%lu\n", p_token->location,
               p_token->size);
    }
	else
	{
		st_print("Open shm info failed with %s\n", path_buf);
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
        perror("Call mmap failed:\n");
        goto failed;
    }

    return p_token;

failed:
    free(p_token);
    return NULL;
}

void st_shm_close(P_ST_SHM_T p_token)
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

void st_shm_destroy(P_ST_SHM_T p_token)
{
	char path_buf[PATH_MAX];
	
    if ( ! p_token)
        return;

    st_shm_close(p_token); 

    // Be very carefully!
	shm_unlink(p_token->shmname);
	
	strncpy(path_buf, ST_SHM_PREFIX, PATH_MAX);
    strncat(path_buf, p_token->shmname, NAME_MAX);
	remove(path_buf);
	
    free(p_token);

    return;
}

struct shm_struct 
{
    int  i;
    char buf[128];
    char c;
};

void st_shm_test(void)
{
    P_ST_SHM_T p_token = NULL;
    p_token = st_shm_create("RPC_SHARE", 4096);

    //st_memmap_open("RPC_SHARE", 0, 0);

    struct shm_struct* p_st = NULL;
    if ( p_token)
    {
        p_st = (struct shm_struct*) p_token->location;
        p_st->i = 0x77889;
        p_st->c = 'T';
        strcpy(p_st->buf, "TAOZHIJIANG TEST");

        st_print("TEST_DATA:\n");
        st_print("i:%x\t c:%c\t str:%s\n", p_st->i, p_st->c, p_st->buf);
    }

    getchar();

    st_print("Rechecking data:\n");

    st_print("i:%x\t c:%c\t str:%s\n", p_st->i, p_st->c, p_st->buf);


    st_shm_destroy(p_token);

}

void st_shm_test2(void)
{
    P_ST_SHM_T p_token = NULL;
    p_token = st_shm_open("/RPC_SHARE", 1, 1);

    struct shm_struct* p_st = NULL;

    if ( p_token)
    {
        p_st = (struct shm_struct*) p_token->location;

        st_print("CHECKING MAP DATA:\n");
        st_print("i:%x\t c:%c\t str:%s\n", p_st->i, p_st->c, p_st->buf);
    }

    st_print("Changing the data from another process!\n");
    p_st->i = 0xAA991;
    p_st->c = 'J';
    strcpy(p_st->buf, "NEW INFO HERE!");

    st_shm_close(p_token);
}
