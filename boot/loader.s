;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 该文件主要作用是获取一些系统参数，将参数保存在0xf000:0x0的位置，然后准备gdt，
; 并切换到保护模式，不过该gdt只是临时性的，在进入kernel之后还会重新设置gdt；
; 在保护模式下将kernel从原来位置加载到指定位置，这里我们假定指定位置为0x0开始的地方。
; 移动kernel文件的方法因为文件的类型不同而不同，在linux3.0.0.12下，gcc版本4.5编译
; 为ELF格式的二进制文件，将program segment按照编译时指定的虚拟地址加载到相应位置。
; kernel的入口地址为KERNEL_ADDR，注意该值必须和在链接内核时指定的入口地址一致，
; 否则加载内核失败；成功加载内核后便跳转到内核执行。
; 
; Author : myjfm(mwxjmmyjfm@gmail.com)
; v0.01 2011/11/22
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
jmp start

PARAM_ADDR	equ 0xf000 ; 系统参数保存在的位置的段基址
KERNEL_ADDR	equ	0x0 ; kernel入口地址，注意，此处的值一定要和编译kernel文件时指定的入口地址相同

[SECTION .code16]
[BITS 16]
start:
	mov ax, cs ; 当前cs值为：0x8000
	mov ds, ax
	mov es, ax
	sub ax, 0x20
	mov ss, ax ; ss地址为0x7fe0
	mov sp, 0x200 ; 堆栈大小为512B = 0.5KB

	; 获取当前光标位置
	mov ah, 0x03
	xor bh, bh
	int 0x10

	; 打印一些信息
	mov ax, cs
	mov es, ax
	mov cx, MSG1_LEN
	mov bx, 0x0007
	mov bp, MSG1 ; 显示字符串"Loading ..."
	mov ax, 0x1301
	int 0x10

	push es ; 先保存es寄存器的值
	mov ax, 0x0
	mov es, ax
	mov byte al, [es:0x7dfb] ; 取出保存在boot文件尾部的loader文件大小值
	mov byte [LOADER_LEN], al
	mov word ax, [es:0x7dfc]
	mov word [KERNEL_LEN], ax ; 取出保存在boot文件尾部的kernel文件大小值
	pop es

	; 取得kernel的入口地址以及第一个program segment在内存中的物理地址
	mov bx, es
	xor ax, ax
	mov byte al, [LOADER_LEN]
	shl ax, 0x05
	add bx, ax
	mov es, bx ; es指向kernel文件头部
	mov si, MSG3
	mov edi, 0x0
	mov cx, 0x06
	cld
	repe cmpsb
	cmp cx, 0x0 ; 如果kernel文件头部的魔数确实为“kernel”，则认为是合法的kernel文件
	jne no_kernel
	mov dword eax, [es:di]
	cmp eax, KERNEL_ADDR
	jne no_kernel
	mov dword [KERNEL_ENTRY], eax ; 取得kernel的入口地址
	add di, 0x04
	mov word ax, [es:di]
	mov word [PROG_SEG_NUM], ax ; 取得kernel文件中program segment的数量
	add di, 0x02
	xor eax, eax
	mov ax, es
	shl eax, 4
	and edi, 0x0000ffff
	add eax, edi
	mov dword [PROG_SEG_FIRST], eax ; 保存kernel中第一个program segment在内存中的物理地址

	; 下面的操作是从linux0.11中“拿”来的
	; 取得当前光标位置
	push ds
	push es

	mov ax, PARAM_ADDR
	mov ds, ax
	mov ah, 0x03
	xor bh, bh
	int 0x10

	; 获得扩展内存大小，即1MB以后的内存大小，以KB计
	mov word [0], dx
	mov ah, 0x88
	int 0x15
	mov word [2], ax

	; 显示卡当前显示模式
	mov ah, 0x0f
	int 0x10
	mov word [4], bx
	mov word [6], ax

	; 检查显示方式（EGA/VGA）并取参数
	mov ah, 0x12
	mov bl, 0x10
	int 0x10
	mov word [8], ax
	mov word [10], bx
	mov word [12], cx

	; 取第一块硬盘hd0的信息（复制硬盘参数表）。
	; 第一个硬盘参数表的首地址竟然是中断向量0x41的向量值！
	; 而第二块硬盘参数表紧接着第一个表的后面。
	; 中断向量0x46的向量值也指向这第二块硬盘的参数表首地址。
	; 表的长度为16字节（0x10）。
	mov ax, 0x0000
	mov ds, ax
	lds si, [4 * 0x41]
	mov ax, PARAM_ADDR
	mov es, ax
	mov di, 0x0080
	mov cx, 0x10
	rep movsb

	; 取第二块硬盘hd1的信息
	mov ax, 0x0000
	mov ds, ax
	lds si, [4 * 0x46]
	mov ax, PARAM_ADDR
	mov es, ax
	mov di, 0x0090
	mov cx, 0x10
	rep movsb

	; 检查系统是否存在第二块硬盘，不存在则将第二个表清零。
	mov ax, 0x1500
	mov dl, 0x81
	int 0x13
	jc no_disk1
	cmp ah, 0x03
	je is_disk1
