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

#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000

unsigned int memtest(unsigned int start, unsigned int end);
unsigned int memtest_sub(unsigned int start, unsigned int end);

#define MEMMAN_FREES 4090 /* これで約32KB */
#define MEMMAN_ADDR 0x003c0000

struct FREEINFO
{ /* あき情報 */
	unsigned int addr, size;
};

struct MEMMAN
{ /* メモリ管理 */
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};

void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);

void HariMain(void)
{
	char *vram;
	int xsize, ysize;
	struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
	char s[40], mcursor[256], keybuf[32], mousebuf[128];
	int mx, my, i;
	struct MOUSE_DEC mdec;
	unsigned int memtotal;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

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
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);

	enable_mouse(&mdec);

	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	// memman_freeの2行があまり分かっていない
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	sprintf(s, "memory %dMB free: %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfont8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

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
				boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 16, 15, 31);
				putfont8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
			}
			else if (fifo8_status(&mouseinfo) != 0)
			{
				i = fifo8_get(&mouseinfo);
				_io_sti();
				if (mouse_decode(&mdec, i) == 1)
				{
					sprintf(s, "%x %x %x", mdec.x, mdec.y, mdec.btn);
					boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 32, 16, 32 + 8 * 8 - 1, 31);
					putfont8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);

					/* マウスカーソルの移動 */
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15); /* マウス消す */
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
					putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16); /* マウス描く */
				}
			}
		}
	}
}

unsigned int memtest(unsigned int start, unsigned int end)
{
	char flag486 = 0; // 486アーキテクチャかどうか。486の場合はキャッシュ機構があるのでOFFにする
	unsigned int eflag, cr0, i;

	eflag = _io_load_eflags();
	eflag |= EFLAGS_AC_BIT; // ACビットを1にする
	_io_store_eflags(eflag);
	eflag = _io_load_eflags();
	// 386ではACフラグを1にしても0に戻るのでそこで判別
	if ((eflag & EFLAGS_AC_BIT) != 0)
	{
		flag486 = 1;
	}
	eflag &= ~EFLAGS_AC_BIT; // ACビットを0に戻す
	_io_store_eflags(eflag);

	// 486以降ならキャッシュ無効か
	if (flag486 == 1)
	{
		cr0 = _load_cr0();
		cr0 |= CR0_CACHE_DISABLE;
		_store_cr0(cr0);
	}

	i = memtest_sub(start, end);

	// キャッシュの無効化を戻す
	if (flag486 == 1)
	{
		cr0 = _load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE;
		_store_cr0(cr0);
	}

	return i;
}

unsigned int memtest_sub(unsigned int start, unsigned int end)
{
	unsigned int i, *p, old, pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;
	for (i = start; i <= end; i += 0x1000)
	{
		p = (unsigned int *)(i + 0xffc);
		old = *p;
		*p = pat0;
		*p ^= 0xffffffff;
		if (*p != pat1)
		{
		not_memory:
			*p = old;
			break;
		}
		*p ^= 0xffffffff;
		if (*p != pat0)
		{
			goto not_memory;
		}
		*p = old;
	}
	return i;
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

void memman_init(struct MEMMAN *man)
{
	man->frees = 0;	   /* あき情報の個数 */
	man->maxfrees = 0; /* 状況観察用：freesの最大値 */
	man->lostsize = 0; /* 解放に失敗した合計サイズ */
	man->losts = 0;	   /* 解放に失敗した回数 */
	return;
}

unsigned int memman_total(struct MEMMAN *man)
/* あきサイズの合計を報告 */
{
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++)
	{
		t += man->free[i].size;
	}
	return t;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
/* 確保 */
{
	unsigned int i, a;
	for (i = 0; i < man->frees; i++)
	{
		if (man->free[i].size >= size)
		{
			/* 十分な広さのあきを発見 */
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0)
			{
				/* free[i]がなくなったので前へつめる */
				man->frees--;
				for (; i < man->frees; i++)
				{
					man->free[i] = man->free[i + 1]; /* 構造体の代入 */
				}
			}
			return a;
		}
	}
	return 0; /* あきがない */
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
/* 解放 */
{
	int i, j;
	/* まとめやすさを考えると、free[]がaddr順に並んでいるほうがいい */
	/* だからまず、どこに入れるべきかを決める */
	for (i = 0; i < man->frees; i++)
	{
		if (man->free[i].addr > addr)
		{
			break;
		}
	}
	/* free[i - 1].addr < addr < free[i].addr */
	if (i > 0)
	{
		/* 前がある */
		if (man->free[i - 1].addr + man->free[i - 1].size == addr)
		{
			/* 前のあき領域にまとめられる */
			man->free[i - 1].size += size;
			if (i < man->frees)
			{
				/* 後ろもある */
				if (addr + size == man->free[i].addr)
				{
					/* なんと後ろともまとめられる */
					man->free[i - 1].size += man->free[i].size;
					/* man->free[i]の削除 */
					/* free[i]がなくなったので前へつめる */
					man->frees--;
					for (; i < man->frees; i++)
					{
						man->free[i] = man->free[i + 1]; /* 構造体の代入 */
					}
				}
			}
			return 0; /* 成功終了 */
		}
	}
	/* 前とはまとめられなかった */
	if (i < man->frees)
	{
		/* 後ろがある */
		if (addr + size == man->free[i].addr)
		{
			/* 後ろとはまとめられる */
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; /* 成功終了 */
		}
	}
	/* 前にも後ろにもまとめられない */
	if (man->frees < MEMMAN_FREES)
	{
		/* free[i]より後ろを、後ろへずらして、すきまを作る */
		for (j = man->frees; j > i; j--)
		{
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees)
		{
			man->maxfrees = man->frees; /* 最大値を更新 */
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0; /* 成功終了 */
	}
	/* 後ろにずらせなかった */
	man->losts++;
	man->lostsize += size;
	return -1; /* 失敗終了 */
}
