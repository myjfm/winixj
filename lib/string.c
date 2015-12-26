/***************************************************************
 * 本文件主要工作是常用库函数包括
 * 字符串操作函数以及内存操作函数的实现
 *
 * Author : myjfm(mwxjmmyjfm@gmail.com)
 * v0.01 2011/12/04
 ***************************************************************/


#include <type.h>

char * strcpy(char *dst, const char *src)
{
	if (NULL == dst || NULL == src)
	{
		return NULL;
	}

	char *strd = dst;

	while ((*strd++ = *src++) != '\0');

	return dst;
}

void * memset(void *s, int num, unsigned int n)
{
	uint8 *t = (uint8 *)s;

	while (n--)
	{
		*t++ = num;
	}
}

