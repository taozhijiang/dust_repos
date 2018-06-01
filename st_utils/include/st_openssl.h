#ifndef _ST_OPENSSL_H_
#define _ST_OPENSSL_H_

#ifdef __cplusplus 
extern "C" {
#endif //__cplusplus 

#include <openssl/crypto.h>
#include <openssl/dh.h>
#include <openssl/dsa.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/opensslv.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <linux/limits.h>
#include <pthread.h>

#include <sys/utsname.h>
#include <unistd.h>
#include <sys/types.h>

#include "st_others.h"

typedef struct ssl_ctx_st SSL_CTX, *P_SSL_CTX;

enum _ST_WORK_STATUS {
    WORK_SERVER, 
    WORK_CLIENT, 
    WORK_GENERAL
};

typedef struct _st_tls_ctx_struct {
    char    cert_file[PATH_MAX];
    char    key_file[PATH_MAX];
    char    ca_file[PATH_MAX];
    enum    _ST_WORK_STATUS work_status;
    P_SSL_CTX    p_ctx;  //唯一的对象
} ST_TLS_STRUCT, *P_ST_TLS_STRUCT;


P_ST_TLS_STRUCT st_tls_create_ctx(P_ST_TLS_STRUCT p_st_tls);
SSL* st_tls_create_ssl(P_ST_TLS_STRUCT p_st_tls, int sock);
void st_tls_destroy(P_ST_TLS_STRUCT p_st_tls);
int st_tls_verify_cert_with_CA(const char* certfile, X509* certX, const char* CAfile, 
				STACK_OF(X509) *tchain, STACK_OF(X509_CRL) *crls);
X509* st_tls_build_cert_from_str_S(const char* pemCertString);

static inline void tls_rand_seed_uniquely(void)
{
    struct {
        pid_t pid;
        time_t time;
        void *stack;
    } data;

    data.pid = getpid();
    data.time = time(NULL);
    data.stack = (void *)&data;

    RAND_seed((const void *)&data, sizeof data);
}

static inline void tls_rand_seed(void)
{
    struct {
        struct utsname uname;
        int uname_1;
        int uname_2;
        uid_t uid;
        uid_t euid;
        gid_t gid;
        gid_t egid;
    } data;

    data.uname_1 = uname(&data.uname);
    data.uname_2 = errno;       /* Let's hope that uname fails randomly :-) */

    data.uid = getuid();
    data.euid = geteuid();
    data.gid = getgid();
    data.egid = getegid();

    RAND_seed((const void *)&data, sizeof data);
    tls_rand_seed_uniquely();
}

#define st_ssl_error(...) \
	do{ fprintf( stderr,"!!!ERROR:%s|%s<%d>:",__FILE__, __FUNCTION__,__LINE__); \
		fprintf( stderr , __VA_ARGS__ ); \
        ERR_print_errors_fp(stderr);    \
	}while(0)


//// openSSL special

#define EXIT_IF_TRUE_S(x) if (x)                                \
    do {                                                        \
            fprintf(stderr, "!!!%s:%d ASSERT '%s' IS TRUE\n",   \
            __FILE__, __LINE__, #x);                            \
            ERR_print_errors_fp(stderr);                        \
            SYS_ABORT(#x);                                      \
    }while(0)  

#define RET_NULL_IF_TRUE_S(x) if (x)                            \
    do {                                                        \
            fprintf(stderr, "!!!%s:%d ASSERT '%s' IS TRUE\n",   \
            __FILE__, __LINE__, #x);                            \
            ERR_print_errors_fp(stderr);                        \
            return NULL;                                        \
    }while(0)  

#define GOTO_IF_TRUE_S(x, flag) if (x)                          \
    do {                                                        \
            fprintf(stderr, "!!!%s:%d ASSERT '%s' IS TRUE\n",   \
            __FILE__, __LINE__, #x);                            \
            ERR_print_errors_fp(stderr);                        \
            goto flag;                                          \
    }while(0)



typedef struct _st_RSA_AES_struct {
    RSA     *p_pubkey;  //客户端用
    RSA     *p_prikey;  //服务器用
    char    aes_str[33];    //客户端产生
    int     socket;     //保留对端的socket，用于更新密钥等操作
    pthread_mutex_t mutex;
} ST_RSA_AES_STRUCT, *P_ST_RSA_AES_STRUCT;
P_ST_RSA_AES_STRUCT st_RSA_AES_setup_srv(const char* prikey_file,
                                         const P_ST_SMALL_OBJ p_aes_obj);
P_ST_RSA_AES_STRUCT st_RSA_AES_setup_cli(const char* pubkey_file,
                                         P_ST_SMALL_OBJ p_aes_obj);
ST_SMALL_POBJ st_AES_encrypt_S(const char* data, size_t len, P_ST_RSA_AES_STRUCT p_st);
size_t st_AES_decrypt(char* data, size_t len, P_ST_RSA_AES_STRUCT p_st);
static X509 *st_tls_load_cert(const char *file);
void st_tls_test(void);


#ifdef __cplusplus 
}
#endif //__cplusplus 

#endif //_ST_OPENSSL_H_
