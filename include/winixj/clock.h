/***************************************************************
 * 本文件声明的函数用于时钟中断
 *
 * Author : myjfm(mwxjmmyjfm@gmail.com)
 * v0.01 2011/12/08
 ***************************************************************/


#ifndef __WINIXJ_CLOCK_H__
#define __WINIXJ_CLOCK_H__

#include <type.h>

#define TIMER_MODE	0x43 //8253模式控制寄存器端口值
#define COUNTER0	0x40
#define SQUARE_WAVE	0x36 //8253工作方式为方波，先写低字节再写高字节
#define TIMER_FREQ	1193182L
#define HZ			100 //设置1秒产生100次时钟中断

//时钟计数，每个时钟周期自增1, 具体参考clock.c文件说明
extern uint32 volatile boot_heartbeat;
extern void init_clock();
//时钟中断处理程序
extern void int_clock();

#endif

