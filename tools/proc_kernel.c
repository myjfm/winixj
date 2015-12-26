#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

typedef	unsigned int	Elf32_Addr;
typedef	unsigned short	Elf32_Half;
typedef unsigned int	Elf32_Off;
typedef unsigned int	Elf32_Sword;
typedef unsigned int	Elf32_Word;

#define EI_NIDENT	16
#define MAX_BUF_LEN	1024

//ELF文件的ELF header结构
typedef struct
{
	unsigned char	e_ident[EI_NIDENT]; //ELF魔数
	Elf32_Half		e_type; //文件类型
	Elf32_Half		e_machine; //支持的机器架构
	Elf32_Word		e_version; //版本号
	Elf32_Addr		e_entry; //程序的入口地址，在编译内核的时候由ld程序手工指定
	Elf32_Off		e_phoff; //program header在文件中的偏移量
	Elf32_Off		e_shoff; //section header在文件中的偏移量
	Elf32_Word		e_flags; //标志
	Elf32_Half		e_ehsize; //ELF头的大小
	Elf32_Half		e_phentsize; //每个program header table的大小
	Elf32_Half		e_phnum; //program header的数量
	Elf32_Half		e_shentsize; //每个section header table的大小
	Elf32_Half		e_shnum; //section header的数量
	Elf32_Half		e_shstrndx;
}Elf32_Ehdr;
#define ELFHDR_LEN sizeof(Elf32_Ehdr)

//ELF文件的program header结构
typedef struct
{
	Elf32_Word		p_type; //该头所指向的program segment的类型
	Elf32_Off		p_offset; //该头所指向的program segment在文件中的偏移
	Elf32_Addr		p_vaddr; //该头所指向的program segment加载进内存后的虚拟地址
	Elf32_Addr		p_paddr; //该头所指向的program segment加载进内存后的物理地址
	Elf32_Word		p_filesz; //该头所指向的program segment在文件中的大小
	Elf32_Word		p_memsz; //该头所指向的program segment在内存中的大小
	Elf32_Word		p_flags;
	Elf32_Word		p_align;
}Elf32_Phdr;
#define PHDR_LEN sizeof(Elf32_Phdr)

//输出到kernel.map文件中的时候，对每一个program segment，
//都有一个Seghdr开头，表征该段的信息，包括段的大小，以及段
//的起始虚拟地址
typedef struct
{
	int memsz;
	int vaddr;
}Seghdr;
#define SEGHDR_LEN sizeof(Seghdr)

#define FILE_NAME_LEN 50
//缓冲区
unsigned char buffer[MAX_BUF_LEN];
char infilename[FILE_NAME_LEN];
char outfilename[FILE_NAME_LEN];
FILE *ifp, *ofp;

static void usage()
{
	fprintf(stderr, "Usage: proc_kernel [-r ../system] [-w ../System.Image]\n");
}

void die()
{
	fclose(ifp);
	fclose(ofp);
	exit(1);
}

static void init()
{
	strcpy(infilename, "../system"); //默认输入文件为在顶层目录的内核文件
	strcpy(outfilename, "../system.map"); //默认输出到顶层目录的kernel子目录中，文件名为system.map
}

static void proc_opt(int argc, char * const *argv)
{
	int ch;
	opterr = 0; //不显示错误信息

	while ((ch = getopt(argc, argv, "r:w:h")) != -1)
	{
		switch (ch)
		{
			case 'r': //指定kernel文件名
				strcpy(infilename, optarg);
				break;
			case 'w': //指定输出的系统映像文件名
				strcpy(outfilename, optarg);
				break;
			case 'h':
				usage();
				exit(1);
		}
	}
}

static void open_file()
{
	//如果输入的内核文件不存在
	if (0 != access(infilename, F_OK))
	{
		fprintf(stderr, "\"%s\": No such file.\n", infilename);
		exit(1);
	}

	//如果输出的内核映像文件已经存在，报warning
	if (0 == access(outfilename, F_OK))
	{
		fprintf(stderr, "Warning: The file \"%s\" exists.\n", outfilename);
		fprintf(stderr, "But we will go on ...\n");
	}

	ifp = fopen(infilename, "r+");
	//如果不能打开输入文件
	if (NULL == infilename)
	{
		fprintf(stderr, "cannot open the file \"%s\".\n", infilename);
		exit(1);
	}

	ofp = fopen(outfilename, "w+");
	//如果不能创建kernel.map文件
	if (NULL == ofp)
	{
		fprintf(stderr, "cannot create the file \"%s\".\n", outfilename);
		exit(1);
	}
}

