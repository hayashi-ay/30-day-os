/* 割り込み関係 */

#include "bootpack.h"
#include "myfunc.h"

void init_pic(void)
/* PICの初期化 */
{
	_io_out8(PIC0_IMR,  0xff  ); /* 全ての割り込みを受け付けない */
	_io_out8(PIC1_IMR,  0xff  ); /* 全ての割り込みを受け付けない */

	_io_out8(PIC0_ICW1, 0x11  ); /* エッジトリガモード */
	_io_out8(PIC0_ICW2, 0x20  ); /* IRQ0-7は、INT20-27で受ける */
	_io_out8(PIC0_ICW3, 1 << 2); /* PIC1はIRQ2にて接続 */
	_io_out8(PIC0_ICW4, 0x01  ); /* ノンバッファモード */

	_io_out8(PIC1_ICW1, 0x11  ); /* エッジトリガモード */
	_io_out8(PIC1_ICW2, 0x28  ); /* IRQ8-15は、INT28-2fで受ける */
	_io_out8(PIC1_ICW3, 2     ); /* PIC1はIRQ2にて接続 */
	_io_out8(PIC1_ICW4, 0x01  ); /* ノンバッファモード */

	_io_out8(PIC0_IMR,  0xfb  ); /* 11111011 PIC1以外は全て禁止 */
	_io_out8(PIC1_IMR,  0xff  ); /* 11111111 全ての割り込みを受け付けない */

	return;
}

#define PORT_KEYDAT		0x0060

struct FIFO8 keyinfo;
struct FIFO8 mouseinfo;

void _inthandler21(int *esp)
/* PS/2キーボードからの割り込み */
{
	unsigned char data;

	_io_out8(PIC0_OCW2, 0x61); // IRQ-1受付完了をPICに通知
	data = _io_in8(PORT_KEYDAT);
	fifo8_put(&keyinfo, data);
	return;
}

void _inthandler2c(int *esp)
/* PS/2マウスからの割り込み */
{
	unsigned char data;

	_io_out8(PIC1_OCW2, 0x64); // IRQ-12受付完了をPIC1（スレーブ）に通知
	_io_out8(PIC0_OCW2, 0x62); // IRQ-02受付完了をPIC0（マスタ）に通知
	data = _io_in8(PORT_KEYDAT);
	fifo8_put(&mouseinfo, data);
	return;
}

void _inthandler27(int *esp)
/* PIC0からの不完全割り込み対策 */
/* Athlon64X2機などではチップセットの都合によりPICの初期化時にこの割り込みが1度だけおこる */
/* この割り込み処理関数は、その割り込みに対して何もしないでやり過ごす */
/* なぜ何もしなくていいの？
	→  この割り込みはPIC初期化時の電気的なノイズによって発生したものなので、
		まじめに何か処理してやる必要がない。									*/
{
	_io_out8(PIC0_OCW2, 0x67); /* IRQ-07受付完了をPICに通知(7-1参照) */
	return;
}
