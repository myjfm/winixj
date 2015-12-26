/***************************************************************
 * 本文件声明高速缓冲区的相关结构
 * 目前的高速缓冲区比较简单，因为本系统的目的是快速搭建起一个可
 * 运行系统，所以高速缓冲区没有像linux、unix那样实现的很精妙
 * 而仅仅是将划出来的缓冲区做为一个大的缓冲池，这样操作缓冲区的
 * 进程就不得不串行，不过这在将来可以很容易进行扩展
 *
 * Author : myjfm(mwxjmmyjfm@gmail.com)
 * v0.01 2012/01/04
 ***************************************************************/


#ifndef __WINIXJ_BUFFER_H__
#define __WINIXJ_BUFFER_H__

#include <type.h>
#include <winixj/process.h>

#if 0
//磁盘高速缓冲区块结构
typedef struct buffer_struct
{
	uint8 dev;//设备号
	uint32 blk_num;//块号
	uint8 dirty;//该页面是否脏
	uint8 locked;//该缓冲区块是否被某进程占用
	uint32 wait_pid;//等待该缓冲区的进程pid
	void *data;//指向的数据(512B每块)
	struct buffer_struct *next;//指向下一个空闲高速缓冲区链表项
	struct buffer_struct *prev;//指向上一个空闲高速缓冲区链表项
	struct buffer_struct *hash_next;//指向下一个空闲高速缓冲区链表项
	struct buffer_struct *hash_prev;//指向上一个空闲高速缓冲区链表项
} BUFFER;
#endif

//高速缓冲区控制块
typedef struct buf_info
{
	uint32 start;//高速缓冲区起始物理地址
	uint32 end;	//高速缓冲区起始物理地址
	uint32 len; //高速缓冲区中有效数据的长度
	PROCESS *occ;//占用缓冲区的进程
	void *data; //指向缓冲区起始地址
	uint8 valid; //缓冲区数据是否准备好的标志
	uint8 locked;//标志高速缓冲区是否为可用的标志
	//关于ready和locked标志有如下不同：
	//ready是用于进程在读取硬盘数据之后等待硬盘中断所使用的
	//标志位，如果ready为0，则说明硬盘数据尚未操作完成，如果
	//ready位置1则说明硬盘数据操作已经完成，进程可以进行其他操作
	//locked标志用于互斥进程的，这里把buf_info做为临界区，一旦一个
	//进程拥有则将locked标志置1，直到其释放，其他进程才有机会使用它
} BUF_INFO;

extern BUF_INFO buf_info;
//缓冲区总长度
#define BUF_LEN (buf_info.end - buf_info.start)
#define VALID 1//数据是有效的，说明此时缓冲区中的数据是dirty的，等待某个进程读取
#define INVALID 0//数据是无效的，说明此时缓冲区空闲
#define ERROR -1//此标志用来表示对硬盘的操作有错误发生
#define LOCKED 1//此缓冲区正在被某个进程占用，别的进程不能抢占
#define UNLOCKED 0//此时缓冲区空闲

extern void init_buffer();
extern void *read_get_buffer();
extern void *write_get_buffer();
extern void release_buffer();

#endif

