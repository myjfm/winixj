####指定kernel的程序入口点
ENTRYPOINT				= 0x0

ASM						= nasm
CC						= gcc
LD						= ld
ASM_KERNEL_FLAGS		= -I include/ -f elf ####内核指定编译为elf格式文件
CFLAGS					= -I include/ -c -fno-builtin ####不是用内建函数（不过在编译tools目录下的两个文件的时候不应该加该标志）
LDFLAGS					= -s -Ttext $(ENTRYPOINT) ####在链接的时候指定kernel的程序入口点

WINIXJ_BOOT				= boot/boot
WINIXJ_LOADER			= boot/loader
WINIXJ_KERNEL			= system
WINIXJ_SYSTEM_IMAGE		= System.Image ####生成的系统映像，由boot、loader、kernel.map三部分生成
WINIXJ_TOOLS			= tools/proc_kernel tools/buildImage

###############################################################################
###############################################################################
#####一定要保证所有的目标文件中init.o在最前，否则entry address不会是指定值#####
###############################################################################
###############################################################################
OBJS					= kernel/init.o kernel/int.o kernel/main.o kernel/process.o \
						  kernel/clock.o kernel/schedule.o kernel/sys_call.o \
						  mm/mm.o mm/buffer.o dev_drv/dev_drv.o dev_drv/hdd.o \
						  lib/string.o lib/getpid.o

.PHONY : everything all image disk clean realclean

####默认从该目标开始
everything : realclean image disk 

all : everything

####该目标是生成System.Image映像
image : $(WINIXJ_BOOT) $(WINIXJ_LOADER) $(WINIXJ_KERNEL) $(WINIXJ_TOOLS)
	tools/proc_kernel -r $(WINIXJ_KERNEL) -w system.map
	tools/buildImage -b boot/boot -l boot/loader -k system.map -w $(WINIXJ_SYSTEM_IMAGE)

####该目标将System.Image映像写入软盘a.img中
disk : $(WINIXJ_SYSTEM_IMAGE)
	dd if=$(WINIXJ_SYSTEM_IMAGE) of=a.img bs=`du -b System.Image | awk '{print $$1}'` count=1 conv=notrunc
	bochs -f bochsrc

tools/proc_kernel : tools/proc_kernel.c
	$(CC) -o $@ $<

tools/buildImage : tools/buildImage.c
	$(CC) -o $@ $<

$(WINIXJ_BOOT) : boot/boot.s
	$(ASM) -o $(WINIXJ_BOOT) $<

$(WINIXJ_LOADER) : boot/loader.s
	$(ASM) -o $(WINIXJ_LOADER) $<

kernel/init.o : kernel/init.s
	$(ASM) $(ASM_KERNEL_FLAGS) -o $@ $<

kernel/int.o : kernel/int.s
	$(ASM) $(ASM_KERNEL_FLAGS) -o $@ $<

kernel/main.o : kernel/main.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/process.o : kernel/process.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/clock.o : kernel/clock.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/schedule.o : kernel/schedule.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/sys_call.o : kernel/sys_call.c
	$(CC) $(CFLAGS) -o $@ $<

mm/mm.o : mm/mm.c
	$(CC) $(CFLAGS) -o $@ $<

mm/buffer.o : mm/buffer.c
	$(CC) $(CFLAGS) -o $@ $<

dev_drv/dev_drv.o : dev_drv/dev_drv.c
	$(CC) $(CFLAGS) -o $@ $<

dev_drv/hdd.o : dev_drv/hdd.c
	$(CC) $(CFLAGS) -o $@ $<

lib/string.o : lib/string.c
	$(CC) $(CFLAGS) -o $@ $<

lib/getpid.o : lib/getpid.c
	$(CC) $(CFLAGS) -o $@ $<

$(WINIXJ_KERNEL) : $(OBJS)
	$(LD) $(LDFLAGS) -o $(WINIXJ_KERNEL) $(OBJS)

clean :
	rm -f $(OBJS)
	rm -f system.map

realclean : 
	rm -f $(OBJS)
	rm -f $(WINIXJ_BOOT)
	rm -f $(WINIXJ_LOADER)
	rm -f $(WINIXJ_KERNEL) $(WINIXJ_SYSTEM_IMAGE) system.map
	rm -f $(WINIXJ_TOOLS)

