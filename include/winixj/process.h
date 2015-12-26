/***************************************************************
 * 本文件主要定义一些进程实现以及进程管理用到的数据结构
 * 包括全局描述符表、中断描述符表、任务状态段、进程链表等
 * 其中全局描述符表和中断描述符表在init.s和int.s中定义，在
 * 此仅仅做声明
 *
 * Author : myjfm(mwxjmmyjfm@gmail.com)
 * v0.01 2011/11/22
 ***************************************************************/


#ifndef __WINIXJ_PROCESS_H__
#define __WINIXJ_PROCESS_H__

#include <type.h>
#include <winixj/mailbox.h>

/*
 *
 *
 *
 * 下面定义方法从linux0.11“拿”来的
 *
 *
 *
 */

//定义段描述符结构，每个描述符占用8个字节
typedef struct seg_struct
{
	uint16	seg_limit_low16;
	uint16	seg_base_low16;
	uint8	seg_base_mid8;
	uint8	attr1;
	uint8	attr2_limit_high4;
	uint8	seg_base_high8;
} SEG_TABLE[256]; //全局段描述符表有256项

//gdt在init.s文件中定义，在此是引用声明
extern SEG_TABLE gdt;

#define KERNEL_CS_SELECTOR 0x08
#define KERNEL_DS_SELECTOR 0x10
#define KERNEL_SS_SELECTOR KERNEL_DS_SELECTOR

//定义门描述符结构，每个描述符占用8个字节
typedef struct gate_struct
{
	uint32 a;
	uint32 b;
} GATE_TABLE[256]; //中断门描述符表有256项

//idt在int.s文件中定义，在此是引用声明
extern GATE_TABLE idt;

//进程所拥有的内核态以及用户态堆栈
//大小为1KB，短期内应该够用
typedef struct kstack
{
	uint8 res[1024];
} KSTACK, USTACK;

typedef struct tss_struct
{
	//上一任务的TSS段描述符的选择子，仅使用低16位，高16位为0
	//仅当eflags的NT位为1时有效
	uint32 back_link;
	uint32 esp0;
	uint32 ss0;					//高16位为0
	uint32 esp1;
	uint32 ss1;					//高16位为0
	uint32 esp2;
	uint32 ss2;					//高16位为0
	uint32 cr3;					//主要为分页机制准备的
	uint32 eip;
	uint32 eflags;
	uint32 eax;
	uint32 ecx;
	uint32 edx;
	uint32 ebx;
	uint32 esp;
	uint32 ebp;
	uint32 esi;
	uint32 edi;
	uint32 es;					//高16位为0
	uint32 cs;					//高16位为0
	uint32 ss;					//高16位为0
	uint32 ds;					//高16位为0
	uint32 fs;					//高16位为0
	uint32 gs;					//高16位为0
	uint32 ldt;					//高16位为0
	uint32 trace_bitmap;		//高16位为0
} TSS;

//目前的进程控制块结构不妨尽量简单
//因为我们要实现的是简陋的进程，请忍受这一点
typedef struct task_struct
{
	uint32	state;					//进程状态
	uint32	priority;				//进程优先级
	uint32	time_slices;			//进程的剩余时间片
	uint32	pid;					//进程的pid
	uint32	ppid;					//父进程的pid
#define PROC_NAME_LEN	32			//进程名的最大长度为32字节
	char	name[PROC_NAME_LEN];
	uint32	*kstack;				//指向内核态堆栈顶
	uint32	*ustack;				//指向用户态堆栈顶
	uint32	running_time;			//进程一共运行了的时间
	struct seg_struct ldt[3];		//进程的局部描述符表一项为空、一项为cs、一项为ds和ss
	TSS tss;
} PROCESS;

//WINIXJ系统最多可同时启动的进程数
#define NR_PROCS	125
//每个进程将在gdt表中占用2项---1项是tss，1项是ldt
//因此gdt中的组织如下：
//DUMMY
//kernel-cs
//kernel-ds
//unused
//process-0-cs
//process-0-ds
//process-1-cs
//process-1-ds
//process-2-cs
//process-2-ds
//...
//...
//process-124-cs
//process-124-ds
//unused
//unused
//进程链表，保存所有的进程控制块信息
extern PROCESS proc_list[NR_PROCS];
//指向当前正在运行或马上将被调度运行的进程
extern PROCESS *current;

//所有进程所拥有的内核态堆栈都放到一起
//进程的内核态堆栈的作用主要是用来保存
//进程被打断时的现场信息
extern KSTACK kstack_list[NR_PROCS];

//所有进程所拥有的用户态堆栈也都放到一起
extern USTACK ustack_list[NR_PROCS];

//gdt中第一个进程的tss描述符索引
#define FIRST_TSS_INDEX	4
//gdt中第一个进程的ldt描述符索引
#define FIRST_LDT_INDEX	(FIRST_TSS_INDEX + 1)

//计算在gdt中第n个进程的tss描述符的选择子
#define TSS_SELECTOR(n) ((((uint32)n) << 4) + (FIRST_TSS_INDEX << 3))

//计算在gdt中第n个进程的ldt描述符的选择子
#define LDT_SELECTOR(n) ((((uint32)n) << 4) + (FIRST_LDT_INDEX << 3))

//获得得n个进程的内核态堆栈的栈顶
#define KSTACKTOP(n) ((uint32)((uint8 *)(&kstack_list[n + 1]) - 1))

//获得得n个进程的用户态堆栈的栈顶
#define USTACKTOP(n) ((uint32)((uint8 *)(&ustack_list[n + 1]) - 1))

//初始化进程链表和current变量
extern void init_proc_list();
//启动第一个进程也即0号init进程
extern void start_proc0();

#endif

