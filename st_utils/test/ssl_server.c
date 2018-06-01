#include <stdio.h>

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>


#include "st_others.h"
#include "st_openssl.h"

#include "st_interface.h"

static const char* CERT_FILE = "./ssl/local.pem";
static const char* PKEY_FILE = "./ssl/local.key";
static const char* CA_FILE   = "./ssl/cacert.pem";


int main(int argc, char* argv[])
{
    ST_TLS_STRUCT st_tls;
    P_ST_TLS_STRUCT p_st_tls = &st_tls;

    memset(p_st_tls, 0, sizeof(ST_TLS_STRUCT));
    st_tls.work_status = WORK_SERVER;
    strncpy(st_tls.cert_file, CERT_FILE, PATH_MAX);
    strncpy(st_tls.key_file,  PKEY_FILE, PATH_MAX);
    if ( CA_FILE && strlen(CA_FILE) > 0)
    {
        strncpy(st_tls.ca_file,   CA_FILE,   PATH_MAX); 
    }

    if( !st_tls_create_ctx(p_st_tls) )
        SYS_ABORT("TLS init FAILED!\n");
    

    int lis_sock = st_buildsocket(8, 44303);
    if (lis_sock <= 0)
    {
        SYS_ABORT("st_buildsocket FAILED!\n");
    }


    struct sockaddr_in in_addr;
    socklen_t in_len;
    int infd;
    char buf[512];
    SSL* p_ssl;
    ssize_t count;
    const char* server_msg = "THIS IS SERVER HEAR AND REPLY MESSAGE!\n";

    while ( TRUE )
    {
		
		st_print("SERVER OK, WAITING FOR CONNECTION!!!\n");
	
        infd = accept (lis_sock, (struct sockaddr*) &in_addr, &in_len);
        if (infd == -1)
            continue;

        st_d_print("Accept Connection %lx, port %x\n",
                in_addr.sin_addr.s_addr, in_addr.sin_port);

        if ( !(p_ssl = st_tls_create_ssl(p_st_tls, infd)) )
        {
            st_print("Error on st_tls_create_ssl\n");
            continue;
        }
		
		while(TRUE)
		{
			count = SSL_read (p_ssl, buf, sizeof(buf) - 1);
			if ( count > 0)
			{
				write(1, buf, count);
				SSL_write (p_ssl, server_msg, strlen(server_msg));	//反馈信息
			}
			else  if( count == 0 )	// peer shutdown 
			{
				st_print("Peer Active Shutdown, give up this connection!\n");
				close(infd);
				SSL_free(p_ssl);
				break;	//next connection
			}
			else //error
			{
				SYS_ABORT("Error Detected!\n");
			}
		}
    }

    st_tls_destroy(p_st_tls);
	
	return 0;
}
