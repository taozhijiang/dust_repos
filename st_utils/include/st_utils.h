#ifndef _ST_UTILS_H
#define _ST_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


char* utf8_to_gbk(char *strUTF8, int size);
char* gbk_to_utf8(char *strGBK, int size);
void st_lowcase_string(char* buf);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_ST_UTILS_H
