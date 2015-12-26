#include <type.h>
#include <winixj/process.h>
#include <winixj/schedule.h>

//进程间的切换
//intel X86进程间切换支持三种方式：
//1、通过段间jmp或者段间call指令实现任务切换
//		这种方式要求jmp或者call的目标地址指向GDT中的一个TSS
//		描述符，或者是一个任务门描述符。（而其实这个任务门描述
//		符无非还是指向一个TSS描述符)
//2、产生了中断（可以是任何形式的中断），而中断向量指向了中断
//		向量表中的一个任务门描述符
//3、使用iret指令，并且在执行这个指令的时候EFLAGS寄存器中的NT
//		位被置1
//
//
//
//WinixJ选择采用第一种方式，主要原因是它比较简单
//不过在未来版本中考虑将进程调度、键盘驱动、鼠标驱动等均做成
//独立任务，这样，中断向量表中将由中断门和任务门组成，这样做的
//好处是保持内核的简洁，能够独立出去的模块尽量独立，不过坏处
//也很明显：任务切换是需要开销的，因为一种假想实现如下：所有的
//时钟中断、键盘中断等任务门均指向同一个处理句柄，比如命名为
//task_handler(uint8 int_vector)，此函数中会根据中断向量号来
//分别调用不同的处理函数，但是这样会存在很多switch、case语句
//汇编出来会有很多类似的je等跳转指令，对处理器的流水线有一定
//影响，不过目前不知道影响有多大，可以参考一些书籍以做进一步研究
void switch_next(uint32 selector)
{
	uint64 dest_addr = ((uint64)selector) << 32;
	__asm__ __volatile__ ("ljmp %0\n"::"m" (dest_addr));
}

//对于每个时钟中断，如果当前被中断的进程的时间片还没有用完
//那么就不需要调用schedule()函数去重新调度
void pre_schedule()
{
	current->running_time++; //进程运行的时间片+1

	//如果当前进程的时间片还没有用完，则不需要进程切换
	if ((--(current->time_slices)) > 0)
	{
		return;
	}
	else
	{
		schedule();
	}
}

//调度函数，基于时间片轮转的调度算法
//如果所有的进程时间片都用完，则采用
//如下公式：
//p->time_slices = p->time_slices / 2 + p->priority
void schedule()
{
	int i;
	int time_slices	= 0;
	PROCESS * next	= proc_list;
	uint8 *p		= (uint8 *)0xb8004;

	while (1)
	{
		*p			= (*p) + 1;
		time_slices	= 0;
		next		= proc_list;

		for (i = 0; i < NR_PROCS; ++i)
		{
			if (proc_list[i].state == PROC_RUNNING && proc_list[i].time_slices > time_slices)
			{
				time_slices	= proc_list[i].time_slices;
				next		= proc_list + i;
			}
		}

		if (time_slices > 0)
		{
			break;
		}

		for (i = 0; i < NR_PROCS; ++i)
		{
			proc_list[i].time_slices = (proc_list[i].time_slices >> 1) + proc_list[i].priority;
		}
	}

	if (next == current)
	{
		return;
	}

	current = next;
	switch_next(TSS_SELECTOR(current->pid));
}

