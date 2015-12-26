/***************************************************************
 * 本文件是硬盘驱动核心文件
 * 当前对缓冲区实现的管理还比较naive
 * 整个buf_info只能被一个进程使用，直到一个进程使用完才能解锁缓冲区
 * 而且对于硬盘的读操作和写操作目前分离实现，仅仅是对缓冲区申请与释放
 * 的标志VALID和INVALID选取的不好，不过这又有什么关系呢，这个小的不雅
 * 观以后会很容易修正，到时候可以将hd_read和hd_write合并实现，而且也
 * 不需要使用两个不同的get_buffer，一个是read_get_buffer一个是write_
 * get_buffer，目前先将就用
 * 而且目前的硬盘驱动还没有和块设备驱动统一起来，在编写文件系统的时候
 * 免不了要对其进行修改
 *
 * Author : myjfm(mwxjmmyjfm@gmail.com)
 * v0.01 2012/01/07
 ***************************************************************/


#include <type.h>
#include <string.h>
#include <asm/system.h>
#include <winixj/int.h>
#include <winixj/hdd.h>
#include <winixj/buffer.h>
#include <winixj/dev_drv.h>

static uint8 *pp = (uint8 *)(0xb8008);

//记录硬盘数量
int NR_HD = 0;
//硬盘以及硬盘上的分区表信息
struct hd_info hd_info[MAX_HD];

static int hd_result();
static void hd_ready();
static int read_wait_hd_int();
static int write_wait_hd_int();
static void get_partition();
static void hd_oprt(HD_CMD *);
static int set_cmd(HD_CMD *, int, int, size_t, int);
static int hd_identify(int);

void init_hd()
{
	*pp = '0';
	install_int_handler(AT_WIN_IV, (void *)int_at_win);//安装硬盘中断对应的处理程序
	enable_hwint(IRQ2_IV);//打开主8259A的IRQ2
	enable_hwint(AT_WIN_IV);//打开从8259A的IRQ14以使可以响应硬盘中断
	//对硬盘初始化
	//获取在boot中通过bios获取的硬盘参数信息
	get_partition();
}

static void get_partition()
{
	register int drive	= 0;
	int i				= 0;
	PARTITION *p		= NULL;
	uint16 *data		= NULL;

	for (; drive < MAX_HD; ++drive)
	{
		//向硬盘发送identify命令
		if (hd_identify(drive) != 0)
		{
			//循环测试HD_STATUS_PORT_1端口的值
			//直到位7为0，位3为1，且位0为0
			//否则说明硬盘不是ATA类型、或者有错误产生
			while ((in_byte(HD_STATUS_PORT_1) & 
						(BUSY_STAT | DRQ_STAT | ERR_STAT)) 
					!= DRQ_STAT) {}
			NR_HD++;
			read_get_buffer();//获取缓冲区，用以存放IDENTIFY指令结果
			read_port(HD_DATA_PORT_1, buf_info.data, SECTOR_SIZE);

			data = (uint16 *)buf_info.data;

			//word49的bit9置位说明支持lba28模式
			if (data[49] & 0x200)
			{
				hd_info[drive].support_lba28	= LBA28_SUPPORTED;
				hd_info[drive].nr_sects_lba28	= ((uint32)data[61] << 16) + data[60];
			}
			else
			{
				hd_info[drive].support_lba28	= LBA28_UNSUPPORTED;
				//如果系统不支持lba28位模式，则系统挂起
				//因为我们所有的硬盘寻址都是以lba28位模式寻址的
				halt();
			}

			//word83的bit10置位说明支持lba28模式
			if (data[83] & 0x400)
			{
				hd_info[drive].support_lba48	= LBA48_SUPPORTED;
				hd_info[drive].nr_sects_lba48	= ((uint64)data[103] << 48) + 
					((uint64)data[102] << 32) + ((uint64)data[101] << 16) + data[100];
			}
			else
			{
				hd_info[drive].support_lba48	= LBA48_UNSUPPORTED;
			}

			release_buffer();
			data = hd_read(drive, 0, SECTOR_SIZE);

			//判断是否是硬盘的MBR
			if (((uint8 *)data)[510] != 0x55 || 
					((uint8 *)data)[511] != 0xaa)
			{
				halt();
			}

			hd_info[drive].partitions[0].start_lba	= 0;
			hd_info[drive].partitions[0].nr_sects	= hd_info[drive].nr_sects_lba28;
			PARTITION *p = (PARTITION *)((void *)data + 0x1be);

			for (i = 1; i < MAX_PARTS; ++i, ++p)
			{
				hd_info[drive].partitions[i] = *p;

				if (p->nr_sects > 0)
				{
					hd_info[drive].nr_parts++;
				}
			}

			release_buffer();
		}
	}
}

