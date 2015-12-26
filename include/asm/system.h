/***************************************************************
 * 本文件主要工作是定义一些宏，这些宏都是嵌入式汇编
 * 用于完成C语言很难或无法完成的工作
 * 包括加载ldtr、tr、将操作数压栈以及在手工启动第一个进程
 * 时用于初始化ds、es、gs、fs寄存器的操作。
 *
 * 是的，winixj尽量避免使用gcc中嵌入汇编
 * 因为实在是很晦涩难懂，但是有时候为了方便
 * 还是会多少有一点，不过原则没有变，尽量少用
 *
 * Author : myjfm(mwxjmmyjfm@gmail.com)
 * v0.01 2011/12/05
 ***************************************************************/


#ifndef __WINIXJ_SYSTEM_H__
#define __WINIXJ_SYSTEM_H__

//加载ldt
#define lldt(selector) \
	__asm__ __volatile__ ("lldt %%ax\n\t" \
			: \
			: "a" (selector))

//加载tss
#define ltr(selector) \
	__asm__ __volatile__ ("ltr %%ax\n\t" \
			: \
			: "a" (selector))

#define push(op) \
	__asm__ __volatile__ ("push %%eax\n\t" \
			: \
			: "a" (op))

//该宏用于加载init进程之前对各个段寄存器的初始化
//因为我们是手工启动第一个进程init，因此各个段寄存器
//都需要手工来初始化
#define init_segr() \
	__asm__ __volatile__ ("movw $0x17, %%ax\n\t" \
			"movw %%ax, %%ds\n\t" \
			"movw %%ax, %%es\n\t" \
			"movw %%ax, %%gs\n\t" \
			"movw %%ax, %%fs" \
			::)

//将iret()宏修改为iretd()，因为iretd更明确的指名了从
//堆栈中弹出的是双字值，虽然gas中没有iretd指令
#define iretd() __asm__ __volatile__ ("iret\n" ::)
#define sti() __asm__ __volatile__ ("sti\n" ::)
#define cli() __asm__ __volatile__ ("cli\n" ::)
#define halt() __asm__ __volatile__ ("cli\n\t" \
		"1:\n" \
		"jmp 1b\n" \
		::)
#define nop() __asm__ ("nop\n" ::)

#endif