int main(int argc, char *const *argv)
{
	int pht_offset, n, i;
	Elf32_Ehdr elf_header;//保存elf文件头
	Elf32_Phdr p_header_buf;
	Seghdr seg_header_buf;
	unsigned short loadable_seg_num = 0;

	init();
	proc_opt(argc, argv);
	open_file();

	//读出ELF文件头
	n = fread((void *)&elf_header, ELFHDR_LEN, 1, ifp);

	if (n < 1)
	{
		fprintf(stderr, "cannot read the \"%s\" file's ELF header!\n", infilename);
		die();
	}

	//输出文件的文件头格式为
	//0  1  2  3  4  5  6  7  8  9  10  11(以字节为单位)
	//k  e  r  n  e  l  |入口地址|  |段数|
	n = fwrite("kernel", 6, 1, ofp);

	if (n < 1)
	{
		fprintf(stderr, "cannot write into \"%s\".\n", outfilename);
		die();
	}

	//entry address
	n = fwrite((void *)(&(elf_header.e_entry)), 4, 1, ofp);

	if (n < 1)
	{
		fprintf(stderr, "cannot write into \"%s\".\n", outfilename);
		die();
	}

	fprintf(stdout, "entry address: %x\n", elf_header.e_entry);
	//the number of segments
	n = fwrite("  ", 2, 1, ofp);

	if (n < 1)
	{
		fprintf(stderr, "cannot write into \"%s\".\n", outfilename);
		die();
	}

	//判断输入文件是否是ELF格式文件
	//ELF格式的文件头部的前四字节是“.ELF”的ascii码
	if (((int *)(elf_header.e_ident))[0]!= 0x464c457f)
	{
		fprintf(stderr, "\"%s\" is not an ELF file!\n", infilename);
		die();
	}

	//判断文件是否是可执行文件
	if (elf_header.e_type != 2)
	{
		fprintf(stderr, "\"%s\" is not an excutable file!\n", infilename);
		die();
	}

	//该ELF支持的机器架构是否是80386类型
	if (elf_header.e_machine != 3)
	{
		fprintf(stderr, "\"%s\" is not for I386!\n", infilename);
		die();
	}

	//输出内核文件包含的程序段信息
	fprintf(stdout, "\"%s\"含有的程序段的段数: %d\n", infilename, elf_header.e_phnum);

	for (i = 0; i < elf_header.e_phnum; ++i)
	{
		if (PHDR_LEN != elf_header.e_phentsize)
		{
			fprintf(stderr, "program header entry is confused!\n");
			die();
		}

		fseek(ifp, elf_header.e_phoff + i * elf_header.e_phentsize, SEEK_SET);
		n = fread(&p_header_buf, elf_header.e_phentsize, 1, ifp);

		if (n < 1)
		{
			fprintf(stderr, "cannot read the program header entry!\n");
			die();
		}

		fprintf(stdout, "第%d个段的段头内容:\n", i + 1);
		fprintf(stdout, "\tp_type:   0x%x\n", p_header_buf.p_type);
		fprintf(stdout, "\tp_offset: 0x%x\n", p_header_buf.p_offset);
		fprintf(stdout, "\tp_vaddr:  0x%x\n", p_header_buf.p_vaddr);
		fprintf(stdout, "\tp_paddr:  0x%x\n", p_header_buf.p_paddr);
		fprintf(stdout, "\tp_filesz: 0x%x\n", p_header_buf.p_filesz);
		fprintf(stdout, "\tp_memsz:  0x%x\n", p_header_buf.p_memsz);
		fprintf(stdout, "\tp_flags:  0x%x\n", p_header_buf.p_flags);
		fprintf(stdout, "\tp_align:  0x%x\n", p_header_buf.p_align);

		if (1 != p_header_buf.p_type)//is not PT_LOAD
		{
			fprintf(stderr, "this segment is not loadable......\n");
			continue;
		}

		loadable_seg_num++;
		//对每个程序段都在头部加上描述该程序段的信息头
		//包含程序段的长度以及程序段加载到内存时的虚拟地址
		seg_header_buf.memsz = p_header_buf.p_filesz;
		seg_header_buf.vaddr = p_header_buf.p_vaddr;

		n = fwrite((void *)&seg_header_buf, SEGHDR_LEN, 1, ofp);
		if (1 != n)
		{
			fprintf(stderr, "cannot write the segment length into \"%s\".\n", outfilename);
			die();
		}

		//将段内容写进输出文件中
		fseek(ifp, p_header_buf.p_offset, SEEK_SET);

		while (p_header_buf.p_filesz > MAX_BUF_LEN)
		{
			n = fread(buffer, 1, MAX_BUF_LEN, ifp);

			if (MAX_BUF_LEN != n)
			{
				fprintf(stderr, "cannot read the segment from \"%s\".\n", infilename);
				die();
			}

			p_header_buf.p_filesz -= MAX_BUF_LEN;
			n = fwrite(buffer, MAX_BUF_LEN, 1, ofp);

			if (1 != n)
			{
				fprintf(stderr, "cannot write the segment into \"%s\".\n", outfilename);
				die();
			}
		}

		if (p_header_buf.p_filesz > 0)
		{
			n = fread(buffer, p_header_buf.p_filesz, 1, ifp);

			if (1 != n)
			{
				fprintf(stderr, "cannot read the segment from \"%s\".\n", infilename);
				die();
			}

			n = fwrite(buffer, p_header_buf.p_filesz, 1, ofp);

			if (1 != n)
			{
				fprintf(stderr, "cannot write the segment into \"%s\".\n", outfilename);
				die();
			}
		}
	}

	//将输入文件中可加载段的段数写进输出文件头中
	fseek(ofp, 10, SEEK_SET);
	n = fwrite((void *)&loadable_seg_num, 2, 1, ofp);

	if (1 != n)
	{
		fprintf(stderr, "cannot write the entry address into the \"kernel.map\" file!\n");
		die();
	}

	fclose(ifp);
	fclose(ofp);
	return 0;
}
