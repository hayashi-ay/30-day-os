#include "bootpack.h"
#include "myfunc.h"

extern struct FIFO8 keyinfo;

void HariMain(void)
{
	char *vram;
	int xsize, ysize;
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40], mcursor[256], keybuf[32];
	int mx, my, i;

	init_gdtidt();
	init_pic();
	_io_sti(); // IDT/PICの初期化が終わったのでCPUの割込み禁止を解除

	init_palette();
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

	mx = (binfo->scrnx - 16) / 2; // 画面中央になるように座標計算
	my = (binfo->scrny - 28 -16) / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);

	_io_out8(PIC0_IMR, 0xf9); /* PIC1とキーボードを許可(11111001) */
	_io_out8(PIC1_IMR, 0xef); /* マウスを許可(11101111) */

	fifo8_init(&keyinfo, 32, keybuf);

	int count = 0;

	for (;;) {
		_io_cli(); // 割り込み禁止
		if (fifo8_status(&keyinfo) == 0) {
			_io_stihlt();
		} else {
			i = fifo8_get(&keyinfo);
			_io_sti();
			sprintf(s, "%x", i);
			boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0 + count * 16, 16, 15, 31);
			putfont8_asc(binfo->vram, binfo->scrnx, 0 + count * 16, 16, COL8_FFFFFF, s);
			count += 1;
		}
	}
}
