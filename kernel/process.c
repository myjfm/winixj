#include <string.h>
#include <asm/system.h>
#include <winixj/mm.h>
#include <winixj/int.h>
#include <winixj/mailbox.h>
#include <winixj/process.h>
#include <winixj/schedule.h>

//所有进程所拥有的内核态堆栈都放到一起
//进程的内核态堆栈的作用主要是用来保存
//进程被打断时的现场信息
KSTACK kstack_list[NR_PROCS];

/*********************************************
 * 这只是权宜之计，因为我们还没有实现内存管理
 * 所以暂时使每个进程都有一个独立的用户态堆栈
 *********************************************/
//所有进程所拥有的用户太堆栈也都放到一起
USTACK ustack_list[NR_PROCS];

//进程链表，保存所有的进程控制块信息
//这是全局变量
PROCESS proc_list[NR_PROCS];

PROCESS *current; //指向当前正在运行的进程

extern void init();
extern void sys();

static void set_tss_seg(int n, void *addr)
{
	gdt[n].seg_limit_low16		= 0x0068; //tss段长为104个字节，不能多也不能少
	gdt[n].seg_base_low16		= (uint16)(((uint32)addr) & 0xffff); //段地址的低16位
	gdt[n].seg_base_mid8		= (uint8)((((uint32)addr) >> 16) & 0xff); //段地址的中间8位
	gdt[n].attr1				= 0x89; //该段在内存中存在，DPL=0，是TSS描述符
	gdt[n].attr2_limit_high4	= 0x40; //段界限粒度是字节
	gdt[n].seg_base_high8		= (uint8)((((uint32)addr) >> 24) & 0xff); //段地址的高8位
}

static void set_ldt_seg(int n, void *addr)
{
	gdt[n].seg_limit_low16		= 0x0018; //所有进程的ldt均只包含三个描述符，因此ldt段长为24个字节
	gdt[n].seg_base_low16		= (uint16)(((uint32)addr) & 0xffff); //段地址的低16位
	gdt[n].seg_base_mid8		= (uint8)((((uint32)addr) >> 16) & 0xff); //段地址的中间8位
	gdt[n].attr1				= 0x82; //该段在内存中存在，DPL=0，是LDT描述符
	gdt[n].attr2_limit_high4	= 0x40; //段界限粒度是字节
	gdt[n].seg_base_high8		= (uint8)((((uint32)addr) >> 24) & 0xff); //段地址的高8位
}