no_disk1:
	mov ax, PARAM_ADDR
	mov es, ax
	mov di, 0x0090
	mov cx, 0x10
	mov ax, 0x00
	rep movsb

is_disk1:
	pop es
	pop ds
	jmp setup_pm

no_kernel:
	; 获取当前光标位置
	mov ah, 0x03
	xor bh, bh
	int 0x10

	; 打印一些信息
	mov ax, cs
	mov es, ax
	mov cx, MSG2_LEN
	mov bx, 0x0007
	mov bp, MSG2 ; 显示字符串"[[ NO KERNEL ]]\r\nPress any key to reboot now ..."
	mov ax, 0x1301
	int 0x10

	xor ax, ax
	int 0x16 ; 等待用户输入任意一个键盘字符
	int 0x19 ; 系统重启

setup_pm:
	; 将保护模式代码段基地址填入GDT1描述符中
	xor eax, eax
	mov ax, cs
	shl eax, 4
	add eax, PMCODE
	mov word [CS_TMP + 2], ax
	shr eax, 16
	mov byte [CS_TMP + 4], al
	mov byte [CS_TMP + 7], ah

	; 将保护模式数据段基地址填入GDT1描述符中
	xor eax, eax
	mov ax, ds
	shl eax, 4
	add eax, PMDATA
	mov word [DS_TMP + 2], ax
	shr eax, 16
	mov byte [DS_TMP + 4], al
	mov byte [DS_TMP + 7], ah

	; 填充GDTPTR结构体
	; 该结构体将被加载进gdtr寄存器
	xor eax, eax
	mov ax, ds
	shl eax, 4
	add eax, GDT
	mov dword [GDTPTR + 2], eax

	lgdt [GDTPTR]

	; 关闭中断
	cli

	; 通过设置键盘控制器的端口值来打开A20地址线
	call empty_8042
	mov al, 0xd1
	out 0x64, al
	call empty_8042
	mov al, 0xdf
	out 0x60, al
	call empty_8042

	; 修改cr0寄存器的最后一位PE位
	; 注意lmsw指令仅仅对cr0寄存器的最后四位有影响
	mov eax, cr0
	or eax, 0x01
	lmsw ax

	; 真正跳转到保护模式！！！！！！
	jmp dword 0x08:0x00

empty_8042:
	nop
	nop
	in al, 0x64
	test al, 0x02
	jnz empty_8042
	ret

[section .pmcode]
align 32
[bits 32]
PMCODE:
	mov ax, 0x10
	mov ds, ax
	xor ecx, ecx
	mov word cx, [pm_PROG_SEG_NUM] ; 取得kernel文件中program segment的段数
	mov dword esi, [pm_PROG_SEG_FIRST] ; 取得kernel文件中的第一个program segment的地址
	mov ax, 0x20
	mov ds, ax
	mov es, ax
