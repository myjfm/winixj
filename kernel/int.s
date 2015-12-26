%include "asm/int.sh"

extern boot_heartbeat
extern pre_schedule
extern validate_buffer
extern sys_call_table
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 到处中断向量表和中断描述符表寄存器
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
global idt
global idtr

global set_idt
global sys_call

global out_byte
global in_byte
global read_port
global write_port
global install_int_handler
global uninstall_int_handler
global install_sys_call_handler
global enable_hwint
global disable_hwint

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 处理器能够处理的默认中断和异常
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
global divide_error
global debug_exception
global nmi
global breakpoint_exception
global overflow
global bounds_check
global inval_opcode
global copr_not_available
global double_fault
global copr_seg_overrun
global inval_tss
global segment_not_present
global stack_exception
global general_protection
global page_fault
global copr_error
global exception

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 可屏蔽的硬件中断
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
global int_clock
global int_keyboard
global int_serial_port2
global int_serial_port1
global int_lpt2
global int_floppy
global int_lpt1
global int_rtc
global int_ps_2_mouse
global int_fpu_fault
global int_at_win
global int_default

[SECTION .text]
set_idt:
	; 对8259A主片写入ICW1
	push 0x11
	push MASTER_CTL_8259
	call out_byte
	add esp, 4 * 2

	; 对8259A从片写入ICW1
	push 0x11
	push SLAVE_CTL_8259
	call out_byte
	add esp, 4 * 2

	; 设置8259A主片的中断入口地址，为IRQ0_IV
	push IRQ0_IV
	push MASTER_CTL_MASK_8259
	call out_byte
	add esp, 4 * 2

	; 设置8259A从片的中断入口地址，为IRQ8_IV
	push IRQ8_IV
	push SLAVE_CTL_MASK_8259
	call out_byte
	add esp, 4 * 2

	; 向8259A主片写入ICW3，表明IR2处级联了从片
	push 0x4
	push MASTER_CTL_MASK_8259
	call out_byte
	add esp, 4 * 2

	; 向8259A从片写入ICW3，表明从片是连接在主片的IR2处
	push 0x2
	push SLAVE_CTL_MASK_8259
	call out_byte
	add esp, 4 * 2

	; 向8259A主片写入ICW4
	push 0x1
	push MASTER_CTL_MASK_8259
	call out_byte
	add esp, 4 * 2

	; 向8259A从片写入ICW4
	push 0x1
	push SLAVE_CTL_MASK_8259
	call out_byte
	add esp, 4 * 2

	; 屏蔽8259A主片的所有硬件中断
	push 0xff
	push MASTER_CTL_MASK_8259
	call out_byte
	add esp, 4 * 2

	; 屏蔽8259A从片的所有硬件中断
	push 0xff
	push SLAVE_CTL_MASK_8259
	call out_byte
	add esp, 4 * 2

	; 除零错误（内核态允许的中断）
	; 指令div or idiv
	push PRIVILEGE_KERNEL
	push divide_error
	push INT_GATE_386
	push DIVIDE_IV
	call init_idt
	add esp, 4 * 4

	; 调试异常（内核态允许的中断）
	push PRIVILEGE_KERNEL
	push debug_exception
	push INT_GATE_386
	push DEBUG_IV
	call init_idt
	add esp, 4 * 4

	; 非屏蔽中断（内核态允许的中断）
	push PRIVILEGE_KERNEL
	push nmi
	push INT_GATE_386
	push NMI_IV
	call init_idt
	add esp, 4 * 4

	; 调试断点异常（用户态允许的中断）
	; 指令int3
	push PRIVILEGE_USER
	push breakpoint_exception
	push INT_GATE_386
	push BREAKPOINT_IV
	call init_idt
	add esp, 4 * 4

	; 溢出异常（用户态允许的中断）
	; 指令into
	push PRIVILEGE_USER
	push overflow
	push INT_GATE_386
	push OVERFLOW_IV
	call init_idt
	add esp, 4 * 4

	; 越界错误（内核态允许的中断）
	; linux将其设置为用户态也允许的中断
	; 指令bound
	push PRIVILEGE_KERNEL
	push bounds_check
	push INT_GATE_386
	push BOUNDS_IV
	call init_idt
	add esp, 4 * 4

	; 无效操作码错误（内核态允许的中断）
	; 主要由ud2或无效指令引起
	push PRIVILEGE_KERNEL
	push inval_opcode
	push INT_GATE_386
	push INVAL_OP_IV
	call init_idt
	add esp, 4 * 4

	; 设备不可用/无数学协处理器（内核态允许的中断）
	; 浮点数或wait/fwait指令
	push PRIVILEGE_KERNEL
	push copr_not_available
	push INT_GATE_386
	push COPROC_NOT_IV
	call init_idt
	add esp, 4 * 4

	; 双重错误（内核态允许的中断）
	; 所有能产生异常或NMI或intr的指令
	push PRIVILEGE_KERNEL
	push double_fault
	push INT_GATE_386
	push DOUBLE_FAULT_IV
	call init_idt
	add esp, 4 * 4

	; 386机器不再产生此种异常
	push PRIVILEGE_KERNEL
	push copr_seg_overrun
	push INT_GATE_386
	push COPROC_SEG_IV
	call init_idt
	add esp, 4 * 4

	; 无效TSS错误（内核态允许的中断）
	; 任务切换或访问TSS段时
	push PRIVILEGE_KERNEL
	push inval_tss
	push INT_GATE_386
	push INVAL_TSS_IV
	call init_idt
	add esp, 4 * 4

	; 段不存在错误（内核态允许的中断）
	; 加载段寄存器或访问系统段时
	push PRIVILEGE_KERNEL
	push segment_not_present
	push INT_GATE_386
	push SEG_NOT_IV
	call init_idt
	add esp, 4 * 4

	; 堆栈段错误（内核态允许的中断）
	; 堆栈段操作或加载ss时
	push PRIVILEGE_KERNEL
	push stack_exception
	push INT_GATE_386
	push STACK_FAULT_IV
	call init_idt
	add esp, 4 * 4

	; 常规保护错误（内核态允许的中断）
	; 内存或其他保护检验时
	push PRIVILEGE_KERNEL
	push general_protection
	push INT_GATE_386
	push PROTECTION_IV
	call init_idt
	add esp, 4 * 4

	; 页错误（内核态允许的中断）
	; 内存访问时
	push PRIVILEGE_KERNEL
	push page_fault
	push INT_GATE_386
	push PAGE_FAULT_IV
	call init_idt
	add esp, 4 * 4


	;;;;;;;;;;;;;;;;注意这里0x0f号中断保留，未使用


	; x87FPU浮点错误（内核态允许的中断）
	; x87FPU浮点指令或WAIT/FWAIT指令
	push PRIVILEGE_KERNEL
	push copr_error
	push INT_GATE_386
	push COPROC_ERR_IV
	call init_idt
	add esp, 4 * 4

	; 从0x20开始到中断向量表尾部，统一初始化成默认的中断处理程序
	mov ecx, IRQ0_IV
	push PRIVILEGE_KERNEL
	push int_default
	push INT_GATE_386
