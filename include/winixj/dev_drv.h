#ifndef __WINIXJ_DEV_DRV_H__
#define __WINIXJ_DEV_DRV_H__

extern void *sys_param;

//目前只支持两种类型的设备
//字符设备tty
//块设备hard disk
#define NR_DEV	2

typedef struct dev_table
{
	//struct action *act;
} DEVICE ;

extern DEVICE device_table[NR_DEV];

#endif

