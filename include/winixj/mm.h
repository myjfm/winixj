/***************************************************************
 * 本文件主要定义内存管理器用到的常量、变量和结构
 * 包括每页内存大小、页目录表和页表首地址等
 *
 * Author : myjfm(mwxjmmyjfm@gmail.com)
 * v0.01 2011/12/05
 ***************************************************************/


#ifndef __WINIXJ_MM_H__
#define __WINIXJ_MM_H__

#include <type.h>

#define PAGE_SIZE 4096

extern uint32* page_dir;
extern uint32* page_tbl;
extern uint32 MEMORY_START;//普通内存从2M开始，前两M为内核使用
extern uint32 MEMORTY_END;	//普通内存的末端
extern uint16 externed_mem;

extern init_mm();

#endif

