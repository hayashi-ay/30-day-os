haribote.img: ipl.bin  haribote.sys Makefile
	mformat -f 1440 -C -B ipl.bin -i haribote.img ::
	mcopy -i haribote.img haribote.sys ::

ipl.bin: ipl.nas Makefile
	nasm ipl.nas -o ipl.bin -l ipl.lst

asmhead.bin: asmhead.nas Makefile
	nasm asmhead.nas -o asmhead.bin -l asmhead.lst

bootpack.hrb: bootpack.c os.lds Makefile
	i386-elf-gcc -march=i486 -m32 -nostdlib -T os.lds bootpack.c -o bootpack.hrb

haribote.sys: asmhead.bin bootpack.hrb Makefile
	cat asmhead.bin bootpack.hrb > haribote.sys

run: haribote.img
	qemu-system-i386 -drive file=haribote.img,if=floppy,format=raw -boot a

clean:
	rm ipl.bin ipl.lst
	rm asmhead.bin asmhead.lst
