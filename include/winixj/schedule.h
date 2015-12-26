/***************************************************************
 * 本文件声明用于进程调度的一些函数及变量
 * 注意进程调度函数十分重要，以至于必须仔细设计
 * 当前linux系统大概被重新设计最频繁的部分也就是进程
 * 调度部分了吧，所以我们先设计一个最简单的轮转调度
 * 更好的调度算法留待以后改进。
 *
 * Author : myjfm(mwxjmmyjfm@gmail.com)
 * v0.01 2011/12/09
 ***************************************************************/


#ifndef __WINIXJ_SCHEDULE_H__
#define __WINIXJ_SCHEDULE_H__

#include <type.h>

#define PROC_RUNNING			1
#define PROC_INTERRUPTIBLE		2
#define PROC_UNINTERRUPTIBLE	3
#define PROC_ZOMBIE				4
#define PROC_STOPPED			5

//这似乎是整个系统最核心的函数了
//它是整个winixj系统的大法官，负责
//决定哪个进程该运行哪个进程停止运行
extern void schedule();
extern void pre_schedule();

#endif

