#include <stdio.h>
#include "st_openssl.h"

void st_mutex_test_intra(void);
void st_event_comsumer_test(void);
void st_event_thread_test(void);
void st_shm_test(void);
void st_tls_test(void);
void st_utils_timer_test(void);
void utf8_gbk_test(void);

const char* pem_str = "-----BEGIN CERTIFICATE-----\n\
MIIEJjCCAw6gAwIBAgIBATANBgkqhkiG9w0BAQsFADCBnTELMAkGA1UEBhMCQ04x\n\
EjAQBgNVBAgMCUd1YW5nZG9uZzERMA8GA1UEBwwIU2hlbnpoZW4xFTATBgNVBAoM\n\
DEZyZWVTaWduIEx0ZDEMMAoGA1UECwwDUiZEMRwwGgYDVQQDDBNmZWRvcmEuZnJl\n\
ZXNpZ24ubmV0MSQwIgYJKoZIhvcNAQkBFhV0YW96aGlqaWFuZ0BnbWFpbC5jb20w\n\
HhcNMTUxMTIwMDY0NTM2WhcNMTgxMTE5MDY0NTM2WjCBkTELMAkGA1UEBhMCQ04x\n\
EjAQBgNVBAgMCUd1YW5nZG9uZzEZMBcGA1UECgwQVGVzdCBDb21wYW55IEx0ZDEY\n\
MBYGA1UECwwPVGVzdCBEZXBhcnRtZW50MRowGAYDVQQDDBF0ZXN0LmZyZWVzaWdu\n\
Lm5ldDEdMBsGCSqGSIb3DQEJARYOdHR0dEBnbWFpbC5jb20wggEiMA0GCSqGSIb3\n\
DQEBAQUAA4IBDwAwggEKAoIBAQDIL/2IsWOtJzpO+6LY7Ivk8ulO48H5RV+ZApLg\n\
mC/8gLA39hPTzqhNteRHrAQGoW64HyQ3pnvejxAxpo1kc1mY4R+7PhGOR333U68l\n\
wcAa3xyeuTVRW5KieH+m5EaSNlG2Rzbe/RvlO2GMBc0OBIase1NH6PQThzqWDSY5\n\
OcbShiYaA5t1vW3dmoM743hVpDUIg+0fVXQktdu94SQpgnFnzvJMCcEhOKQbKEs9\n\
Q/oPw4wpIy5+wfB/WK2lgaZLyjif44XQJ8YwVKzUC/UklnVMetMtITT+pgiVdzuT\n\
IWs4I9Er4XZumFsVRm79VehBgxvDB3LiRTDk48I8kKfAIlLTAgMBAAGjezB5MAkG\n\
A1UdEwQCMAAwLAYJYIZIAYb4QgENBB8WHU9wZW5TU0wgR2VuZXJhdGVkIENlcnRp\n\
ZmljYXRlMB0GA1UdDgQWBBTvln+XGSuHda+y97RvWCYb6aoHPjAfBgNVHSMEGDAW\n\
gBSvZM0xqUz7XA3jxJEOqRzYzhmX5jANBgkqhkiG9w0BAQsFAAOCAQEAcpl29iP2\n\
TGNXmSfI6q2Qxa12Bi+xGWPINVMyDCWG+TLdFketi/LQzompfQjlEtrGoWBbEK65\n\
2mln6EMV3oCrT9mg/9szVNPNCdqdToIz6PbsWc/W48AvaQF8MWwlKnoNbUUwGMe9\n\
cv72S+AMc7Ntp2JN0DuD/+/yDWXiJK1KE6dPhx71/8WOuS7a+nVSpmo6lOuiveRp\n\
sPnnQgz2cGtNmxqpBvp8K/zGYrRalEwQ0TBoCCJJy5FU5nLwuyQhI0CBxLhQSJhm\n\
oD373ljzw/jqJWlqm/5qWLouC8E6MivoS1KM5VYe4AdMk3irbZiVonSJx4pvD/Li\n\
Ni/4LdJvefxTkw==\n\
-----END CERTIFICATE-----";				
				
int st_mysql_conn_test(void);

int main(int argc, char* argv[])
{
    //st_threadpool_test();
    //st_mutex_test_intra();

    //st_event_comsumer_test();

    //st_event_thread_test();
    //st_tls_test();
	
    //utf8_gbk_test(); 
	
    // st_tls_verify_cert_with_CA("./ssl/test.crt", NULL,  "./ssl/cacert.pem", 0, 0);
	
    // X509* certX = st_tls_build_cert_from_str_S(pem_str);
    // st_tls_verify_cert_with_CA( NULL, certX, "./ssl/cacert.pem", 0, 0);
	
    // X509_free(certX);
	
    //st_utils_timer_test();

    st_mysql_conn_test();
}
