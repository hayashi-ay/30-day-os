#include "bootpack.h"
#include "myfunc.h"

extern struct FIFO8 keyinfo;
extern struct FIFO8 mouseinfo;

void HariMain(void)
{
	char *vram;
	int xsize, ysize;
	struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
	char s[40], keybuf[32], mousebuf[128];
	int mx, my, i;
	struct MOUSE_DEC mdec;
	unsigned int memtotal;
	struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
	struct SHTCTL *shtctl;
	struct SHEET *sht_back, *sht_mouse;
	unsigned char *buf_back, buf_mouse[256];

	init_gdtidt();
	init_pic();
	_io_sti(); // IDT/PICの初期化が終わったのでCPUの割込み禁止を解除
	fifo8_init(&keyinfo, 32, keybuf);
	fifo8_init(&mouseinfo, 128, mousebuf);
	_io_out8(PIC0_IMR, 0xf9); /* PIC1とキーボードを許可(11111001) */
	_io_out8(PIC1_IMR, 0xef); /* マウスを許可(11101111) */

	init_keyboard();
	enable_mouse(&mdec);
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	// memman_freeの2行があまり分かっていない
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	init_palette();
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	sht_back = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
	buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, COL8_008484);
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(buf_mouse, COL8_008484);
	sheet_slide(sht_back, 0, 0);
	mx = (binfo->scrnx - 16) / 2; // 画面中央になるように座標計算
	my = (binfo->scrny - 28 - 16) / 2;
	sheet_slide(sht_mouse, mx, my);
	sheet_updown(sht_back, 0);
	sheet_updown(sht_mouse, 1);
	sprintf(s, "memory %dMB free: %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfont8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
	sheet_refresh(sht_back, 0, 0, binfo->scrnx, binfo->scrny);

	for (;;)
	{
		_io_cli(); // 割り込み禁止
		if (fifo8_status(&keyinfo) == 0 && fifo8_status(&mouseinfo) == 0)
		{
			_io_stihlt();
		}
		else
		{
			if (fifo8_status(&keyinfo) != 0)
			{
				i = fifo8_get(&keyinfo);
				_io_sti();
				sprintf(s, "%x", i);
				boxfill8(buf_back, binfo->scrnx, COL8_000000, 0, 16, 15, 31);
				putfont8_asc(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
				sheet_refresh(sht_back, 0, 16, 16, 32);
			}
			else if (fifo8_status(&mouseinfo) != 0)
			{
				i = fifo8_get(&mouseinfo);
				_io_sti();
				if (mouse_decode(&mdec, i) == 1)
				{
					sprintf(s, "%x %x %x", mdec.x, mdec.y, mdec.btn);
					boxfill8(buf_back, binfo->scrnx, COL8_000000, 32, 16, 32 + 8 * 8 - 1, 31);
					putfont8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
					sheet_refresh(sht_back, 32, 16, 32 + 15 * 8, 32);

					/* マウスカーソルの移動 */
					mx += mdec.x;
					my += mdec.y;
					// なんか本と値が異なる
					if (mx < -15)
						mx = -15;
					if (my < 0)
						my = 0;
					if (mx > binfo->scrnx - 1)
						mx = binfo->scrnx - 1;
					if (my > binfo->scrny - 1)
						my = binfo->scrny - 1;
					sheet_slide(sht_mouse, mx, my);
				}
			}
		}
	}
}
