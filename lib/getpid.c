#include <type.h>

int getpid()
{
	int res;
	__asm__ __volatile__ (
			"movl $0x0, %%eax\n\t"
			"movl $0x0, %%ebx\n\t"
			"movl $0x0, %%ecx\n\t"
			"movl $0x0, %%edx\n\t"
			"int $0x30\n"
			:"=a" (res)
			:);
	return res;
}