init_rest:
	push ecx
	call init_idt
	pop ecx
	inc ecx
	cmp ecx, 255
	jna init_rest
	add esp, 4 * 3

	; 全部中断向量入口程序加载完成之后便加载中断描述符表
	lidt [idtr] ; 加载中断描述符表
	ret

init_idt:
	mov eax, [esp + 4 * 1] ; 中断向量号
	mov ebx, [esp + 4 * 2] ; 描述符类型（中断门/调用门/陷阱门）
	mov ecx, [esp + 4 * 3] ; 中断处理程序入口
	mov edx, [esp + 4 * 4] ; 特权级
	mov esi, idt
	shl eax, 3
	add esi, eax ; 中断向量号乘以8然后加上idt基地址就能找到对用中断向量号的idt描述符
	mov word [esi], cx
	add esi, 2
	mov word [esi], 0x8 ; CS段描述符
	add esi, 2
	mov byte [esi], 0x0
	add esi, 1
	shl edx, 5
	and bl, 0x0f
	or bl, 0x80
	or bl, dl
	mov byte [esi], bl
	add esi, 1
	shr ecx, 16
	mov word [esi], cx
	ret
	
; 在发生中断时，eflags、cs、eip将自动被压入栈中
; 如果有出错码的话，那么出错码紧接着继续被压入栈中（同样被自动压入栈中）
; 如果有堆栈切换，也就是说有特权级变化，那么原ss和esp将被压入内层堆栈，之后才是eflags、cs、eip
; 从中断或者异常中返回时必须用iretd，它与ret不同的时它会改变eflags的值
divide_error:
	push 0xffffffff
	push DIVIDE_IV
	jmp exception

