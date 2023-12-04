haribote.img: ipl.bin  haribote.sys Makefile
	mformat -f 1440 -C -B ipl.bin -i haribote.img ::
	mcopy -i haribote.img haribote.sys ::

ipl.bin: ipl.nas Makefile
	nasm ipl.nas -o ipl.bin -l ipl.lst

haribote.sys: haribote.nas Makefile
	nasm haribote.nas -o haribote.sys -l haribote.lst

run: haribote.img
	qemu-system-i386 -drive file=haribote.img,if=floppy,format=raw -boot a

clean:
	rm ipl.bin ipl.lst
	rm haribote.sys haribote.lst
