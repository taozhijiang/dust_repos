#include <stdio.h>
#include "WINDEF.H"

#include "st_others.h"
#include "st_interface.h"


struct shm_struct 
{
    int  i;
    char buf[128];
    char c;
};

static void st_shm_test3(void)
{
    P_ST_SHM_T p_token = NULL;
    p_token = st_shm_open("RPC_SHARE", 1, 1);

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


static void st_mutex_sync_test(void)
{
    P_ST_MEMMAP_T p_token = st_memmap_open("RPC_SHARE", 1, 1);
    P_ST_WINSYNC_T p_mutex = (P_ST_WINSYNC_T)OpenMutex(0, 0, "RPC_MUTEX");

    int i = 0, nloop = 10;
    lseek(p_token->fd, 0, SEEK_SET);

    for (i = 0; i < nloop; i++) 
    {
        WaitForSingleObject(p_mutex, INFINITE);
        st_print("O_LOCK\n");
        write(p_token->fd, "OOOOO", 5);
        usleep(5000*1000);
        write(p_token->fd, "OOOOO", 5);
        st_print("O_UNLOCK\n");
        ReleaseMutex(p_mutex);
        usleep(4000*1000);
    }
}

void st_event_producer_test(void);

int main(int argc, char *argv[])
{
    st_shm_test3();

    //st_event_producer_test();

    return 0;
}
