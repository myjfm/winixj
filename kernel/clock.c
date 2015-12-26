#include <type.h>
#include <time.h>
#include <winixj/int.h>
#include <winixj/clock.h>

//记录自系统启动以来的时钟中断次数
//注意！！！！！ 单纯用该变量和从CMOS中读取出来的
//开始时间来计算任意时刻的时间是有较大误差的，因为
//每当系统陷入内核态的时候会有关中断的情况，这时候
//boot_heartbeat不会自增，因此会出现误差，该误差暂
//不解决，等待未来版本，具体解决办法可以参考
//《操作系统设计与实现》中的minix源码或者《ULK3》
//以及linux源码
uint32 volatile boot_heartbeat = 0;
//32位整型，因此最多可以表示到(2^32) / (365 * 24 * 60 * 60) + 1970 = 
uint32 cur_time;

//从CMOS中读取系统启动时的时间，注意CMOS中存储的时间
//是自1900年来的年份，而UNIX普遍的标准是需要用一个全局
//变量存储自1970年1月1日凌晨0点0分0秒开始的秒数，因此
//根据从CMOS中读取的时间得到cur_time这个全局值
void get_boot_time()
{
//从cmos内存指定偏移地址处取出当前时间
//将地址与0x80或是为了关闭NMI不可屏蔽中断
//最后一个out_byte()函数是为了将NMI不可屏蔽中断打开
#define read_cmos(addr) \
({ \
	register uint8 res; \
	out_byte(0x70, addr | 0x80); \
	res = in_byte(0x71); \
	out_byte(0x70, 0x0); \
	res; \
})

#define bcd_to_bin(n) \
	((((n) >> 4) & 0xf) * 10 + ((n) & 0xf))

	int month[12] =
	{
		0, 
		31, 
		31 + 29, 
		31 + 29 + 31, 
		31 + 29 + 31 + 30, 
		31 + 29 + 31 + 30 + 31, 
		31 + 29 + 31 + 30 + 31 + 30, 
		31 + 29 + 31 + 30 + 31 + 30 + 31, 
		31 + 29 + 31 + 30 + 31 + 30 + 31 + 31, 
		31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30, 
		31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31, 
		31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30, 
	};

	struct tm cmos_time;

	//下面是从linux中拿来的
	//这个循环的目的我猜是为了不将秒数设置成一秒中比较靠后的地方
	//否则这样时间误差比较大
	do {
		cmos_time.tm_sec	= (int)bcd_to_bin(read_cmos(0x00));
		cmos_time.tm_min	= (int)bcd_to_bin(read_cmos(0x02));
		cmos_time.tm_hour	= (int)bcd_to_bin(read_cmos(0x04));
		cmos_time.tm_mday	= (int)bcd_to_bin(read_cmos(0x07));
		cmos_time.tm_mon	= (int)bcd_to_bin(read_cmos(0x08)) - 1;
		cmos_time.tm_year	= (int)bcd_to_bin(read_cmos(0x09));
	} while (cmos_time.tm_sec != (int)bcd_to_bin(read_cmos(0x00)));

#define MINUTE	60
#define HOUR	(60 * (MINUTE))
#define DAY		(24 * (HOUR))

	int year = cmos_time.tm_year - 70;
	int leapyear = ((year - 2) >> 2) + 1;
	cur_time = (year * 365 + leapyear) * DAY;

	//今年是闰年
	if ((year - 2) % 4 == 0)
	{
		cur_time -= DAY;
	}

	cur_time += month[cmos_time.tm_mon];

	if (cmos_time.tm_mon > 1 && (year - 2) % 4 != 0)
	{
		cur_time -= DAY;
	}

	cur_time += DAY * (cmos_time.tm_mday - 1);
	cur_time += HOUR * cmos_time.tm_hour;
	cur_time += MINUTE * cmos_time.tm_min;
	cur_time += cmos_time.tm_sec;
}

//设置时钟中断发生的频率
//默认HZ为100，在编译内核前可自行调整
static void set_clock_freq()
{
	out_byte(TIMER_MODE, SQUARE_WAVE);
	out_byte(COUNTER0, (uint8)(TIMER_FREQ / HZ));
	out_byte(COUNTER0, (uint8)((TIMER_FREQ / HZ) >> 8));
}

void init_clock()
{
	boot_heartbeat = 0;
	get_boot_time();
	set_clock_freq();
	install_int_handler(CLOCK_IV, (void *)int_clock);
	enable_hwint(CLOCK_IV);	//打开8259A主片的irq0，以允许时钟中断
}

