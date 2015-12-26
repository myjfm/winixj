/***************************************************************
 * 本文件主要工作为重命名一些数据类型
 * 包括常用的无符号单字节型、无符号短整型等等
 *
 * Author : myjfm(mwxjmmyjfm@gmail.com)
 * v0.01 2011/11/22
 ***************************************************************/


#ifndef __TYPE_H__
#define __TYPE_H__

// 定义数据类型
typedef unsigned char		uint8;
typedef unsigned short		uint16;
typedef unsigned int		uint32;
typedef unsigned long long	uint64;

typedef uint16				mail_t;
typedef int					size_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#endif

