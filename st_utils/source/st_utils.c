#include "st_others.h"
#include "st_utils.h"

#include <string.h>
#include <iconv.h>
#include <ctype.h>

#define MAX_LINE 2048

// 将字符串中的英文字符全部小写
// 采用UTF-8编码
/*
    00000000 -- 0000007F:   0xxxxxxx
    00000080 -- 000007FF:   110xxxxx 10xxxxxx
    00000800 -- 0000FFFF:   1110xxxx 10xxxxxx 10xxxxxx
    00010000 -- 001FFFFF:   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
*/
#define IS_IN_RANGE(c, f, l)    (((c) >= (f)) && ((c) <= (l)))
#define UTF8_CHAR_LEN( byte )   ((( 0xE5000000 >> (( byte >> 3 ) & 0x1e )) & 3 ) + 1)


//删除buf语句开头和结尾的空白字符
void st_strip(char* buf, size_t len)
{
	char tmp_buf[MAX_LINE];
	char* ptr = NULL,* p_end = NULL;
	size_t l_len = (len+1) > MAX_LINE ? (len+1) : MAX_LINE;

	if(!buf || strlen(buf) < 1)
		return;

	strncpy(tmp_buf, buf, l_len);

	ptr = tmp_buf + len - 1;
	while(ptr >= tmp_buf)
	{
		if(isspace(* ptr))
			ptr --;
		else
			break;
	}

	p_end = ptr;
	*(p_end+1) = '\0';

	ptr = tmp_buf;
	while(ptr <= p_end)
	{
		if(isspace(* ptr))
			ptr ++;
		else
			break;
	}

	//strncpy(buf, ptr, l_len);
	strncpy(buf, ptr, len);

	return;
}

void st_lowcase_string(char* buf)
{
    while(*buf != '\0')
    {
        //ascii 字符
        if(UTF8_CHAR_LEN(*buf) == 1)
        {
            if(IS_IN_RANGE(*buf, 0x41, 0x5a))
                *buf = *buf | 0x20;
            buf += 1;
            continue;
        }

        // 多字符
        buf += UTF8_CHAR_LEN(*buf);
    }
}

int st_getline(char* buf, FILE* fp)
{
	static char dirty[MAX_LINE];
	int ret = 0;

	if(!buf || !fp)
		return -1;

	memset(buf, 0, MAX_LINE);
	if( fgets( buf, MAX_LINE, fp ) == NULL)
	{
		if( feof(fp) )
			return 0;
		else
			return -1;
	}

	if(buf[MAX_LINE-2] == '\n' || buf[MAX_LINE-2] == '\0')
		return 1;

	do{
		memset(dirty, 0, MAX_LINE);
		if(fgets(dirty, MAX_LINE, fp))
		{
			if(dirty[MAX_LINE-2] == '\0' || dirty[MAX_LINE-2] == '\n')
				break;
		}
		else
			break;
	}while(1);

	return 1;
}

static char* code_convert(char* src, int size   ,
                          const char* tocode, const char* fromcode)
{
    char*  in_buf = NULL; char* p_in = NULL;
    char*  out_buf = NULL; char* p_out = NULL;
    size_t in_len = strlen(src);
    size_t out_len = in_len * 4;
    size_t conv_cnt = 0;
    iconv_t cd = 0;

    if (!src || !tocode || !fromcode)
        return NULL;

    in_buf = strdup(src);
    if (!in_buf)
        return NULL;
    out_buf = (char *)malloc(out_len);
    if (!out_buf)
    {
        free(in_buf);
        return NULL;
    }
    p_in = in_buf;
    p_out = out_buf;

    // 忽略输入中非法的字符。测试过程中有这种情况，看实际的效果
    GOTO_IF_TRUE ( (cd = iconv_open(tocode, fromcode)) == (iconv_t)-1, failed);

    memset(out_buf, 0, out_len);
    conv_cnt = iconv(cd, &p_in, &in_len, &p_out, &out_len);
    iconv_close(cd);

    //Perfect ONE
    if ( conv_cnt == 0 && strlen(out_buf) < size)
    {
        //st_print("GOOD: code_convert(%s->%s)[%s]",
        //fromcode, tocode, out_buf);
        memset(src, 0, size);
        strcpy(src, out_buf);
    }
    else if ( strlen(out_buf) > 0)
    {
        st_print("WARNING: code_convert(%s->%s)[%s]",
                 fromcode, tocode, out_buf);
        memset(src, 0, size);
        strcpy(src, out_buf);
    }
    else
    {
        st_print("ERROR: code_convert(%s->%s)[%s]",
                 fromcode, tocode, src);
        goto failed;
    }
    free(in_buf);
    free(out_buf);
    return src;

failed:
    free(in_buf);
    free(out_buf);
    return NULL;
}

// 以下两个函数是字符编码的转换，如果转换成功，传入的地址
// 的内容将会被更新，同时被return返回，否则原先的内容不会
// 被改变，返回NULL
char* utf8_to_gbk(char *strUTF8, int size)
{
    return code_convert(strUTF8, size, "GBK//IGNORE", "UTF-8");
}

char* gbk_to_utf8(char *strGBK, int size)
{
    return code_convert(strGBK, size,"UTF-8//IGNORE", "GBK" );
}



void utf8_gbk_test(void)
{
    //char buf[512] = "外边的字到底怎么为什么有的时候通不过呢";
    //utf8_to_gbk(buf, sizeof(buf));
    //gbk_to_utf8(buf, sizeof(buf));

    char line[MAX_LINE];
    int ret = 0;
    FILE* fp = fopen("input.txt","r");
    FILE* fp_gbk = fopen("out.gbk", "w");

    rewind(fp);
    while((ret = st_getline(line, fp)) > 0)
	{
        utf8_to_gbk(line, MAX_LINE);
        fputs(line, fp_gbk);
    }
    fclose(fp);
    fclose(fp_gbk);

    fp_gbk = fopen("out.gbk", "r");
    FILE* fp_utf8 = fopen("out.utf8", "w");
    rewind(fp_gbk);
    while((ret = st_getline(line, fp_gbk)) > 0)
    {
        gbk_to_utf8(line, MAX_LINE);
        fputs(line, fp_utf8);
    }


    fclose(fp_gbk);
    fclose(fp_utf8);

    st_print("TEST TERMINATED!\n");

    return;
}

