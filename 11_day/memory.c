#include "bootpack.h"

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

unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
	unsigned int address;
	size = (size + 0xfff) & 0xfffff000; // 切り上げ
	address = memman_alloc(man, size);
	return address;
}

int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i;
	size = (size + 0xfff) & 0xfffff000;
	i = memman_free(man, addr, size);
	return i;
}