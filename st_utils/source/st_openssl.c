#include "st_openssl.h"
#include "st_others.h"

#include <openssl/md5.h>
#include <openssl/aes.h>
#include <openssl/x509v3.h>
#include <pthread.h>
#include <stdio.h>

P_ST_TLS_STRUCT st_tls_create_ctx(P_ST_TLS_STRUCT p_st_tls)
{
    P_SSL_CTX p_ctx = NULL;

    OpenSSL_add_ssl_algorithms();
    SSL_load_error_strings();
    SSL_library_init();     //SSL_library_init() always returns "1"

    tls_rand_seed();

    if ( p_st_tls->work_status == WORK_SERVER )
    {
        st_print("Initialize with TLSv1_server_method() \n");
        RET_NULL_IF_TRUE_S(!(p_ctx = SSL_CTX_new(TLSv1_server_method()))); 
    }
    else if ( p_st_tls->work_status == WORK_CLIENT )
    {
        st_print("Initialize with TLSv1_client_method() \n");
        RET_NULL_IF_TRUE_S(!(p_ctx = SSL_CTX_new(TLSv1_client_method())));
    }
    else
    {
        st_print("Initialize with TLSv1_method() \n");
        RET_NULL_IF_TRUE_S(!(p_ctx = SSL_CTX_new(TLSv1_method())));
    }


    //SSL_CTX_set_default_passwd_cb(p_ctx, no_passphrase_callback);
    SSL_CTX_set_mode(p_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);

    if ( strlen(p_st_tls->ca_file) )
    {
        RET_NULL_IF_TRUE_S( !SSL_CTX_load_verify_locations(p_ctx, p_st_tls->ca_file, NULL));
    }
    else
    {
        st_print("No CAfile Verify!\n");
    }

    RET_NULL_IF_TRUE_S( !SSL_CTX_use_PrivateKey_file(p_ctx, p_st_tls->key_file, SSL_FILETYPE_PEM)); 

    RET_NULL_IF_TRUE_S( !SSL_CTX_use_certificate_chain_file(p_ctx, p_st_tls->cert_file));

    // 判定私钥是否正确  
    RET_NULL_IF_TRUE_S( !SSL_CTX_check_private_key(p_ctx));

    p_st_tls->p_ctx = p_ctx;

    return p_st_tls;
}


SSL* st_tls_create_ssl(P_ST_TLS_STRUCT p_st_tls, int sock)
{
    char*   str = NULL;
    X509*   peer_cert = NULL;
    SSL*    p_ssl;

    if ( !p_st_tls || !p_st_tls->p_ctx || sock < 0)
    {
        st_d_print("Invalid argument!\n");
        return NULL;
    }

    RET_NULL_IF_TRUE_S( !(p_ssl = SSL_new (p_st_tls->p_ctx)) );
    RET_NULL_IF_TRUE_S( !SSL_set_fd (p_ssl, sock) );

    if ( p_st_tls->work_status == WORK_SERVER)
    {
        RET_NULL_IF_TRUE_S((SSL_accept(p_ssl) != 1)); 
    }
    else if ( p_st_tls->work_status == WORK_CLIENT)
    {
        RET_NULL_IF_TRUE_S((SSL_connect(p_ssl) != 1));
    }
    else
    {
        SYS_ABORT("YOU SHOULD NOT CALL THIS FUNC!!!\n");
    }
	
#if 1
    peer_cert = SSL_get_peer_certificate (p_ssl);

    if (peer_cert != NULL) 
    {
        str = X509_NAME_oneline (X509_get_subject_name (peer_cert), 0, 0);
        if(str)
        {
            st_print("OBJECT:%s\n", str);
            OPENSSL_free (str);
        }
    
        str = X509_NAME_oneline (X509_get_issuer_name  (peer_cert), 0, 0);
        if(str)
        {
            st_print("ISSUER:%s\n", str);
            OPENSSL_free (str);
        }
    
        /* We could do all sorts of certificate verification stuff here before
           deallocating the certificate. */
    
        X509_free (peer_cert);
    } 
    else
        st_d_print("Peer does not have certificate!!!\n");
#endif

    return p_ssl;
}

void st_tls_destroy(P_ST_TLS_STRUCT p_st_tls)
{
    if ( ! p_st_tls)
        return;

    if ( ! p_st_tls->p_ctx)
        SSL_CTX_free(p_st_tls->p_ctx); 
}

