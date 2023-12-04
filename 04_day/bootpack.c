void _io_hlt();

void HariMain(void)
{
	int i;
	char *p;

	p = (char *) 0xa0000;

	for (i = 0; i <= 0xffff; i++) {
		p[i] = i & 0x0f;
	}

	for (;;) {
		_io_hlt(); // naskfunc.nasの_io_hltが実行される
	}
}
