/***************************************************************
 * 本文件主要工作是声明两个指针变量用于指向页表和页目录表
 *
 * Author : myjfm(mwxjmmyjfm@gmail.com)
 * v0.01 2011/12/04
 ***************************************************************/


#include <type.h>
#include <winixj/buffer.h>

//定义指向页表和页目录表的指针，为全局变量
uint32* page_dir	= (uint32 *)(0x100000);
uint32* page_tbl	= (uint32 *)(0x101000);
uint32 MEMORY_START	= 0x200000;	//普通内存从2M开始，前两M为内核使用
uint32 MEMORY_END	= 0;		//普通内存的末端

//扩展内存的大小，以KB为单位
uint16 externed_mem = 0;

void init_mm()
{
	externed_mem = *(uint16 *)0xf0002;
	init_buffer();
	MEMORY_END = buf_info.start;  //普通内存末端为高速缓冲区的起始
}

