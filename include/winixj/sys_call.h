#ifndef __WINIXJ_SYS_CALL_H__
#define __WINIXJ_SYS_CALL_H__

//#define SYS_CALL_IV	0x30

//系统调用最大数量
#define NR_SYSCALLS 256
extern void *sys_call_table[NR_SYSCALLS];

//系统调用入口函数
extern void sys_call();

extern void install_sys_call(int number, void *handler);
extern void uninstall_sys_call(int number);

#include <type.h>

//系统调用函数集
extern int default_sys_call();
extern int sys_call_partition();
extern int sys_call_getpid();

#endif

