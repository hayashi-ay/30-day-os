#include "bootpack.h"
#include "myfunc.h"

void HariMain(void)
{
	char *vram;
	int xsize, ysize;
	struct BOOTINFO *binfo;
	char s[40], mcursor[256];
	int mx, my;

	init_gdtidt();
	init_palette();
	binfo = (struct BOOTINFO *)0x0ff0;

	xsize = binfo->scrnx;
	ysize = binfo->scrny;
	vram = (char *) binfo->vram;


	boxfill8(vram, xsize, COL8_008484,  0,         0,          xsize -  1, ysize - 29);
	boxfill8(vram, xsize, COL8_C6C6C6,  0,         ysize - 28, xsize -  1, ysize - 28);
	boxfill8(vram, xsize, COL8_FFFFFF,  0,         ysize - 27, xsize -  1, ysize - 27);
	boxfill8(vram, xsize, COL8_C6C6C6,  0,         ysize - 26, xsize -  1, ysize -  1);

	boxfill8(vram, xsize, COL8_FFFFFF,  3,         ysize - 24, 59,         ysize - 24);
	boxfill8(vram, xsize, COL8_FFFFFF,  2,         ysize - 24,  2,         ysize -  4);
	boxfill8(vram, xsize, COL8_848484,  3,         ysize -  4, 59,         ysize -  4);
	boxfill8(vram, xsize, COL8_848484, 59,         ysize - 23, 59,         ysize -  5);
	boxfill8(vram, xsize, COL8_000000,  2,         ysize -  3, 59,         ysize -  3);
	boxfill8(vram, xsize, COL8_000000, 60,         ysize - 24, 60,         ysize -  3);

	boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 24, xsize -  4, ysize - 24);
	boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 23, xsize - 47, ysize -  4);
	boxfill8(vram, xsize, COL8_FFFFFF, xsize - 47, ysize -  3, xsize -  4, ysize -  3);
	boxfill8(vram, xsize, COL8_FFFFFF, xsize -  3, ysize - 24, xsize -  3, ysize -  3);

	mx = (binfo->scrnx - 16) / 2; // 画面中央になるように座標計算
	my = (binfo->scrny - 28 -16) / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);

	putfont8_asc(binfo->vram, binfo->scrnx, 8, 16, COL8_FFFFFF, "ABC 1234");
	putfont8_asc(binfo->vram, binfo->scrnx, 31, 39, COL8_000000, "Haribote OS.");
	putfont8_asc(binfo->vram, binfo->scrnx, 30, 38, COL8_FFFFFF, "Haribote OS.");

	sprintf(s, "scrnx = %d", binfo->scrnx);
	putfont8_asc(binfo->vram, binfo->scrnx, 16, 80, COL8_FFFFFF, s);


	for (;;) {
		_io_hlt();
	}
}
