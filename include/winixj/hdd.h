/***************************************************************
 * 本文件是硬盘驱动主要用到的头文件
 * 目前版本的硬盘还不支持分区，只能将整个系统安装到
 * 整个一块硬盘上，对于分区的支持以后会得以实现
 *
 * Author : myjfm(mwxjmmyjfm@gmail.com)
 * v0.01 2011/12/20
 ***************************************************************/


#ifndef __WINIXJ_HDD_H__
#define __WINIXJ_HDD_H__

#include <type.h>

#define MAX_PARTS 5	//每块硬盘最多4个分区
#define MAX_HD 2	//最大支持的硬盘数量

//实际的硬盘数量
extern int NR_HD;

typedef struct partition
{
	uint8 bootable;//分区是否可启动的标志
	uint8 start_head;//起始磁头号
	uint8 start_sector;//起始扇区号(仅用了低6位，高2位为起始柱面号的8、9位)
	uint8 start_cyl_low8;//起始柱面号的低8位
	uint8 sys_id;//分区类型
	uint8 end_head;//结束磁头号
	uint8 end_sector;//结束扇区号(仅用了低6位，高2位为结束柱面号的第8、9位)
	uint8 end_cyl_low8;//结束柱面号低8位
	uint32 start_lba;//起始扇区的lba
	uint32 nr_sects;//扇区数目 
} PARTITION;

struct hd_info
{
	uint8 support_lba28;//硬盘是否支持lba28模式
#define LBA28_SUPPORTED 1
#define LBA28_UNSUPPORTED 0
	uint8 support_lba48;//硬盘是否支持lba48模式
#define LBA48_SUPPORTED 1
#define LBA48_UNSUPPORTED 0
	uint32 nr_sects_lba28;//lba28模式下硬盘最大扇区数量
	uint64 nr_sects_lba48;//lba48模式下硬盘最大扇区数量
	PARTITION partitions[MAX_PARTS];//硬盘分区信息
	uint8 nr_parts;//该硬盘的分区数量
};

#define HD_DATA_PORT_1			0x1f0
#define HD_FEATURES_PORT_1		0x1f1
#define HD_ERROR_PORT_1			0x1f1
#define HD_NSECTOR_PORT_1		0x1f2
#define HD_LBA_LOW_PORT_1		0x1f3
#define HD_LBA_MID_PORT_1		0x1f4
#define HD_LBA_HIGH_PORT_1		0x1f5
#define HD_DEVICE_PORT_1		0x1f6
#define HD_CMD_PORT_1			0x1f7
#define HD_STATUS_PORT_1		0x1f7
#define HD_ALT_STATUS_PORT_1	0x3f6
#define HD_DEV_CTRL_PORT_1		0x3f6

//用来操作IDE2通道
//WinixJ目前不支持
#define HD_DATA_PORT_2			0x170
#define HD_FEATURES_PORT_2		0x171
#define HD_ERROR_PORT_2			0x171
#define HD_NSECTOR_PORT_2		0x172
#define HD_LBA_LOW_PORT_2		0x173
#define HD_LBA_MID_PORT_2		0x174
#define HD_LBA_HIGH_PORT_2		0x175
#define HD_DEVICE_PORT_2		0x176
#define HD_CMD_PORT_2			0x177
#define HD_STATUS_PORT_2		0x177
#define HD_ALT_STATUS_PORT_2	0x376
#define HD_DEV_CTRL_PORT_2		0x376

#define SECTOR_SIZE				512

//硬盘状态寄存器各位的定义
#define ERR_STAT				0x01//命令执行错误
#define INDEX_STAT				0x02//收到索引
#define ECC_STAT				0x04//ECC校验错误
#define DRQ_STAT				0x08//请求服务
#define SEEK_STAT				0x10//寻道错误
#define WRERR_STAT				0x20//驱动器故障
#define READY_STAT				0x40//驱动器准备好（就绪）
#define BUSY_STAT				0x80//控制器忙碌

//硬盘操作需要的命令
typedef struct hd_oprt_cmd
{
	uint8 features;
	uint8 count;//要读/写的扇区数量
	uint8 lba_low;//lba地址的低8位
	uint8 lba_mid;//lba地址的中间8位
	uint8 lba_high;//lba地址的高8位
	//device寄存器比较复杂
	//它的八位如下：
	//|  0  |  1  |  2  |  3  |  4  |  5  |  6  | 7
	//| HS0 | HS1 | HS2 | HS3 | DRV |  1  |  L  | 1
	//其中L位如果为0则表明对磁盘操作采用CHS模式，即柱面/磁头/扇区号
	//如果L位为1则对磁盘操作采用LBA模式
	//DRV位为0则代表主磁盘，1则代码从磁盘
	//如果L = 0，则0～3位代表磁头号
	//如果L = 1，则0～3位代表LBA地址的高4位
	//在WinixJ中，对硬盘的操作全部采用LBA模式
	//所以L位为1
	//另外LBA地址为28位，因此最多能表示2^28B=128G硬盘容量
	uint8 device;
#define SET_DEVICE_REG(drive, lba_highest) \
	(0xe0 | (((drive) & 0x1) << 4) | \
	((lba_highest) & 0xf))
	uint8 command;
#define HD_READ 0x20
#define HD_WRITE 0x30
#define HD_IDENTIFY 0xec
} HD_CMD;

//记录硬盘信息
extern struct hd_info hd_info[MAX_HD];

//对硬盘进行初始化，包括安装硬盘中断处理函数以及打开硬盘中断
extern void init_hd();
extern void int_at_win();//硬盘中断处理程序
extern void validate_buffer();
//读取第drive号磁盘，起始lba地址为lba，扇区数量是count
extern void *hd_read(int drive, int lba, size_t count);
//向第drive号磁盘写，起始lba地址为lba，扇区数量是count
extern void *hd_write(int drive, int lba, size_t count);

#endif

