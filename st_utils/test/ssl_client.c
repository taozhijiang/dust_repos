#include <stdio.h>

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>


#include "st_others.h"
#include "st_openssl.h"

#include "st_interface.h"

static const char* CERT_FILE = "./ssl/local.pem";
static const char* PKEY_FILE = "./ssl/local.key";
static const char* CA_FILE   = "";
// Client NO CA, just verify!
//static const char* CA_FILE   = "./ssl/cacert.pem";

int main(int argc, char* argv[])
{
    
    ST_TLS_STRUCT st_tls;
    P_ST_TLS_STRUCT p_st_tls = &st_tls;

    memset(p_st_tls, 0, sizeof(ST_TLS_STRUCT));
    st_tls.work_status = WORK_CLIENT;
    strncpy(st_tls.cert_file, CERT_FILE, PATH_MAX);
    strncpy(st_tls.key_file,  PKEY_FILE, PATH_MAX);
    if ( CA_FILE && strlen(CA_FILE) > 0)
    {
        strncpy(st_tls.ca_file,   CA_FILE,   PATH_MAX); 
    }

    if( !st_tls_create_ctx(p_st_tls) )
        SYS_ABORT("TLS init FAILED!\n");
    

    int sock = socket (AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family      = AF_INET;
    srv_addr.sin_addr.s_addr = inet_addr ("192.168.80.131");  /* Server IP */
    srv_addr.sin_port        = htons     (44303);             /* Server Port number */

    if (connect(sock, (struct sockaddr*) &srv_addr, sizeof(srv_addr)) == -1)
        return -1;

    char buf[512];
    SSL* p_ssl;
    ssize_t count;
    int i = 0;

	if ( !(p_ssl = st_tls_create_ssl(p_st_tls, sock)) )
	{
        st_print("Error on st_tls_create_ssl\n");
        return -1;
    }
    
	printf ("SSL connection using %s\n", SSL_get_cipher (p_ssl));

    while ( i < 5 )
    {
        snprintf(buf, sizeof(buf), "THIS IS HELLO FROM CLIENT(TIMES %d)!\n", i);
        count = SSL_write(p_ssl, buf, strlen(buf)); 
        count = SSL_read (p_ssl, buf, sizeof(buf));
        if ( count > 0)
            write(1, buf, count);
        
        sleep(2);
		i++;
    }
    
    close(sock);
    SSL_free(p_ssl);
	st_tls_destroy(p_st_tls);
}
