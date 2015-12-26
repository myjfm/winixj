#include <type.h>
#include <asm/system.h>
#include <winixj/clock.h>
#include <winixj/mm.h>
#include <winixj/hdd.h>
#include <winixj/sys_call.h>
#include <winixj/process.h>
#include <getpid.h>

//指向在loader.s中保存的系统参数表
//包括显示卡参数、硬盘参数等等
void *sys_param = (void *)0xf0000;
volatile int cursor_pos = 0;



/*
 * cbegin()函数的入口地址为0x6a4
 */



//这个函数是真正的第一个C函数，从_start函数中跳入
//最好将cbegin函数用volatile关键字修饰，这样做的好处
//是：用volatile修饰函数的话则是告诉gcc编译器该函数
//不返回（可能是函数内含有exit()或者死循环之类的），这样
//gcc在函数优化的时候就不会将返回值压入堆栈，这样，起到
//优化的作用，在cbegin中start_proc0()启动了第一个init进程
//下面的for循环永远不可能执行到，就算执行到那cbegin()也不会
//返回
void cbegin()
{
	//初始化系统调用，包括将0x30号中断与自陷框架sys_call函数挂钩
	//以及安装对用系统调用号的中断入口函数
	init_sys_call();
	//初始化proc_list进程链表以及对init和sys进程进行初始化
	init_proc_list();
	//初始化心跳值为0，以及打开时钟中断、初始化开机时间等
	init_clock();
	init_mm();

	//这里调试了很久！！！！！！！！！！
	//注意这里一定要开中断，因为当前中断是关闭的
	//init_hd()中有涉及到硬盘中断的操作
	//如果中断关闭那么将响应不到硬盘中断
	sti();
	init_hd();
	cli();

	//启动第一个进程也即0号init进程
	start_proc0();

	//下面的代码应该永远不会被执行，因为start_proc0()函数不会返回，
	//start_proc0()函数执行完iretd命令后便启动了第一个init进程，
	//自此系统便开始了多进程的运行
	for(;;){}
}

//proc0
//第一个进程
void init()
{
	int i;
	uint8 *p = (uint8 *)(0xb8000);
	*p = (uint8)getpid() + '0';

	for(;;)
	{
		for ( i = 0; i < 1000000; ++i);

		if (*p < '9')
		{
			*p = ++(*p);
		}
	};
}

//proc1
//第二个进程
void sys()
{
	int i;
	//这里由sys进程调用partition()系统调用来完成硬盘的初始化
	//之所以在不在内核中完成硬盘的初始化是因为这里需要读取硬盘
	//MBR的内容，而读取硬盘需要将当前进程睡眠，而内核是不允许
	//睡眠的，因此选择在sys进程中初始化硬盘
	//这里的工作包括获取硬盘柱面、柱头、磁道、每磁道扇区数等的
	//信息
	uint8 *p = (uint8 *)(0xb8002);
	*p = (uint8)getpid() + '0';

	for(;;)
	{
		for ( i = 0; i < 1000000; ++i);

		if (*p < '9')
		{
			*p = ++(*p);
		}
	};
}