static X509 *st_tls_load_cert(const char *file)
{
    X509 *x=NULL;
    BIO *cert;

    if ((cert=BIO_new(BIO_s_file())) == NULL)
        goto end;

    if (BIO_read_filename(cert,file) <= 0)
        goto end;

    x=PEM_read_bio_X509_AUX(cert,NULL, NULL, NULL);
end:
    if (cert != NULL) BIO_free(cert);
    return(x);
}

X509* st_tls_build_cert_from_str_S(const char* pemCertString)
{
	if(!pemCertString)
		return NULL;
	
	size_t certLen = strlen(pemCertString);
	BIO* certBio = BIO_new(BIO_s_mem());
    BIO_write(certBio, pemCertString, certLen);
    
	X509* certX509 = PEM_read_bio_X509(certBio, NULL, NULL, NULL);
    if (!certX509) 
        st_ssl_error("unable to parse certificate in memory\n");

    BIO_free(certBio);
	
	return certX509;
}

// 用根证书来验证用户证书，成功返回1，失败0
// certfile 和 certStr不能同时指定
// 可不可以直接字符串比较certificate和issuer ？？？？
int st_tls_verify_cert_with_CA(const char* certfile, X509* certX, const char* CAfile, 
				STACK_OF(X509) *tchain, STACK_OF(X509_CRL) *crls)
{
	const int SHA1LEN = 20;
	int ret 		= 0, i = 0;
	X509 		*x 	= NULL;
    X509_STORE  *store = NULL;
	X509_STORE_CTX *csc = NULL;
    X509_LOOKUP *lookup = NULL;
	unsigned char	x_finger[SHA1LEN];
	unsigned char	x_finger_s[SHA1LEN*2 +1];
	unsigned int	len = 0;
	
	if(!CAfile || (!certfile && !certX) || (certfile && certX))
	{
		st_d_print("Invalid argument!");
		return -1;
	}
	
	GOTO_IF_TRUE ( !(store = X509_STORE_new()), end);

	OpenSSL_add_all_algorithms();
	
	// 自检CA证书的合法性 
	GOTO_IF_TRUE ( !(x = st_tls_load_cert(CAfile)), end);
	if( (i = X509_check_ca(x)) == 0)
	{
		st_ssl_error("Invalid Root CA %s!\n", CAfile);
		X509_free(x);
		return 0;
	}
	else
	{
		st_print("Root CA type:%d\n", i);
		X509_free(x);
	}
	
	// 加载CA证书
    GOTO_IF_TRUE ( !(lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file())), end);
	if (!X509_LOOKUP_load_file(lookup, CAfile, X509_FILETYPE_PEM))
	{
		st_ssl_error("Error loading file %s\n", CAfile);
        goto end;
	}
	
	// 如果是文件验证
	if(!certX)
	{
		GOTO_IF_TRUE ( !(x = st_tls_load_cert(certfile)), end);
	}
	else
	{
		x = certX;
	}
		
	GOTO_IF_TRUE ( !(csc = X509_STORE_CTX_new()), end_2);
	
	// 计算证书指纹，方便作为数据库主键的其它操作
	const EVP_MD *digest = EVP_sha1();
	i = X509_digest(x, digest, (unsigned char*) x_finger, &len);
	GOTO_IF_TRUE ( (i == 0 || len != SHA1LEN), end_2);
	memset(x_finger_s, 0, sizeof(x_finger_s));
	for(i=0; i < len; i++) { 
		char *l = (char*) (2*i +  x_finger_s); 
		sprintf(l, "%02X", x_finger[i]); 
	} 
	st_print("FINGER SHA1:%s\n", x_finger_s);
	
	// 计算验证证书的有效日期
	//x509_check_cert_time
	
	X509_STORE_set_flags(store, 0);	//?
    GOTO_IF_TRUE ( !X509_STORE_CTX_init(csc, store, x, 0/*untrust chain*/), end_3)	;//最后添加代验证的证书
	
	if (tchain)	/*trust chain*/
        X509_STORE_CTX_trusted_stack(csc, tchain);
    if (crls)	/*revoke chain*/
        X509_STORE_CTX_set0_crls(csc, crls);
	
    i = X509_verify_cert(csc);
	if(i > 0)
	{
		ret = 1; // OK
		st_print("Verify OK!\n");
		
		int j, k;
		int Nid = 0;
		STACK_OF(X509) *chain = X509_STORE_CTX_get1_chain(csc);
		
	    for (j = 0; j < sk_X509_num(chain); j++) 
		{
			X509 *cert = sk_X509_value(chain, j);
			char test_buf[256];
			X509_NAME* subject = X509_get_subject_name(cert);
			// subject MUST NOT be freed
			if ( X509_NAME_get_text_by_NID(subject, NID_organizationName, test_buf, sizeof(test_buf)) > 0)
				st_print("NID_organizationName=>%s\n", test_buf);
#if 0			
			char subj  [1024+1];
			char issuer[1024+1];
			X509_NAME_oneline(X509_get_subject_name(cert), subj, 1024);
			X509_NAME_oneline(X509_get_issuer_name(cert), issuer, 1024);
			st_print("certificate: %s\n", subj);
			st_print("\tissuer: %s\n\n", issuer);
#endif
			
#if 0			
			X509_NAME* subject = X509_get_subject_name(cert);
			X509_NAME_ENTRY *entry = NULL;
			int entry_cnt = sk_X509_NAME_ENTRY_num(subject->entries);
			
			for (k=0; k< entry_cnt; k++)
			{
				entry = sk_X509_NAME_ENTRY_value(subject->entries,k);
				Nid = OBJ_obj2nid(entry->object);
				switch(Nid)
				{
					case NID_countryName://国家C
						write(1,"国家：", strlen("国家："));
						write(1, entry->value->data, entry->value->length); write(1,"\n",1);
						break;
					case NID_stateOrProvinceName://省ST
						write(1,"省：", strlen("省："));
						write(1, entry->value->data, entry->value->length); write(1,"\n",1);
						break;
					case NID_localityName://地区L
						write(1,"地区：", strlen("地区："));
						write(1, entry->value->data, entry->value->length); write(1,"\n",1);
						break;
					case NID_organizationName://组织O
						write(1,"组织：", strlen("组织："));
						write(1, entry->value->data, entry->value->length); write(1,"\n",1);
						break;
					case NID_organizationalUnitName://单位OU
						write(1,"单位：", strlen("单位："));
						write(1, entry->value->data, entry->value->length); write(1,"\n",1);
						break;
					case NID_commonName://通用名CN
						write(1,"通用名：", strlen("通用名："));
						write(1, entry->value->data, entry->value->length); write(1,"\n",1);
						break;
					case NID_pkcs9_emailAddress://Mail
						write(1,"EMAIL：", strlen("EMAIL："));
						write(1, entry->value->data, entry->value->length); write(1,"\n",1);
						break;
					default:
						break;
				}
			}
			//st_print("depth=%d: \n", j);
			//X509_NAME_print_ex_fp(stderr,
			//	      X509_get_subject_name(cert),
			//	      0, XN_FLAG_ONELINE);
			
			st_print("\n");
#endif	   			
	    } 
		sk_X509_pop_free(chain, X509_free);
	}
	else
	{
		st_ssl_error("Verify Failed!\n");
	}
	