debug_exception:
	push 0xffffffff
	push DEBUG_IV
	jmp exception

nmi:
	push 0xffffffff
	push NMI_IV
	jmp exception

breakpoint_exception:
	push 0xffffffff
	push BREAKPOINT_IV
	jmp exception

overflow:
	push 0xffffffff
	push OVERFLOW_IV
	jmp exception

bounds_check:
	push 0xffffffff
	push BOUNDS_IV
	jmp exception

inval_opcode:
	push 0xffffffff
	push INVAL_OP_IV
	jmp exception

copr_not_available:
	push 0xffffffff
	push COPROC_NOT_IV
	jmp exception

double_fault:
	push DOUBLE_FAULT_IV
	jmp exception

copr_seg_overrun:
	push 0xffffffff
	push COPROC_SEG_IV
	jmp exception

inval_tss: ; 系统将出错码自动压栈
	push INVAL_TSS_IV
	jmp exception

segment_not_present: ; 系统将出错码自动压栈
	push SEG_NOT_IV
	jmp exception

stack_exception: ; 系统将出错码自动压栈
	push STACK_FAULT_IV
	jmp exception

general_protection: ; 系统将出错码自动压栈
	push PROTECTION_IV
	jmp $
	jmp exception

page_fault: ; 系统将出错码自动压栈
	push PAGE_FAULT_IV
	jmp exception

copr_error:
	push 0xffffffff
	push COPROC_ERR_IV
	jmp exception

exception:
	add esp, 4 * 2 ; 跳过出错码和向量号
	cli
	hlt ; 我们目前不处理错误，只要出错就让机器hlt
	;iretd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 每个进程的内核态堆栈顶部栈帧应该是这样的
; ss
; esp
; eflags
; cs
; eip
; eax
; ecx
; edx
; ebx
; ebp
; esi
; edi
; ds
; es
; fs
; gs
; 其中ss、esp、eflags、cs、eip是在发生中断时CPU自动压栈的
; 而其他的是由中断程序压栈的，这个顺序不能改变，否则后果自负
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

CS_OFFSET	equ 0x30
ESP_OFFSET	equ 0x38
SS_OFFSET	equ 0x3c

; 一个宏，因为所有的irq中断函数都是先保存现场并将数据段等堆栈段
; 切换到内核态，因此，该操作所有的irq中断入口函数均相同
; 故写成宏节省空间^_!
%macro save_all 0
	push eax
	push ecx
	push edx
	push ebx
	push ebp
	push esi
	push edi
	push ds
	push es
	push fs
	push gs
	mov si, ss
	mov ds, si
	mov es, si
	mov gs, si
	mov fs, si
%endmacro

; 一个宏，恢复现场
%macro recover_all 0
	pop gs
	pop fs
	pop es
	pop ds
	pop edi
	pop esi
	pop ebp
	pop ebx
	pop edx
	pop ecx
	pop eax
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 时钟中断处理程序
; 这是整个系统中最要求“速度快”的程序，因为时钟中断没隔1/HZ(s)
; 就发生一次，大概它是整个系统调用最频繁的函数，所以需要该函数
; 尽量短，没有必要的函数调用尽量避免。
; 另外判断中断重入minix和linux采取的方法也是不一样的，minix采用
; 一个全局变量，类似于信号量的概念；而linux的方法则比较简单，它
; 直接获取存储在内核堆栈中的cs段寄存器的RPL值来判断被中断的程序
; 是内核态程序的还是用户态的进程；我们打算采用linux的办法，虽然
; minix方法更酷，但是linux的显然更加简单:)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
int_clock:
	save_all
	; 增加心跳数
	inc dword [boot_heartbeat]

	; 发送EOI指令结束本次中断
	mov ax, 0x20
	out 0x20, al
	sti
	
	mov eax, [esp + CS_OFFSET]
	and eax, 0x03
	cmp eax, 0x0 ; 如果CS段寄存器的RPL为0，则说明是由内核态进入时钟中断，则是中断重入
	je return
	call pre_schedule
