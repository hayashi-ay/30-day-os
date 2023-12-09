#include "bootpack.h"

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