end_3:
    X509_STORE_CTX_free(csc);
end_2:
	if(!certX)	//的确是文件加载的
		X509_free(x);
end:
	if(store)
		X509_STORE_free(store);	
	
    return ret;
}

// RSA加密的时候，使用参数RSA_PKCS1_PADDING时，明文长度不能大于密文长度-11

//服务端加载私钥
P_ST_RSA_AES_STRUCT st_RSA_AES_setup_srv(const char* prikey_file,
                                         const P_ST_SMALL_OBJ p_aes_obj)
{
    P_ST_RSA_AES_STRUCT p_st = NULL;
    FILE* fp = NULL;
    char dep_buf[1024];

    p_st = (P_ST_RSA_AES_STRUCT) malloc(sizeof(ST_RSA_AES_STRUCT));
    
    if (!p_st || !prikey_file || !p_aes_obj || p_aes_obj->len <=0 )
        return NULL;

    memset(p_st, 0, sizeof(ST_RSA_AES_STRUCT));
    pthread_mutex_init(&p_st->mutex, NULL);
    GOTO_IF_TRUE( !(fp = fopen(prikey_file, "r")), failed );

    //RSA_private_decrypt RSA_PKCS1_PADDING
    p_st->p_prikey = RSA_new();
    GOTO_IF_TRUE_S ( !(PEM_read_RSAPrivateKey(fp, &p_st->p_prikey, 0, 0)), fail_2 );

    //解密aes_key
    memset(dep_buf, 0, sizeof(dep_buf));
    size_t len = RSA_private_decrypt(p_aes_obj->len, p_aes_obj->data, dep_buf,
                       p_st->p_prikey, RSA_PKCS1_PADDING);

    GOTO_IF_TRUE_S ( (len == -1), fail_2 );
    dep_buf[len] = '\0';
    strncpy(p_st->aes_str, dep_buf, len);

    fclose(fp);
    return p_st;
   
fail_2:
    fclose(fp);
    RSA_free(p_st->p_prikey);
failed:
    pthread_mutex_destroy(&p_st->mutex);
    free(p_st);
    return NULL;

    //RSA_public_encrypt RSA_PKCS1_PADDING
}