return:
	recover_all
	iretd

int_keyboard:
	save_all

	recover_all
	iretd

int_serial_port2:
	save_all

	recover_all
	iretd

int_serial_port1:
	save_all

	recover_all
	iretd

int_lpt2:
	save_all

	recover_all
	iretd

int_floppy:
	save_all

	recover_all
	iretd

int_lpt1:
	save_all

	recover_all
	iretd

int_rtc:
	save_all

	recover_all
	iretd

int_ps_2_mouse:
	save_all

	recover_all
	iretd

int_fpu_fault:
	save_all

	recover_all
	iretd

;硬盘中断处理程序
int_at_win:
	save_all

	mov byte [gs:0xb8006], 'e'; 试验硬盘中断是否成功:)

	; 发送EOI指令给从8259A结束本次中断
	mov ax, 0x20
	out 0xa0, al
	nop
	nop
	; 发送EOI指令给主8259A结束本次中断
	out 0x20, al
	nop
	nop

	; 调用该函数使buf_info缓冲区生效
	call validate_buffer

	recover_all
	iretd

; 默认的中断处理函数，所有的未定义中断都会调用此函数
int_default:
	save_all
	recover_all
	iretd

; 注意从系统调用返回时不需要从栈中弹出eax的值，因为eax保存着调用
; 对应系统调用之后的返回值
%macro recover_from_sys_call 0
	pop gs
	pop fs
	pop es
	pop ds
	pop edi
	pop esi
	pop ebp
	pop ebx
	pop edx
	pop ecx
	add esp, 4 * 1
%endmacro

; 系统调用框架，系统调用采用0x30号中断向量，利用int 0x30指令产
; 生一个软中断，之后便进入sys_call函数，该函数先调用save_all框
; 架保存所有寄存器值，然后调用对应系统调用号的入口函数完成系统调用
; 注意！！！！！系统调用默认有三个参数，分别利用ebx、ecx、edx来
; 传递，其中eax保存系统调用号
sys_call:
	save_all

	sti

	push ebx
	push ecx
	push edx
	call [sys_call_table + eax * 4]
	add esp, 4 * 3

	recover_from_sys_call

	cli

	iretd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 以下为库函数
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; 对端口进行写操作
; void out_byte(unsigned short port, unsigned char value);
out_byte:
	mov edx, [esp + 4 * 1]
	mov al, [esp + 4 * 2]
	out dx, al
	nop
	nop
	ret

; 对端口进行读操作
; uint8 in_byte(unsigned short port);
in_byte:
	mov edx, [esp + 4 * 1]
	xor eax, eax
	in al, dx
	nop
	nop
	ret

; 对从指定端口进行读操作，读出的n个字节数据放入buf缓冲区中
; void read_port(uint16 port, void* buf, int n);
read_port:
	mov	edx, [esp + 4 * 1]	; port
	mov	edi, [esp + 4 * 2]	; buf
	mov	ecx, [esp + 4 * 3]	; n
	shr	ecx, 1
	cld
	rep	insw
	ret

; 对从指定端口进行写操作，数据源在buf缓冲区中，写n个字节
; void write_port(uint16 port, void* buf, int n);
write_port:
	mov	edx, [esp + 4 * 1]	; port
	mov	edi, [esp + 4 * 2]	; buf
	mov	ecx, [esp + 4 * 3]	; n
	shr	ecx, 1
	cld
	rep	outsw
	ret

