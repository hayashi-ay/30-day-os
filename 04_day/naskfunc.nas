; naskfunc
; TAB=4

; NASMではエラーが出るのでコメントアウト
;[FORMAT "WCOFF"] ; オブジェクトファイルを作るモード
[BITS 32] ; 32ビットモードの機械語を作らせる

; NASMではエラーが出るのでコメントアウト
;[FILE "naskfunc.nas]
	GLOBAL _io_hlt ; このプログラムに含まれる関数名

; 以下は実際の関数
[SECTION .text] ; オブジェクトファイルではこれを書いてからプログラムを書く
_io_hlt: ; void io_hlt(void);
	HLT
	RET
