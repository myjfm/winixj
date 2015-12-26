#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define BUF_LEN 520
#define BOOT_BUF_LEN BUF_LEN
#define FILE_NAME_LEN 50

unsigned char boot_buf[BOOT_BUF_LEN]; //缓存boot文件内容
unsigned char buffer[BOOT_BUF_LEN];

char boot[FILE_NAME_LEN]; //存储boot文件名
char loader[FILE_NAME_LEN]; //存储loader文件名
char kernel[FILE_NAME_LEN]; //存储kernel文件名
char image[FILE_NAME_LEN]; //存储输出文件的文件名

FILE *bootp = NULL; //boot文件的文件操作句柄
FILE *loaderp = NULL; //loader文件的文件操作句柄
FILE *kernelp = NULL; //kernel文件的文件操作句柄
FILE *imagep = NULL; //image文件的文件操作句柄

static void usage()
{
	fprintf(stderr, "Usage: build [-b ../boot/boot] ");
	fprintf(stderr, "[-l ../boot/loader] [-k ../system.map] [-w ../System.Image]\n");
}

static void init()
{
	//指定默认的boot、loader、kernel和输出文件的文件名
	strcpy(boot, "../boot/boot"); //默认情况下boot文件在顶层目录的boot子目录中
	strcpy(loader, "../boot/loader"); //默认情况下loader文件在顶层目录的loader子目录中
	strcpy(kernel, "../system.map"); //默认情况下kernel文件在顶层目录的kernel子目录中
	strcpy(image, "../System.Image"); //默认在顶层目录生成系统映像
}

static void proc_opt(int argc, char * const *argv)
{
	int ch;
	opterr = 0; //不显示错误信息

	while ((ch = getopt(argc, argv, "b:l:k:w:h")) != -1)
	{
		switch (ch)
		{
			case 'b': //指定boot文件名
				strcpy(boot, optarg);
				break;
			case 'l': //指定loader文件名
				strcpy(loader, optarg);
				break;
			case 'k': //指定kernel文件名
				strcpy(kernel, optarg);
				break;
			case 'w': //指定输出的系统映像文件名
				strcpy(image, optarg);
				break;
			case 'h':
				usage();
				exit(1);
		}
	}
}

static void open_file()
{
	//如果指定的boot文件不存在，则退出
	if (0 != access(boot, F_OK))
	{
		fprintf(stderr, "\"%s\": No such file.\n", boot);
		exit(1);
	}
	
	//如果指定的loader文件不存在，则退出
	if (0 != access(loader, F_OK))
	{
		fprintf(stderr, "\"%s\": No such file.\n", loader);
		exit(1);
	}
	
	//如果指定的kernel文件不存在，则退出
	if (0 != access(kernel, F_OK))
	{
		fprintf(stderr, "\"%s\": No such file.\n", kernel);
		exit(1);
	}

	//如果指定的image文件存在，则给出warning
	if (0 == access(image, F_OK))
	{
		fprintf(stderr, "Warning: The file \"%s\" exists.\n", image);
		fprintf(stderr, "But we will go on ...\n");
	}

	bootp = fopen(boot, "r+");
	//如果不能打开boot文件
	if (NULL == bootp)
	{
		fprintf(stderr, "cannot open the file \"%s\".\n", boot);
		exit(1);
	}

	loaderp = fopen(loader, "r+");
	//如果不能打开loader文件
	if (NULL == loaderp)
	{
		fprintf(stderr, "cannot open the file \"%s\".\n", loader);
		exit(1);
	}

	kernelp = fopen(kernel, "r+");
	//如果不能打开kernel文件
	if (NULL == kernelp)
	{
		fprintf(stderr, "cannot open the file \"%s\".\n", kernel);
		exit(1);
	}

	imagep = fopen(image, "w+");
	//如果不能创建image文件
	if (NULL == imagep)
	{
		fprintf(stderr, "cannot create the file \"%s\".\n", image);
		exit(1);
	}
}

int main(int argc, char * const *argv)
{
	int n;
	struct stat loader_stat, kernel_stat;
	int loader_len = 0, kernel_len = 0;

	init(); //初始化

	proc_opt(argc, argv); //处理命令行参数

	open_file(); //打开boot、loader、kernel和image文件

	//将boot文件的512字节读入boot_buf缓冲区
	n = fread(boot_buf, 512, 1, bootp);

	if (1 != n)
	{
		fprintf(stderr, "cannot read 512 bytes from %s.\n", boot);
		exit(1);
	}

	//检查boot文件的末尾两个字节，如果为0xaa55，则认为是合法的boot文件
	if (0xaa55 != *(unsigned short *)(boot_buf + 510))
	{
		fprintf(stderr, "%s is not bootable file.\n", boot);
		exit(1);
	}

	//获取loader文件的信息
	n = stat(loader, &loader_stat);
	
	if (-1 == n)
	{
		fprintf(stderr, "cannot get %s's status.\n", loader);
		exit(1);
	}

	//获取loader文件的长度，按字节计算
	loader_len = loader_stat.st_size;

	//获取kernel文件的信息
	n = stat(kernel, &kernel_stat);

	if (-1 == n)
	{
		fprintf(stderr, "cannot get %s's status.\n", kernel);
		exit(1);
	}

	//获取kernel文件的长度，按字节计算
	kernel_len = kernel_stat.st_size;

	//修改boot中LOADER_LEN和KERNEL_LEN字段，详细请查看boot.asm源码最后10行
	boot_buf[507] = (0 == (loader_len & 0x1ff)) ? (loader_len >> 9) : (loader_len >> 9) + 1;
	*(unsigned short *)(boot_buf + 508) = (0 == (kernel_len & 0x1ff)) ? (kernel_len >> 9) : (kernel_len >> 9) + 1;

	//将boot文件内容写入image文件中
	n = fwrite(boot_buf, 512, 1, imagep);

	if (1 != n)
	{
		fprintf(stderr, "cannot write into %s.\n", image);
		exit(1);
	}

	//将loader文件写入image中，按扇区数来写入，最后一扇区不够的用0补足
	while (loader_len > 0)
	{
		memset(buffer, 0, sizeof(buffer));
		n = fread(buffer, loader_len > 512 ? 512 : loader_len, 1, loaderp);

		if (1 != n)
		{
			fprintf(stderr, "cannot read %d bytes from %s.\n", 
					loader_len > 512 ? 512 : loader_len, boot);
			exit(1);
		}

		n = fwrite(buffer, 512, 1, imagep);
		
		if (1 != n)
		{
			fprintf(stderr, "cannot write into %s.\n", image);
			exit(1);
		}

		loader_len -= 512;
	}

	//将kernel文件写入image中，按扇区数来写入，最后一扇区不够的用0补足
	while (kernel_len > 0)
	{
		memset(buffer, 0, sizeof(buffer));
		n = fread(buffer, kernel_len > 512 ? 512 : kernel_len, 1, kernelp);

		if (1 != n)
		{
			fprintf(stderr, "cannot read %d bytes from %s.\n", 
					kernel_len > 512 ? 512 : kernel_len, boot);
			exit(1);
		}

		n = fwrite(buffer, 512, 1, imagep);
		
		if (1 != n)
		{
			fprintf(stderr, "cannot write into %s.\n", image);
			exit(1);
		}

		kernel_len -= 512;
	}

	fclose(bootp);
	fclose(loaderp);
	fclose(kernelp);
	fclose(imagep);

	return 0;
}

