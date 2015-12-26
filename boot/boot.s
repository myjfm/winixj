;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 该文件主要用于加载loader和kernel入内存，其中默认loader和kernel是被压缩成System.Image
; 文件存放到软盘的第二个扇区起始的地方，loader最大大小不应超过128KB，loader+kernel不应
; 超过512KB - 32KB = 480KB大小，否则系统将崩溃。
; 
; Author : myjfm(mwxjmmyjfm@gmail.com)
; v0.01 2011/11/22
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
org 0x7c00
jmp start

; loader和kernel映像一起被加载到
; 内存段基址:0x8000 (* 0x10)
; 即在1MB寻址范围的正中间
; 将来还会从0xf0000地址起始处放置一些系统参数
; 因此loader+kernel大小不应超过512KB - 32KB = 480KB，如果
; 超过这个数字那么地址将回滚，系统崩溃
LOADER_ADDR equ 0x8000

start:
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, 0x7b00 ; 栈顶为0x7b00，向低地址延伸;其实boot中我们没有用到栈，因此不需设置堆栈
	
	; 清屏
	mov ax, 0x0600
	mov bx, 0x0700
	mov cx, 0
	mov dx, 0x184f
	int 0x10

	; 设置光标位置
	mov ah, 0x02
	mov bh, 0x00
	mov dx, 0x0
	int 0x10

get_disk_param:
	mov dl, 0x00 ; 读软盘
	mov ax, 0x0800 ; 获取磁盘参数
	int 0x13
	jnc goon_get_disk_param ; 如果读磁盘驱动器参数成功则保存相关参数
	mov dx, 0x0000 ; 出错则将磁盘复位然后无限循环读
	mov ax, 0x0000
	int 0x13
	jmp get_disk_param ; 出错则将磁盘复位然后无限循环读

goon_get_disk_param:
	mov bl, cl
	and bl, 0x3f
	mov byte [SECTOR_PER_TRACK], bl ; SECTOR_PER_TRACK保存每磁道最大扇区数
	shr cx, 6
	mov word [TRACK_NUM], cx ; TRACK_NUM保存最大磁道号

	; 获取当前光标位置
	mov ah, 0x03
	xor bh, bh
	int 0x10

	; 打印一些信息
	mov ax, cs
	mov es, ax
	mov cx, MSG1_LEN
	mov bx, 0x0007
	mov bp, MSG1 ; 显示字符串"Loading System ..."
	mov ax, 0x1301
	int 0x10

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 加载loader和kernel文件，存放位置在0x80000起始的地方
; loader+kernel大小不应超过480KB，否则由于在实模式下仅有
; 1MB的寻址能力，loader+kernel的存放空间将超过0x80000~0xf0000的大小
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

load_loader_and_kernel:
	mov ax, 0x00
	mov byte al, [LOADER_LEN]
	add word ax, [KERNEL_LEN]
	mov word [CUR_UNREAD_SECTOR], ax ; 初始化当前未加载的扇区数
	mov ax, LOADER_ADDR
	mov es, ax
	mov bx, 0x00 ; es:bx指向loader+kernel映像加载的位置:0x8000 * 0x10 + 0x0

read_one_track:
	mov word ax, [CUR_TRACK]
	mov ch, al ; 当前读到的磁道号的低8位
	shl ah, 0x06
	mov cl, ah
	mov byte al, [CUR_READ]
	inc al
	or cl, al ; cl寄存器高两位为当前读到的磁道号的高两位，低六位为开始扇区号
	mov byte dh, [CUR_HEAD] ; 当前的磁头号
	mov dl, 0x00 ; 驱动器号为0，代表是软盘
	mov ah, 0x02 ; 读扇区
	mov byte al, [SECTOR_PER_TRACK]
	sub byte al, [CUR_READ] ; 要读取的扇区数量
	int 0x13
	mov byte cl, [SECTOR_PER_TRACK]
	sub byte cl, [CUR_READ]
	cmp al, cl
	jne die ; 如果实际读出的扇区数比所请求的扇区数少，则报错
	xor ah, ah
	sub word [CUR_UNREAD_SECTOR], ax ; 修改当前剩余的未读扇区数
	jc finish_loading
	cmp word [CUR_UNREAD_SECTOR], 0x00
	je finish_loading ; 如果已经读完loader和kernel，则跳出
	shl ax, 0x09 ; al * 512代表本次读出的字节数
	add bx, ax
	jno no_overflow
	mov cx, es
	add cx, 0x1000
	cmp cx, 0xf000
	ja die ; es已经超过了1MB的界限，说明loader+kernel过大
	mov es, cx ; 修改es:bx缓冲区地址
no_overflow:
	mov byte [CUR_READ], 0x0
	cmp byte [CUR_HEAD], 0x0
	je read_head_1
	add byte [CUR_TRACK], 0x01
	mov word ax, [TRACK_NUM]
	cmp word [CUR_TRACK], ax
	ja die
read_head_1:
	add byte [CUR_HEAD], 0x01
	and byte [CUR_HEAD], 0x01 ; 磁头号由0转为1，或者由1转为0
	jmp read_one_track

die:
	; 获取当前光标位置
	mov ah, 0x03
	xor bh, bh
	int 0x10

	; 打印一些信息
	mov ax, cs
	mov es, ax
	mov cx, MSG2_LEN
	mov bx, 0x0007
	mov bp, MSG2 ; 显示字符串"The loader or kernel is too long ..."
	mov ax, 0x1301
	int 0x10

	xor ax, ax
	int 0x16
	int 0x19
	jmp $

finish_loading:
	; 关闭软驱马达
	mov dx, 0x03f2
	mov al, 0
	out dx, al
	nop
	
	; 获取当前光标位置
	mov ah, 0x03
	xor bh, bh
	int 0x10

	; 打印一些信息
	mov ax, cs
	mov es, ax
	mov cx, MSG3_LEN
	mov bx, 0x0007
	mov bp, MSG3 ; 显示字符串"Success to load the loader and kernel\n\nEntering loader ..."
	mov ax, 0x1301
	int 0x10

	jmp LOADER_ADDR:0 ; 跳转到loader去执行

; 要显示的字符串
MSG1:				db 13, 10, "Loading loader and kernel ...", 13, 10
MSG1_LEN			equ $ - MSG1
MSG2:				db 13, 10, "The loader or kernel is too long ...", 13, 10
MSG2_LEN			equ $ - MSG2
MSG3:				db 13, 10, "Entering loader ...", 13, 10
MSG3_LEN			equ $ - MSG3

CUR_READ:			db 1 ; 当前磁道已经读出的扇区数
CUR_HEAD:			db 0 ; 当前的磁头号
CUR_TRACK:			dw 0 ; 当前读到的磁道号
CUR_UNREAD_SECTOR	dw 0 ; 当前剩余的未读扇区数

TRACK_NUM:			dw 0 ; 最大磁道号
SECTOR_PER_TRACK:	db 0 ; 每磁道扇区数

times 507 - ($ - $$) db 0 ; 填充剩余容量为0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 将loader文件的大小和kernel文件的大小都
; 放到boot文件oxaa55前的末尾
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; loader文件所占用的扇区数，loader不会大过
; 2^8 * 512 = 128KB大小，这在短期内应该足够了
LOADER_LEN			db 0
; kernel文件所占用的扇区数，kernel不会大过
; 2^16 * 512 = 32MB大小，这在用1.44MB大小的软盘里足够了
KERNEL_LEN			dw 0

dw 0xaa55