//初始化init和sys进程
//主要包括进程pid、进程名、ldt、tss以及init进程在gdt中对应的项的初始化
static void init_proc01()
{
	struct seg_struct ldt_temp[3] = {{0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00}, 
									 {0xffff, 0x0000, 0x00, 0xfa, 0xcf, 0x00}, 
									 {0xffff, 0x0000, 0x00, 0xf2, 0xcf, 0x00}};

	//pid为0
	//进程名为"init"
	proc_list[0].pid				= 0;
	proc_list[0].ppid				= -1;
	proc_list[0].state				= PROC_RUNNING;
	proc_list[0].priority			= 15;
	proc_list[0].time_slices		= proc_list[0].priority;
	proc_list[0].running_time		= 0;
	strcpy(proc_list[0].name, "init");
	//ldt共包括三项，0->空，1->cs，2->ds&ss
	proc_list[0].ldt[0]				= ldt_temp[0];
	proc_list[0].ldt[1]				= ldt_temp[1];
	proc_list[0].ldt[2]				= ldt_temp[2];
	proc_list[0].tss.back_link		= 0;
	proc_list[0].tss.esp0			= KSTACKTOP(0); //内核态堆栈顶
	proc_list[0].tss.ss0			= KERNEL_SS_SELECTOR;
	proc_list[0].tss.esp1			= 0;
	proc_list[0].tss.ss1			= 0;
	proc_list[0].tss.esp2			= 0;
	proc_list[0].tss.ss2			= 0;
	proc_list[0].tss.cr3			= (uint32)page_dir; //页目录表地址
	proc_list[0].tss.eip			= 0;
	proc_list[0].tss.eflags			= 0;
	proc_list[0].tss.eax			= 0;
	proc_list[0].tss.ecx			= 0;
	proc_list[0].tss.edx			= 0;
	proc_list[0].tss.ebx			= 0;
	proc_list[0].tss.esp			= 0;
	proc_list[0].tss.ebp			= 0;
	proc_list[0].tss.esi			= 0;
	proc_list[0].tss.edi			= 0;
	proc_list[0].tss.es				= 0x17; //指向ldt中的选择子
	proc_list[0].tss.cs				= 0x0f; //指向ldt中的选择子
	proc_list[0].tss.ss				= 0x17; //指向ldt中的选择子
	proc_list[0].tss.ds				= 0x17; //指向ldt中的选择子
	proc_list[0].tss.fs				= 0x17; //指向ldt中的选择子
	proc_list[0].tss.gs				= 0x17; //指向ldt中的选择子
	proc_list[0].tss.ldt			= LDT_SELECTOR(0);
	proc_list[0].tss.trace_bitmap	= 0x80000000;

	set_tss_seg(FIRST_TSS_INDEX, &(proc_list[0].tss));
	set_ldt_seg(FIRST_LDT_INDEX, proc_list[0].ldt);

	//pid为1
	//进程名为"sys"
	proc_list[1].pid				= 1;
	proc_list[1].ppid				= 0;
	proc_list[1].state				= PROC_RUNNING;
	proc_list[1].priority			= 15;
	proc_list[1].time_slices		= proc_list[1].priority;
	proc_list[1].running_time		= 0;
	strcpy(proc_list[1].name, "sys");
	//ldt共包括三项，0->空，1->cs，2->ds&ss
	proc_list[1].ldt[0]				= ldt_temp[0];
	proc_list[1].ldt[1]				= ldt_temp[1];
	proc_list[1].ldt[2]				= ldt_temp[2];
	proc_list[1].tss.back_link		= 0;
	proc_list[1].tss.esp0			= KSTACKTOP(1); //内核态堆栈顶
	proc_list[1].tss.ss0			= KERNEL_SS_SELECTOR;
	proc_list[1].tss.esp1			= 0;
	proc_list[1].tss.ss1			= 0;
	proc_list[1].tss.esp2			= 0;
	proc_list[1].tss.ss2			= 0;
	proc_list[1].tss.cr3			= (uint32)page_dir; //页目录表地址
	proc_list[1].tss.eip			= (uint32)sys;
	proc_list[1].tss.eflags			= 0x1202;
	proc_list[1].tss.eax			= 0;
	proc_list[1].tss.ecx			= 0;
	proc_list[1].tss.edx			= 0;
	proc_list[1].tss.ebx			= 0;
	proc_list[1].tss.esp			= USTACKTOP(1);
	proc_list[1].tss.ebp			= USTACKTOP(1);
	proc_list[1].tss.esi			= 0;
	proc_list[1].tss.edi			= 0;
	proc_list[1].tss.es				= 0x17; //指向ldt中的选择子
	proc_list[1].tss.cs				= 0x0f; //指向ldt中的选择子
	proc_list[1].tss.ss				= 0x17; //指向ldt中的选择子
	proc_list[1].tss.ds				= 0x17; //指向ldt中的选择子
	proc_list[1].tss.fs				= 0x17; //指向ldt中的选择子
	proc_list[1].tss.gs				= 0x17; //指向ldt中的选择子
	proc_list[1].tss.ldt			= LDT_SELECTOR(1);
	proc_list[1].tss.trace_bitmap	= 0x80000000;

	set_tss_seg(FIRST_TSS_INDEX + 2, &(proc_list[1].tss));
	set_ldt_seg(FIRST_LDT_INDEX + 2, proc_list[1].ldt);
}

void init_proc_list()
{
	int i;
	for (i = 0; i < NR_PROCS; ++i)
	{
		proc_list[i].pid			= -1;
		proc_list[i].ppid			= -1;
		proc_list[i].state			= PROC_STOPPED;
		proc_list[i].priority		= 0;
		proc_list[i].time_slices	= 0;
		proc_list[i].running_time	= 0;
		memset(proc_list[i].name, 0, PROC_NAME_LEN);
		memset(proc_list[i].ldt, 0, 3 * sizeof(struct seg_struct));
		memset(&(proc_list[i].tss), 0, sizeof(TSS));
		set_tss_seg(FIRST_TSS_INDEX + i * 2, &(proc_list[i].tss));
		set_ldt_seg(FIRST_LDT_INDEX + i * 2, proc_list[i].ldt);
	}

	//启动的第一个进程是进程链表proc_list中第一个进程，为init进程
	current	= proc_list;
}

void start_proc0()
{
	//初始化init进程
	init_proc01();

	uint32 ss		= 0x17;
	uint32 esp		= USTACKTOP(0);
	uint32 eflags	= 0x1202; //init进程可以使用I/O命令，同时要求init进程允许中断
	uint32 cs		= 0x0f;
	uint32 eip		= (uint32)init;

	/**********************************************************
	 * 下面调试了很久！！！！！！！！！！！！！！！！！
	 * 不能将enable_hwint(CLOCK_IV);语句放到紧挨sti()的前面
	 * 因为enable_hwint(CLOCK_IV)是c语句宏，展开之后会
	 * 用到堆栈，就会把刚push进去的ss、esp、eflags、cs、eip
	 * 给覆盖掉！！！！！！！！！！！！！！！！！！！！！！
	 * 将enable_hwint()放到init_clock()里面
	 **********************************************************/

	//加载init进程对应的tss
	//加载init进程对应的ldt
	ltr(TSS_SELECTOR(0));
	lldt(LDT_SELECTOR(0));
	push(ss);				//push ss
	push(esp);				//push esp
	push(eflags);			//push eflags
	push(cs);				//push cs
	push(eip);				//push eip
	init_segr();			//初始化ds、es、gs、fs四个段寄存器, 使其指向ldt中的数据段描述符
	sti();					//打开中断，从现在开始内核态将允许中断
	iretd();				//iretd这条指令之后第一个进程init进程便开始运行了
}