; 安装指定中断号的中断处理程序
; extern int install_int_handler(uint8 INT_IV, void* handler);
install_int_handler:
	mov eax, [esp + 4 * 1] ; 中断向量号
	mov ebx, [esp + 4 * 2] ; 中断程序入口
	cmp eax, 256
	jae failed
	cmp eax, 0
	jbe failed
	push PRIVILEGE_KERNEL
	push ebx
	push INT_GATE_386
	push eax
	call init_idt
	add esp, 4 * 4
failed:
	ret
	
; 卸载指定中断号的中断处理程序
; extern int uninstall_int_handler(uint8 INT_IV);
uninstall_int_handler:
	mov eax, [esp + 4 * 1] ; 中断向量号
	cmp eax, 256
	jae failed
	cmp eax, 0
	jbe failed
	push PRIVILEGE_KERNEL
	push int_default
	push INT_GATE_386
	push eax
	call init_idt
	add esp, 4 * 4
	ret
	
; 安装指定中断号的系统调用入口
; extern int install_sys_call_handler(uint8 INT_IV, void* handler);
install_sys_call_handler:
	mov eax, [esp + 4 * 1] ; 中断向量号
	mov ebx, [esp + 4 * 2] ; 中断程序入口
	cmp eax, 256
	jae failed_inst_sys
	cmp eax, 0
	jbe failed_inst_sys
	push PRIVILEGE_USER
	push ebx
	push INT_TRAP_386
	push eax
	call init_idt
	add esp, 4 * 4
failed_inst_sys:
	ret

; 打开对应向量号的硬件中断
; 注意，这里传入的参数是硬件中断对应的中断向量号
; 需要将该中断向量号转化为在8259A上的索引号
; void enable_hwint(uint8 IV);
enable_hwint:
	mov ecx, [esp + 4 * 1]
	cmp cl, IRQ0_IV
	jae master_1
	jmp ret_1
master_1:
	cmp cl, IRQ8_IV
	jae slave_1
	push MASTER_CTL_MASK_8259
	call in_byte
	add esp, 4 * 1
	sub cl, IRQ0_IV
	mov bl, 1
	shl bl, cl
	xor bl, 0xff
	and al, bl
	push eax
	push MASTER_CTL_MASK_8259
	call out_byte
	add esp, 4 * 2
	jmp ret_1
slave_1:
	cmp cl, IRQ15_IV
	ja ret_1
	push SLAVE_CTL_MASK_8259
	call in_byte
	add esp, 4 * 1
	sub cl, IRQ8_IV
	mov bl, 1
	shl bl, cl
	xor bl, 0xff
	and al, bl
	push eax
	push SLAVE_CTL_MASK_8259
	call out_byte
	add esp, 4 * 2
ret_1:
	ret

; 关闭对应向量号的硬件中断
; 注意，这里传入的参数是硬件中断对应的中断向量号
; 需要将该中断向量号转化为在8259A上的索引号
; void disable_hwint(uint8 IV);
disable_hwint:
	mov ecx, [esp + 4 * 1]
	cmp cl, IRQ0_IV
	jae master_2
	jmp ret_2
master_2:
	cmp cl, IRQ8_IV
	jae slave_2
	push MASTER_CTL_MASK_8259
	call in_byte
	add esp, 4 * 1
	sub cl, IRQ0_IV
	mov bl, 1
	shl bl, cl
	or al, bl
	push eax
	push MASTER_CTL_MASK_8259
	call out_byte
	add esp, 4 * 2
	jmp ret_2
slave_2:
	cmp cl, IRQ15_IV
	ja ret_2
	push SLAVE_CTL_MASK_8259
	call in_byte
	add esp, 4 * 1
	sub cl, IRQ8_IV
	mov bl, 1
	shl bl, cl
	or al, bl
	push eax
	push SLAVE_CTL_MASK_8259
	call out_byte
	add esp, 4 * 2
ret_2:
	ret

[SECTION .data]
idt:
		; idt表共可存放256个中断门描述符
		times 256 * 8 db 0

idtr:	dw $ - idt - 1
		dd idt