static char* random_str(void)
{
    char* ptr = (char *)malloc(128);
    if( !ptr)
        return NULL;

    time_t t = time(NULL);
    RAND_seed(&t, sizeof(time_t));

    RAND_bytes(ptr, 128);

    return ptr;
}

P_ST_RSA_AES_STRUCT st_RSA_AES_setup_cli(const char* pubkey_file,
                                         P_ST_SMALL_OBJ p_aes_obj)
{
    P_ST_RSA_AES_STRUCT p_st = NULL;
    unsigned char md5_data[16];
    char md5_str[33];
    FILE* fp = NULL;
    char* ptr = NULL;
    int	  i = 0;

    RET_NULL_IF_TRUE( !(ptr = random_str()) );
    p_st = (P_ST_RSA_AES_STRUCT) malloc(sizeof(ST_RSA_AES_STRUCT));
    
    if (!p_st || !pubkey_file || !p_aes_obj)
        return NULL;

    memset(p_st, 0, sizeof(ST_RSA_AES_STRUCT));
    pthread_mutex_init(&p_st->mutex, NULL);
    GOTO_IF_TRUE( !(fp = fopen(pubkey_file, "r")), failed );


    //后续随机化
    //产生MD5值，作为会话的随机密钥
    MD5(ptr, strlen(ptr), md5_data);
    free(ptr);

    memset(md5_str, 0, sizeof(md5_str));
    for(i = 0; i < 16; i++ )  
    {  
        char tmp[3];
        sprintf(tmp,"%02X", md5_data[i]); // sprintf并不安全  
        strcat(md5_str, tmp); // strcat亦不是什么好东西  
    }

    GOTO_IF_TRUE((strlen(md5_str) != 32), failed);
    strncpy(p_st->aes_str, md5_str, sizeof(md5_str)-1);
    //st_print("随机密钥:%s\n", md5_str);

    //RSA_private_decrypt RSA_PKCS1_PADDING
    p_st->p_pubkey = RSA_new();
    GOTO_IF_TRUE_S ( !(PEM_read_RSA_PUBKEY(fp, &p_st->p_pubkey, 0, 0)), fail_2 );

    memset(p_aes_obj->data, 0, sizeof(p_aes_obj->data));
    size_t len = RSA_public_encrypt(strlen(p_st->aes_str), p_st->aes_str, p_aes_obj->data,
                       p_st->p_pubkey, RSA_PKCS1_PADDING);

    GOTO_IF_TRUE_S ( (len == -1), fail_2 );
    p_aes_obj->len = len;

    fclose(fp);
    return p_st;
   
fail_2:
    fclose(fp);
    RSA_free(p_st->p_pubkey);
failed:
    pthread_mutex_destroy(&p_st->mutex);
    free(p_st);
    return NULL;
}

void st_RSA_AES_destroy(P_ST_RSA_AES_STRUCT p_aes_obj)
{
    if (!p_aes_obj)
        return;

    RSA_free(p_aes_obj->p_pubkey);
    pthread_mutex_destroy(&p_aes_obj->mutex);
    free(p_aes_obj);
    return; 
}

