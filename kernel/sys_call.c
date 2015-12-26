#include <type.h>
#include <string.h>
#include <winixj/hdd.h>
#include <winixj/int.h>
#include <winixj/process.h>
#include <winixj/sys_call.h>

void *sys_call_table[NR_SYSCALLS];

//默认的系统调用函数函数
int default_sys_call()
{
	return -1;
}

//获得进程pid的系统调用
int sys_call_getpid(int dummy1, int dummy2, int dummy3)
{
	return current->pid;
}

//将系统调用入口函数sys_call与中断号0x30挂钩
void init_sys_call()
{
	memset(sys_call_table, 0, sizeof(sys_call_table));
	install_sys_call_handler(SYS_CALL_IV, (void *)sys_call);

	int i = 0;

	for (i = 0; i < NR_SYSCALLS; ++i)
	{
		install_sys_call(i, (void *)default_sys_call);
	}

	install_sys_call(0, (void *)sys_call_getpid);
}

//将系统调用号与对应的内核态系统调用函数入口挂钩
void install_sys_call(int number, void *handler)
{
	if (number < 0 || number >= NR_SYSCALLS)
	{
		return;
	}

	sys_call_table[number] = handler;
}

//将系统调用号与对应的内核态系统调用函数入口拆卸
//这个函数应该很少用到
void uninstall_sys_call(int number)
{
	if (number < 0 || number >= NR_SYSCALLS)
	{
		return;
	}

	sys_call_table[number] = (void *)default_sys_call;
}

