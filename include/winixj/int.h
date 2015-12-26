/***************************************************************
 * 本文件主要定义中断向量号等一些常量
 * 注意这些常量的值需要和include/asm/protect.sh中定义的
 * 常量值保持一致
 *
 * Author : myjfm(mwxjmmyjfm@gmail.com)
 * v0.01 2011/12/06
 ***************************************************************/


#ifndef __WINIXJ_INT_H__
#define __WINIXJ_INT_H__

#define DIVIDE_IV		0x0 //除零异常的中断向量号
#define DEBUG_IV		0x1 //调试异常的中断向量号
#define NMI_IV			0x2 //非屏蔽外部中断向量号
#define BREAKPOINT_IV	0x3 //调试断点中断向量号
#define OVERFLOW_IV		0x4 //溢出中断向量号
#define BOUNDS_IV		0x5 //越界中断向量号
#define INVAL_OP_IV		0x6 //无效操作码中断向量号
#define COPROC_NOT_IV	0x7 //设备不可用/无数学协处理器中断向量号
#define DOUBLE_FAULT_IV	0x8 //双重错误中断向量号
#define COPROC_SEG_IV	0x9 //协处理器段越界中断向量号
#define INVAL_TSS_IV	0xa //无效TSS中断向量号
#define SEG_NOT_IV		0xb //段不存在中断向量号
#define STACK_FAULT_IV	0xc //堆栈段错误中断向量号
#define PROTECTION_IV	0xd //常规保护错误中断向量号
#define PAGE_FAULT_IV	0xe //页错误中断向量号
////////////////////////////////这里的0xf向量号保留，未使用
#define COPROC_ERR_IV	0x10 //x87FPU浮点错中断向量号

#define IRQ0_IV			0x20 //IRQ0时钟对应的中断向量号
#define IRQ1_IV			0x21 //IRQ1键盘对应的中断向量号
#define IRQ2_IV			0x22 //连接从8259A片
#define IRQ3_IV			0x23 //IRQ3串口2对应的中断向量号
#define IRQ4_IV			0x24 //IRQ4串口1对应的中断向量号
#define IRQ5_IV			0x25 //IRQ5LPT2对应的中断向量号
#define IRQ6_IV			0x26 //IRQ6软盘对应的中断向量号
#define IRQ7_IV			0x27 //IRQ7LPT1对应的中断向量号
#define IRQ8_IV			0x28 //IRQ8实时时钟对应的中断向量号
#define IRQ9_IV			0x29 //IRQ9重定向IRQ2
#define IRQ10_IV		0x2a //IRQ10保留
#define IRQ11_IV		0x2b //IRQ11保留
#define IRQ12_IV		0x2c //IRQ12ps/2鼠标对应的中断向量号
#define IRQ13_IV		0x2d //IRQ13FPU异常对应的中断向量号
#define IRQ14_IV		0x2e //IRQ14AT温彻斯特盘对应的中断向量号
#define IRQ15_IV		0x2f //IRQ15保留

#define SYS_CALL_IV		0x30 //系统调用中断向量号

#define CLOCK_IV		IRQ0_IV
#define KEYBOARD_IV		IRQ1_IV
#define SPORT2_IV		IRQ3_IV
#define SPORT1_IV		IRQ4_IV
#define LPT2_IV			IRQ5_IV
#define FLOPPY_IV		IRQ6_IV
#define LPT1_IV			IRQ7_IV
#define RTC_IV			IRQ8_IV
#define REDRCT_IRQ2_IV	IRQ9_IV
#define RES1_IV			IRQ10_IV
#define RES2_IV			IRQ11_IV
#define PS2_MOUSE_IV	IRQ12_IV
#define FPU_FAULT_IV	IRQ13_IV
#define AT_WIN_IV		IRQ14_IV
#define RES3_IV			IRQ15_IV

/*
 * 一下是初始化8259A主片和从片的时候用到的操作端口
 * 初始化8259A的过程如下：
 * 往端口20h（主片）或A0h（从片）写入ICW1
 * 往端口21h（主片）或A1h（从片）写入ICW2
 * 往端口21h（主片）或A1h（从片）写入ICW3
 * 往端口21h（主片）或A1h（从片）写入ICW4
 */
#define MASTER_CTL_8259			0x20
#define SLAVE_CTL_8259			0xa0
#define MASTER_CTL_MASK_8259	0x21
#define SLAVE_CTL_MASK_8259		0xa1

#include <type.h>

//对端口进行读写操作
extern void out_byte(uint16 port, uint8 value);
extern uint8 in_byte(uint16 port);
extern void read_port(uint16 port, void* buf, int n);
extern void write_port(uint16 port, void* buf, int n);

//注册相应的中断处理程序or卸载对应的中断处理程序
extern int install_int_handler(uint8 INT_IV, void *handler);
extern int uninstall_int_handler(uint8 INT_IV);
extern int install_sys_call_handler(uint8 INT_IV, void* handler);

//打开or关闭对应的中断
extern void enable_hwint(uint8 IV);
extern void disable_hwint(uint8 IV);

#endif