//读取第drive号磁盘，起始lba地址为lba，读取count字节
void *hd_read(int drive, int lba, size_t count)
{
	void *data = NULL;
	int res;
	HD_CMD c;

	//如果缓冲区此时正在被某个进程使用，则进程忙等待
	//实际上这种情况不可能发生，因为WinixJ的进程对高速
	//缓冲区的访问是串行的，也就是一个进程如果没有获得
	//该缓冲区，则会一直忙等待，这样其他进程就得不到调度
	data = read_get_buffer();

	if (!data)
	{
		//如果无法获取缓冲区则说明系统出问题，挂起
		halt();
	}

	if (set_cmd(&c, drive, lba, count, HD_READ) == -1)
	{
		return NULL;
	}

read_again:
	hd_oprt(&c);
	res = read_wait_hd_int();

	//如果读硬盘出错，则重新读
	if (res == -1)
	{
		buf_info.valid = INVALID;
		goto read_again;
	}
	
	while (count > 0)
	{
		read_port(HD_DATA_PORT_1, data, SECTOR_SIZE);
		data += SECTOR_SIZE;
		count -= SECTOR_SIZE;
	}

	return buf_info.data;
}

//写入第drive号磁盘，起始lba地址为lba，字节数为count字节
void *hd_write(int drive, int lba, size_t count)
{
	void *data = NULL;
	int res;
	HD_CMD c;

	data = write_get_buffer();

	if (!data)
	{
		//如果无法获取缓冲区则说明内核出错，挂起
		halt();
	}

	if (set_cmd(&c, drive, lba, count, HD_WRITE) == -1)
	{
		return NULL;
	}

write_again:
	hd_oprt(&c);
	res = write_wait_hd_int();

	//如果读硬盘出错，则重新读
	if (res == -1)
	{
		buf_info.valid = VALID;
		goto write_again;
	}
	
	while (count >= SECTOR_SIZE)
	{
		write_port(HD_DATA_PORT_1, data, SECTOR_SIZE);
		data += SECTOR_SIZE;
		count -= SECTOR_SIZE;
	}

	if (count > 0)
	{
		write_port(HD_DATA_PORT_1, data, count);
	}

	return buf_info.data;
}

//构造硬盘操作命令
//c为HD_CMD结构体
//drive为驱动器号
//lba为起始的lba地址
//count为欲读/写的字节数
//opr为欲进行的操作：读/写
static int set_cmd(HD_CMD *c, int drive, int lba, size_t count, int opr)
{
	if (!c || 
		(drive != 0 && drive != 1) || 
		lba < 0 || lba >= (1 << 28) || 
		count <= 0 || count > BUF_LEN || 
		(opr != HD_READ && opr != HD_WRITE))
	{
		return -1;
	}

	memset(c, 0, sizeof(HD_CMD));
	c->features	= 0;
	c->count	= (count + SECTOR_SIZE - 1) / SECTOR_SIZE;
	c->lba_low	= lba & 0xff;
	c->lba_mid	= (lba & 0xff00) >> 8;
	c->lba_high	= (lba & 0xff0000) >> 16;
	c->device	= SET_DEVICE_REG(drive, (lba & 0xf000000) >> 24);
	c->command	= opr;
	return 0;
}

