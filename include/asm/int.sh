;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 这些常量在加载中断描述符表的时候会用到
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PRIVILEGE_KERNEL		equ	0x0  ; ring0特权级
PRIVILEGE_USER			equ	0x3  ; ring3特权级

INT_GATE_386			equ 0xe  ; 80386类型的中断门
INT_TRAP_386			equ 0xf  ; 80386类型的陷阱门

DIVIDE_IV				equ	0x0  ; 除零异常的中断向量号
DEBUG_IV				equ 0x1  ; 调试异常的中断向量号
NMI_IV					equ 0x2  ; 非屏蔽外部中断向量号
BREAKPOINT_IV			equ 0x3  ; 调试断点中断向量号
OVERFLOW_IV				equ 0x4  ; 溢出中断向量号
BOUNDS_IV				equ 0x5  ; 越界中断向量号
INVAL_OP_IV				equ 0x6  ; 无效操作码中断向量号
COPROC_NOT_IV			equ 0x7  ; 设备不可用（无数学协处理器）中断向量号
DOUBLE_FAULT_IV			equ 0x8  ; 双重错误中断向量号
COPROC_SEG_IV			equ 0x9  ; 协处理器段越界中断向量号
INVAL_TSS_IV			equ 0xa  ; 无效TSS中断向量号
SEG_NOT_IV				equ 0xb  ; 段不存在中断向量号
STACK_FAULT_IV			equ 0xc  ; 堆栈段错误中断向量号
PROTECTION_IV			equ 0xd  ; 常规保护错误中断向量号
PAGE_FAULT_IV			equ 0xe  ; 页错误中断向量号
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; 这里的0xf向量号保留，未使用
COPROC_ERR_IV			equ 0x10 ; x87FPU浮点错中断向量号

IRQ0_IV					equ 0x20 ; IRQ0时钟对应的中断向量号
IRQ1_IV					equ 0x21 ; IRQ1键盘对应的中断向量号
IRQ2_IV					equ 0x22 ; 连接从8259A片
IRQ3_IV					equ 0x23 ; IRQ3串口2对应的中断向量号
IRQ4_IV					equ 0x24 ; IRQ4串口1对应的中断向量号
IRQ5_IV					equ 0x25 ; IRQ5LPT2对应的中断向量号
IRQ6_IV					equ 0x26 ; IRQ6软盘对应的中断向量号
IRQ7_IV					equ 0x27 ; IRQ7LPT1对应的中断向量号
IRQ8_IV					equ 0x28 ; IRQ8实时时钟对应的中断向量号
IRQ9_IV					equ 0x29 ; IRQ9重定向IRQ2
IRQ10_IV				equ 0x2a ; IRQ10保留
IRQ11_IV				equ 0x2b ; IRQ11保留
IRQ12_IV				equ 0x2c ; IRQ12ps/2鼠标对应的中断向量号
IRQ13_IV				equ 0x2d ; IRQ13FPU异常对应的中断向量号
IRQ14_IV				equ 0x2e ; IRQ14AT温彻斯特盘对应的中断向量号
IRQ15_IV				equ 0x2f ; IRQ15保留

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 一下是初始化8259A主片和从片的时候用到的操作端口
; 初始化8259A的过程如下：
; 往端口20h（主片）或A0h（从片）写入ICW1
; 往端口21h（主片）或A1h（从片）写入ICW2
; 往端口21h（主片）或A1h（从片）写入ICW3
; 往端口21h（主片）或A1h（从片）写入ICW4
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
MASTER_CTL_8259			equ 0x20 ;
SLAVE_CTL_8259			equ 0xa0 ;
MASTER_CTL_MASK_8259	equ	0x21 ;
SLAVE_CTL_MASK_8259		equ	0xa1 ;