// 由于加密的数据肯定比原来的数据长，所以不覆盖原来的数据，调用者
// 自己判断是否覆盖原来的数据空间
// AES加密，块大小必须为128位，那么必须将数据的长度编码到内容中去
ST_SMALL_POBJ st_AES_encrypt_S(const char* data, size_t len, P_ST_RSA_AES_STRUCT p_st)
{
    size_t data_len = 0;
    int padding = 0;
    char*  data_en  = NULL;
    char*  data_out = NULL;
    AES_KEY aes_key;
    ST_SMALL_POBJ ret_pobj;
    int	i = 0;
    
    ret_pobj.data = NULL;
    ret_pobj.len  = 0;

    if (!data || len <= 0 || !p_st || !strlen(p_st->aes_str))
        return ret_pobj;

    if(AES_set_encrypt_key(p_st->aes_str, strlen(p_st->aes_str) * 8, &aes_key) < 0)
        return ret_pobj;

    padding = AES_BLOCK_SIZE - ( len + 2) % AES_BLOCK_SIZE; //resv 2byte for length
    data_len = 2 + len + padding;

    data_en = (char *)malloc( data_len );
    if (! data_en )
        return ret_pobj;

    data_out = (char *)malloc( data_len );
    if (! data_out )
    {
        free(data_en);
        return ret_pobj;
    }

    memset(data_out, 0, data_len);
    memset(data_en, 0, data_len);
    data_en[0] = len >> 8;
    data_en[1] = len & 0xFF;
    memcpy(data_en + 2, data, len); //allready padded

    for(i = 0; i < data_len/AES_BLOCK_SIZE; i++)
    {
        AES_encrypt( data_en + i*AES_BLOCK_SIZE , 
                     data_out + i*AES_BLOCK_SIZE , &aes_key);
    }

    free(data_en);
    
    ret_pobj.len = data_len;
    ret_pobj.data = data_out;

    return ret_pobj;
}

// 如果成功，返回0，覆盖原数据，返回解密后数据的长度
// 如果失败，返回-1，原来的数据不变
size_t st_AES_decrypt(char* data, size_t len, P_ST_RSA_AES_STRUCT p_st)
{
    size_t data_len = 0;
    char*  data_en  = NULL;
    char*  data_out = NULL;
    AES_KEY aes_key;
    int	i = 0;

    if (!data || len <= 0 || !p_st || !strlen(p_st->aes_str))
        return -1;

    if(AES_set_decrypt_key(p_st->aes_str, strlen(p_st->aes_str) * 8, &aes_key) < 0)
        return -1;

    data_en = (char *)malloc( len );
    if (! data_en )
        return -1;

    data_out = (char *)malloc( len );
    if (! data_out )
    {
        free(data_en);
        return -1;
    }

    memset(data_out, 0, len);
    memset(data_en, 0, len);
    memcpy(data_en, data, len); //allready padded

    for(i = 0; i < len/AES_BLOCK_SIZE; i++)
    {
        AES_decrypt( data_en + i*AES_BLOCK_SIZE , 
                     data_out + i*AES_BLOCK_SIZE , &aes_key);
    }

    data_len = data_out[0] << 8 | data_out[1];

    if ( (data_len%AES_BLOCK_SIZE) &&
         data_out[2+data_len] != '\0')
    {
        st_d_print("数据边界检查错误!!!!!\n");
        free(data_en);
        return -1;
    }

    memset(data, 0, len);
    memcpy(data, data_out+2, data_len);

    return data_len;
}

void st_tls_test(void)
{
    const char* pubkey = "./ssl/public.key";
    const char* prikey = "./ssl/local.key";

    //const char* msg = "桃子家de仔仔好帅啊，真的好帅啊啊啊！";
    const char* msg = "AAAAAAAA";

    P_ST_RSA_AES_STRUCT ps1 = NULL;
    P_ST_RSA_AES_STRUCT ps2 = NULL;
    ST_SMALL_OBJ aes;

    ps1 = st_RSA_AES_setup_cli(pubkey, &aes);
    ps2 = st_RSA_AES_setup_srv(prikey, &aes);

    ST_SMALL_POBJ pobj;

    pobj = st_AES_encrypt_S(msg, strlen(msg), ps1);
    
    st_print("原始数据加密长度:%lu\n", pobj.len);

    st_AES_decrypt(pobj.data, pobj.len, ps2);

    st_print("解密数据：%s\n", pobj.data);
    st_print("解密长度：%lu\n", pobj.len);
    
    free(pobj.data);

}
