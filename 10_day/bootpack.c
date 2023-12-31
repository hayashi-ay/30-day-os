#include "bootpack.h"
#include "myfunc.h"

extern struct FIFO8 keyinfo;
extern struct FIFO8 mouseinfo;

struct MOUSE_DEC
{
	unsigned char buf[3], phase;
	int x, y, btn;
};

void enable_mouse(struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);
void init_keyboard(void);

void HariMain(void)
{
	char *vram;
	int xsize, ysize;
	struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
	char s[40], keybuf[32], mousebuf[128];
	int mx, my, i;
	struct MOUSE_DEC mdec;
	unsigned int memtotal;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
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
	sheet_slide(shtctl, sht_back, 0, 0);
	mx = (binfo->scrnx - 16) / 2; // 画面中央になるように座標計算
	my = (binfo->scrny - 28 - 16) / 2;
	sheet_slide(shtctl, sht_mouse, mx, my);
	sheet_updown(shtctl, sht_back, 0);
	sheet_updown(shtctl, sht_mouse, 1);
	sprintf(s, "memory %dMB free: %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfont8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
	sheet_refresh(shtctl, sht_back, 0, 0, binfo->scrnx, binfo->scrny);

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
				sheet_refresh(shtctl, sht_back, 0, 16, 16, 32);
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
					sheet_refresh(shtctl, sht_back, 32, 16, 32 + 15 * 8, 32);

					/* マウスカーソルの移動 */
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0)
					{
						mx = 0;
					}
					if (my < 0)
					{
						my = 0;
					}
					if (mx > binfo->scrnx - 16)
					{
						mx = binfo->scrnx - 16;
					}
					if (my > binfo->scrny - 16)
					{
						my = binfo->scrny - 16;
					}
					sheet_slide(shtctl, sht_mouse, mx, my);
				}
			}
		}
	}
}

#define PORT_KEYDAT 0x0060
#define PORT_KEYSTA 0x0064
#define PORT_KEYCMD 0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE 0x47

void wait_KBC_sendready(void)
{
	/* キーボードコントローラがデータ送信可能になるのを待つ */
	for (;;)
	{
		if ((_io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0)
		{
			break;
		}
	}
	return;
}

void init_keyboard(void)
{
	/* キーボードコントローラの初期化 */
	wait_KBC_sendready();
	_io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	_io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}

#define KEYCMD_SENDTO_MOUSE 0xd4
#define MOUSECMD_ENABLE 0xf4

void enable_mouse(struct MOUSE_DEC *mdec)
{
	/* マウス有効 */
	wait_KBC_sendready();
	_io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	_io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	mdec->phase = 0; // マウスの0xfaを待っているフェーズ
	return;			 /* うまくいくとACK(0xfa)が送信されてくる */
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
	if (mdec->phase == 0 && dat == 0xfa)
	{
		mdec->phase = 1;
		return 0;
	}
	else if (mdec->phase == 1)
	{
		mdec->buf[0] = dat;
		mdec->phase = 2;
		return 0;
	}
	else if (mdec->phase == 2)
	{
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	else if (mdec->phase == 3)
	{
		// 3バイトが揃った
		mdec->buf[2] = dat;
		mdec->phase = 1;
		// 下位3ビットがボタンの状態を表す
		mdec->btn = mdec->buf[0] & 0x07;
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];
		// x方向がマイナスの値なので、符号拡張
		if ((mdec->buf[0] & 0x10) != 0)
		{
			mdec->x |= 0xffffff00;
		}

		// yについて
		if ((mdec->buf[0] & 0x20) != 0)
		{
			mdec->y |= 0xffffff00;
		}

		// yは画面上の値とマウスの値が逆
		mdec->y = -mdec->y;

		return 1;
	}
}
