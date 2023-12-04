; naskfunc
; TAB=4

; NASMではエラーが出るのでコメントアウト
;[FORMAT "WCOFF"] ; オブジェクトファイルを作るモード
[BITS 32] ; 32ビットモードの機械語を作らせる

; NASMではエラーが出るのでコメントアウト
;[FILE "naskfunc.nas]
	GLOBAL _io_hlt, _write_mem8 ; このプログラムに含まれる関数名

; 以下は実際の関数
[SECTION .text] ; オブジェクトファイルではこれを書いてからプログラムを書く
_io_hlt: ; void io_hlt(void);
	HLT
	RET

_write_mem8: ; void write_mem8(int addr, int data);
	MOV	ECX,[ESP+4] ; [ESP+4]にaddrが入っているのでそれをECSに読み込む
	MOV	AL,[ESP+8] ; [ESP+8]にdataが入っているのでそれをALに読み込む
	MOV [ECX],AL
	RET