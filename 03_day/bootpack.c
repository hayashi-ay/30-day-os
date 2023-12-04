void _io_hlt();

void HariMain(void)
{
	fin:
		_io_hlt(); // naskfunc.nasの_io_hltが実行される
		goto fin;
}
