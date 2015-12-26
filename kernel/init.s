PARAM_ADDR	equ 0xf0000 ; 在laoder中保存的bios参数的地址，此地址是相当于段基地址0x0的偏移量
PDE_ADDR	equ 0x100000 ; 页目录表从1M地址开始, 大小4K，含有1024个表项
PTE_ADDR	equ 0x101000 ; 页表紧接着页目录表开始

extern set_idt
extern cbegin

global _start ; kernel程序入口，需要导出
global gdt
global gdtr
global stacktop ; 内核态的堆栈顶

[SECTION .text]
_start:
	lgdt [gdtr] ; 重新加载新的全局描述符表
	jmp 0x08:new_gdt ; 使新的全局描述符表立即生效
new_gdt:
	;设置各个段寄存器
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov gs, ax
	mov fs, ax
	mov ss, ax
	mov esp, stacktop ; 重新设置堆栈

	call set_idt ; 设置中断

	call setup_paging ; 开启分页机制

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; 从下面这一指令之后便真正跳入c函数内执行
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	call cbegin ; 跳转到cbegin函数执行，cbegin函数负责进程方面的初始化工作



setup_paging:
	xor eax, eax
	xor ebx, ebx
	xor ecx, ecx
	xor edx, edx
	mov esi, PARAM_ADDR
	add esi, 0x2
	mov word ax, [esi] ; 扩展内存的大小（即1MB以后的内存的大小），以KB为单位，因此扩展内存最大64MB
	cmp ax, 0x08 ; 扩展内存大小不能小于8KB，因为我们保证至少要能存放页目录表和一个页表
	jb extended_mem_shortage
	cmp ax, 0xfbff ; 如果扩展内存大于63MB内存则报错
	ja too_many_mem
	add ax, 0x400 ; 扩展内存大小加上1MB等于总内存数量
	mov ebx, 0x1000
	div ebx ; 总内存数量(XKB)除以每个页表代表的内存数量(4KKB)，即用X/4K即得总共应该分的页数
	mov ecx, eax ; 商在eax中，需要分的页数
	test edx, edx
	jz no_remainder
	inc ecx
no_remainder:
	push ecx

	; 开始初始化页目录表
	mov edi, PDE_ADDR
	mov eax, PTE_ADDR + 0x007 ; 页表首地址0x100000，属性是在内存中存在且普通用户可读可写
write_pde:
	stosd
	add eax, 0x1000
	loop write_pde

	; 开始初始化页表
	pop eax ; 页表个数
	; 页表总数乘以每个页表含有的表项数目(1024个)则得到总共需要的表项数目
	mov ecx, 10
goon_shl:
	shl eax, 1 ; 页表总数乘以每个页表含有的表项数目(1024个)则得到总共需要的表项数目
	jc too_many_mem
	loop goon_shl
	mov ecx, eax
	mov edi, PTE_ADDR
	mov eax, 0x007 ; 从物理地址0x0开始，属性是在内存中存在且普通用户可读可写
write_pte:
	stosd
	add eax, 0x1000 ; 每页内存大小为4KB
	loop write_pte

	mov eax, PDE_ADDR
	mov cr3, eax ; cr3高20位存放pde的地址的高20位
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax ; 打开cr0的最高一位PG位
	ret

extended_mem_shortage:
	hlt

too_many_mem:
	hlt

[SECTION .data]
gdt:
		; 第一个描述符空，不使用
		db 0x00,	0x00
		db 0x00,	0x00,	0x00
		db 0x00,	0x00
		db 0x00

		; 第二个描述符的段基地址为0；
		; 界限为0xfffff，段界限粒度为4KB；
		; 是可执行可读的32位代码段
		db 0xff,	0xff
		db 0x00,	0x00,	0x00
		db 0x9a,	0xcf
		db 0x00

		; 第三个描述符的段基地址为0；
		; 界限为0xfffff，段界限粒度为4KB；
		; 是可读可写32位数据段
		db 0xff,	0xff
		db 0x00,	0x00,	0x00
		db 0x92,	0xcf
		db 0x00
		
		; gdt表共可存放256个段描述符
		times 253 * 8 db 0

gdtr:				dw $ - gdt - 1
					dd gdt

[SECTION .bss]
stackbase resb	4 * 1024 ;4KB堆栈大小
stacktop: ;堆栈顶部，向下扩展
