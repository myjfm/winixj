#ifndef __TIME_H__
#define __TIME_H__

//下面的结构是UNIX世界的标准结构
//tm_sec:秒，取值区间[0, 59]
//tm_min:分，取值区间[0, 59]
//tm_hour:时，取值区间[0, 23]
//tm_mday:一个月中的日期，取值区间为[1, 31]
//tm_mon:月份，取值区间[0, 11]
//tm_year:年，其值从1900年开始
//tm_wday:星期，取值区间[0, 6]，0代表周日
//tm_yday:从每年的1月1日开始的天数，取值区间[0, 365]，0代表1月1日，依次类推
//tm_isdst:夏时令标志，tm_isdst > 0则为夏时令；tm_isdst = 0则不实行夏时令；不了解情况时为负
struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

#endif

