void _io_hlt();

void HariMain(void)
{
	int i;
	char *p;

	for (i = 0xa0000; i <= 0xaffff; i++) {
		p = (char *)i;
		*p = i & 0x0f;
	}

	for (;;) {
		_io_hlt(); // naskfunc.nasの_io_hltが実行される
	}
}
