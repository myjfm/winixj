#include <type.h>
#include <asm/system.h>
#include <winixj/mm.h>
#include <winixj/process.h>
#include <winixj/buffer.h>

BUF_INFO buf_info;

void init_buffer()
{
	buf_info.end	= (externed_mem << 10) + 0x100000;
	buf_info.end	&= 0xfffff000; //内存以4KB对齐

	if (buf_info.end < 0x400000)	//如果总内存数小于4MB，则无缓冲区
	{
		buf_info.start = buf_info.end - 0;
		halt();
	}
	else if (buf_info.end < 0x1000000)	//如果总内存在4MB～16MB之间则缓冲区为1MB
	{
		buf_info.start = buf_info.end - 0x100000;
	}
	else if (buf_info.end < 0x2000000)	//如果总内存在16MB～32MB之间则缓冲区为4MB
	{
		buf_info.start = buf_info.end - 0x400000;
	}
	else	//如果总内存在32MB～64MB之间则缓冲区为6MB
	{
		buf_info.start = buf_info.end - 0x600000;
	}

	buf_info.data	= (void *)buf_info.start;
	buf_info.occ	= NULL;
	buf_info.len	= 0;
	buf_info.valid	= INVALID;
	buf_info.locked	= UNLOCKED;
}

//获取缓冲区
//对于硬盘读操作使用
void *read_get_buffer()
{
	//如果缓冲区被锁定则进程选择忙等待
	while (buf_info.locked == LOCKED) {}

	buf_info.occ	= current;
	buf_info.locked	= LOCKED;
	buf_info.valid	= INVALID;
	return buf_info.data;
}

//获取缓冲区
//对于硬盘写操作使用
void *write_get_buffer()
{
	//在调用hd_write()函数时缓冲区应当已经
	//被相应进程锁定，并且将想要写入的数据
	//写入缓冲区
	if (buf_info.locked == UNLOCKED || 
			buf_info.occ != current || 
			buf_info.valid != VALID)
	{
		return NULL;
	}

	return buf_info.data;
}

//释放缓冲区
void release_buffer()
{
	//如果缓冲区不是被当前进程锁定则说明内核出错
	if (buf_info.occ != current || 
			buf_info.locked == UNLOCKED)
	{
		halt();
	}

	buf_info.occ	= NULL;
	buf_info.locked	= UNLOCKED;
	buf_info.valid	= INVALID;
}

