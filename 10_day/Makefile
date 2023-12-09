ZTOOL = ../z_tools_osx

OBJS = naskfunc.o hankaku.o
CFILES = bootpack.c myfunc.c graphic.c dsctbl.c int.c fifo.c

haribote.img: ipl.bin  haribote.sys Makefile $(OBJS) $(CFILES)
	mformat -f 1440 -C -B ipl.bin -i haribote.img ::
	mcopy -i haribote.img haribote.sys ::

ipl.bin: ipl.nas Makefile
	nasm ipl.nas -o ipl.bin -l ipl.lst

asmhead.bin: asmhead.nas Makefile
	nasm asmhead.nas -o asmhead.bin -l asmhead.lst

hankaku.bin: hankaku.txt Makefile
	$(ZTOOL)/makefont hankaku.txt hankaku.bin

hankaku.o: hankaku.bin Makefile
	$(ZTOOL)/bin2obj hankaku.bin hankaku.o hankaku

%.o: %.nas Makefile
	nasm -f elf $*.nas -o $*.o -l $*.lst

bootpack.hrb: bootpack.c naskfunc.o hankaku.o os.lds Makefile
	i386-elf-gcc -march=i486 -m32 -nostdlib -fno-builtin -T os.lds -g $(CFILES) $(OBJS) -o bootpack.hrb

haribote.sys: asmhead.bin bootpack.hrb Makefile
	cat asmhead.bin bootpack.hrb > haribote.sys

run: haribote.img
	qemu-system-i386 -m 32M -drive file=haribote.img,if=floppy,format=raw -boot a

clean:
	rm ipl.bin ipl.lst
	rm asmhead.bin asmhead.lst
	rm naskfunc.o bootpack.hrb
	rm hankaku.bin hankaku.o

fclean:
	make clean
	rm haribote.img haribote.sys
