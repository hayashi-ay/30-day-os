#include "bootpack.h"
#include "myfunc.h"

extern struct FIFO8 keyinfo;
extern struct FIFO8 mouseinfo;

void enable_mouse(void);
void init_keyboard(void);

void HariMain(void)
{
	char *vram;
	int xsize, ysize;
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40], mcursor[256], keybuf[32], mousebuf[128];
	int mx, my, i;

	init_gdtidt();
	init_pic();
	_io_sti(); // IDT/PICの初期化が終わったのでCPUの割込み禁止を解除
	fifo8_init(&keyinfo, 32, keybuf);
	fifo8_init(&mouseinfo, 128, mousebuf);
	_io_out8(PIC0_IMR, 0xf9); /* PIC1とキーボードを許可(11111001) */
	_io_out8(PIC1_IMR, 0xef); /* マウスを許可(11101111) */

	init_keyboard();

	init_palette();
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

	mx = (binfo->scrnx - 16) / 2; // 画面中央になるように座標計算
	my = (binfo->scrny - 28 -16) / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);

	enable_mouse();

	for (;;) {
		_io_cli(); // 割り込み禁止
		if (fifo8_status(&keyinfo) == 0 && fifo8_status(&mouseinfo) == 0) {
			_io_stihlt();
		} else {
			if (fifo8_status(&keyinfo) != 0) {
				i = fifo8_get(&keyinfo);
				_io_sti();
				sprintf(s, "%x", i);
				boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 16, 15, 31);
				putfont8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
			} else if (fifo8_status(&mouseinfo) != 0) {
				i = fifo8_get(&mouseinfo);
				_io_sti();
				sprintf(s, "%x", i);
				boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 32, 16, 47, 31);
				putfont8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
			}
		}
	}
}

#define PORT_KEYDAT				0x0060
#define PORT_KEYSTA				0x0064
#define PORT_KEYCMD				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47

void wait_KBC_sendready(void)
{
	/* キーボードコントローラがデータ送信可能になるのを待つ */
	for (;;) {
		if ((_io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
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

#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4

void enable_mouse(void)
{
	/* マウス有効 */
	wait_KBC_sendready();
	_io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	_io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	return; /* うまくいくとACK(0xfa)が送信されてくる */
}