//通过向指定端口写入相应参数用来对硬盘进行读/写操作
static void hd_oprt(HD_CMD *c)
{
	hd_ready();

	//向端口0x3f6写0以打开硬盘中断
	out_byte(HD_DEV_CTRL_PORT_1, 0);
	out_byte(HD_FEATURES_PORT_1, c->features);
	//需要读/写的扇区数
	out_byte(HD_NSECTOR_PORT_1, c->count);
	out_byte(HD_LBA_LOW_PORT_1, c->lba_low);
	out_byte(HD_LBA_MID_PORT_1, c->lba_mid);
	out_byte(HD_LBA_HIGH_PORT_1, c->lba_high);
	out_byte(HD_DEVICE_PORT_1, c->device);
	//需要的操作，读HD_READ/写HD_WRITE
	out_byte(HD_CMD_PORT_1, c->command);
}

static int read_wait_hd_int()
{
	//只要缓冲区数据没有准备好就死等下去
	while (buf_info.occ == current && 
			buf_info.locked == LOCKED && 
			buf_info.valid == INVALID) {}
	
	if (buf_info.occ != current || 
			buf_info.locked == UNLOCKED)
	{
		halt();
	}

	if (buf_info.valid == ERROR)
	{
		return -1;
	}

	return 0;
}

static int write_wait_hd_int()
{
	//只要硬盘没有准备好被写入数据，就死等
	while (buf_info.occ == current && 
			buf_info.locked == LOCKED && 
			buf_info.valid == VALID) {}
	
	if (buf_info.occ != current || 
			buf_info.locked == UNLOCKED)
	{
		halt();
	}

	if (buf_info.valid == ERROR)
	{
		return -1;
	}

	return 0;
}

//实质上的硬盘中断处理程序
//用来将buf_info的valid位置1，通过该位来通知进程
//硬盘操作已经完成
void validate_buffer()
{
	int res = hd_result();

	if (res == 1)
	{
		buf_info.valid = ERROR;
	}
	else if (res == 0)
	{
		if (buf_info.valid	== VALID)
		{
			buf_info.valid = INVALID;
		}
		else
		{
			buf_info.valid = VALID;
		}
	}
}

//等待硬盘就绪，如果硬盘无法就绪则系统会在此死机
static void hd_ready()
{
	uint8 res = in_byte(HD_STATUS_PORT_1);

	//照文档来说判断硬盘控制器是否准备好应该如此判断
	//但是读取出来的状态值却是0
	//暂且忽略，仅判断BSY位
#if 0
	//status端口值的BSY位为0
	//说明硬盘已经准备好
	while ((res & (BUSY_STAT | READY_STAT)) != READY_STAT)
	{
		res = in_byte(HD_STATUS_PORT_1);
	}
#endif

	//status端口值的BSY位为0
	//说明硬盘已经准备好
	while ((res & BUSY_STAT) == BUSY_STAT)
	{
		res = in_byte(HD_STATUS_PORT_1);
	}
}

//读取硬盘中断完之后的结果
//如果正常就返回0
//否则返回1
static int hd_result()
{
	uint8 i = in_byte(HD_STATUS_PORT_1);

	if ((i & (BUSY_STAT | READY_STAT | WRERR_STAT | 
					SEEK_STAT | ERR_STAT)) 
			== (READY_STAT | SEEK_STAT))
	{
		return 0;
	}

	if (i & 1)
	{
		i = in_byte(HD_ERROR_PORT_1);
	}

	return 1;
}

static int hd_identify(int drive)
{
	uint8 device = 0;

	if (drive == 0)
	{
		device = 0xa0;
	}
	else if (drive == 1)
	{
		device = 0xb0;
	}
	else
	{
		return -1;
	}

	out_byte(HD_NSECTOR_PORT_1, 0);
	out_byte(HD_LBA_LOW_PORT_1, 0);
	out_byte(HD_LBA_MID_PORT_1, 0);
	out_byte(HD_LBA_HIGH_PORT_1, 0);
	out_byte(HD_DEVICE_PORT_1, device);
	out_byte(HD_CMD_PORT_1, HD_IDENTIFY);
	return in_byte(HD_STATUS_PORT_1);
}

