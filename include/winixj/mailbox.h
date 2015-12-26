#ifndef __WINIXJ_MAILBOX_H__
#define __WINIXJ_MAILBOX_H__

#include <type.h>

#define MAX_MAIL_LEN 16

//邮件的总体格式
//srcpid为发送邮件的进程pid
//srcmid为发送邮件的进程欲接收邮件的邮箱号
//dstpid为接收邮件的进程pid
//dstmid为接收邮件的进程的邮箱号
typedef struct mail
{
	uint16 srcpid;
	mail_t srcmid;
	uint16 dstpid;
	mail_t dstmid;
	uint8 mail_len; //邮件的长度
	uint8 mail[MAX_MAIL_LEN]; //邮件缓存
} MAIL;

#define MAX_MQUEUE_LEN 10

//信箱的格式
//信箱是一个循环队列
//队列最大大小为MAX_MQUEUE_LEN
//head指向队列中的头
//tail指向队列中的尾
//isunlocked表明邮箱是否被使用
//1表明邮箱有效
//0表明进程没有使用该邮箱
typedef struct mailbox
{
	uint8 isunlocked;
	MAIL mail_queue[MAX_MQUEUE_LEN];
	uint8 head;
	uint8 tail;
} MAILBOX;

#endif