move_kernel:
	push ecx
	mov ecx, [esi] ; 取得program segment在文件中的大小
	add esi, 4
	mov edi, [esi] ; 取得段在内存中的地址
	add esi, 4
	cld
	rep movsb
	pop ecx
	loop move_kernel

	mov ax, 0x20
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov fs, ax
	mov gs, ax
	jmp 0x18:KERNEL_ADDR

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; GDT描述符中由低到高的含义如下：
;  BYTE7  || BYTE6 BYTE5 || BYTE4 BYTE3 BYTE2 || BYTE1 BYTE0 ||
; 段基地址||	  属性   ||       段基址      ||    段界限   ||
; (31-24) ||             ||      (23-0)       ||    (15-0)   ||
;
; 属性含义如下：
; 7 || 6 || 5 || 4 || 3   2   1   0 || 7 || 6 || 5 || 4   3   2  1   0 ||
; G ||D/B|| 0 ||AVL|| 段界限(19-16) || P ||DPL|| S ||       TYPE       ||
;
; G位：段界限的粒度，0为字节，1为4KB
; D/B；在可执行代码段中，这一位叫做D位：
;		D=1时指令使用32位地址及32位或8位操作数；D=0时使用16位地址及16位或8位操作数
;      在数据段描述符中，这一位叫做B位：
;		B=1时段上部界限为4GB；B=0时段上部界限为64KB
;      在堆栈段描述符中，这一位叫做B位：
;		B=1时隐式的堆栈访问指令（比如push、pop、call等）使用32位堆栈指针寄存器esp
;		B=0时隐式的堆栈访问指令使用16位堆栈指针寄存器sp
; AVL: 保留
; P位: 1代表段在内存中存在；0代表段在内存中不存在
; DPL：描述符特权级
; S位：1代表是数据段或者代码段描述符；0代表是系统段或者门描述符
;TYPE：说明描述符类型
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[section .pmdata]
align 32
[bits 32]
PMDATA:
		; 第一个描述符空，不使用
GDT:	db 0x00,	0x00
		db 0x00,	0x00,	0x00
		db 0x00,	0x00
		db 0x00

		; 第二个描述符为保护模式代码所在的段
		; 段基地址在切换到保护模式前填入；
		; 界限为0xfffff，段界限粒度为4KB，因此段长为4GB；
		; 是可执行可读的32位代码段
CS_TMP:	db 0xff,	0xff
		db 0x00,	0x00,	0x00
		db 0x9a,	0xcf
		db 0x00

		; 第三个描述符为切换到保护模式用到的数据段
		; 段基地址为PMDATA，在切换到保护模式前填入；
		; 界限为0xfffff，段界限粒度为4KB，因此段长为4GB；
		; 是可读可写32位数据段
DS_TMP:	db 0xff,	0xff
		db 0x00,	0x00,	0x00
		db 0x92,	0xcf
		db 0x00

		; 第四个描述符的段基地址为0；
		; 界限为0xfffff，段界限粒度为4KB；
		; 是可执行可读的32位代码段
CODE:	db 0xff,	0xff
		db 0x00,	0x00,	0x00
		db 0x9a,	0xcf
		db 0x00

		; 第五个描述符的段基地址为0；
		; 界限为0xfffff，段界限粒度为4KB；
		; 是可读可写32位数据段
DATA:	db 0xff,	0xff
		db 0x00,	0x00,	0x00
		db 0x92,	0xcf
		db 0x00

GDT_LEN				equ $ - GDT
GDTPTR				dw GDT_LEN - 1
					dd 0

; 在实模式下直接使用LOADER_LEN等标号就能取出内存中的内容
; 在保护模式下想要取LOADER_LEN等内存中的内容需要使用基于段基地址的偏移
pm_LOADER_LEN		equ $ - PMDATA
LOADER_LEN:			db 0 ; 记录loader文件的大小

pm_KERNEL_LEN		equ $ - PMDATA
KERNEL_LEN:			dw 0 ; 记录kernel文件的大小，注意，这里指的是从System.Image解压出来前的大小

pm_KERNEL_ENTRY		equ $ - PMDATA
KERNEL_ENTRY:		dd 0 ; 记录kernel的入口虚拟地址

pm_PROG_SEG_FIRST	equ $ - PMDATA
PROG_SEG_FIRST		dd 0 ; 记录kerne文件中第一个program segment在内存中的物理地址

pm_PROG_SEG_NUM		equ $ - PMDATA
PROG_SEG_NUM		dw 0 ; 记录kerne文件中program segment的数量

; 以下为在实模式下需要打印的字符串
MSG1:				db 13, 10, "Loading ...", 13, 10
MSG1_LEN			equ $ - MSG1

MSG2:				db 13, 10, "[[ NO KERNEL ]]", 13, 10, "Press any key to reboot now ...", 13, 10
MSG2_LEN			equ $ - MSG2

MSG3:				db "kernel"
MSG3_LEN			equ $ - MSG3

