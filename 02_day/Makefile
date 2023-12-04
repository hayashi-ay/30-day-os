helloos.img: ipl.bin Makefile
	mformat -f 1440 -C -B ipl.bin -i helloos.img ::

ipl.bin: ipl.nas Makefile
	nasm ipl.nas -o ipl.bin -l ipl.lst

run: helloos.img
	qemu-system-i386 -drive file=helloos.img,if=floppy,format=raw -boot a

clean:
	rm ipl.bin ipl.lst