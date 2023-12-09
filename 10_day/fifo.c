#include "bootpack.h"

#define FLAGS_OVERRUN 0x0001

void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf)
{
	fifo->size = size;
	fifo->buf = buf;
	fifo->free = size;
	fifo->flags = 0;
	fifo->rp = 0;
	fifo->wp = 0;
}

int fifo8_put(struct FIFO8 *fifo, unsigned char data)
{
	if (fifo->free == 0)
	{
		// 空きがなくてあふれた
		fifo->flags |= FLAGS_OVERRUN;
		return -1;
	}
	fifo->buf[fifo->rp] = data;
	fifo->rp = (fifo->rp + 1) % fifo->size;
	fifo->free--;
	return 0;
}

int fifo8_get(struct FIFO8 *fifo)
{
	int data;
	if (fifo->free == fifo->size)
	{
		// バッファがないときを-1を返す
		return -1;
	}
	data = fifo->buf[fifo->wp];
	fifo->wp = (fifo->wp + 1) % fifo->size;
	fifo->free++;
	return data;
}

int fifo8_status(struct FIFO8 *fifo)
{
	// バッファにあるデータのサイズを返す
	return fifo->size - fifo->free;
}
