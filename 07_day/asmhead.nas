; haribote-os
; TAB=4

BOTPAK	EQU	0x00280000 ; bootpackのロード先
DSKCAC	EQU 0x00100000 ; ディスクキャッシュの場所
DSKCAC0	EQU	0x00008000 ; ディスクキャッシュの場所（リアルモード）

; BOOT_INFO関係
CYLS 	EQU	0x0ff0
LEDS	EQU	0x0ff1
VMODE	EQU 0x0ff2
SCRNX	EQU	0xff4 ; 解像度のX
SCRNY	EQU	0xff6 ; 解像度のY
VRAM	EQU	0xff8 ; グラフィックバッファの開始番地

		ORG 0xc200

		MOV	AL,0x13
		MOV	AH,0x00
		INT	0x10
		MOV	BYTE [VMODE],8
		MOV	WORD [SCRNX],320
		MOV	WORD [SCRNY], 200
		MOV	DWORD [VRAM], 0x000a0000

; キーボードのLED状態をBIOSに教えてもらう
		MOV	AH,0x02
		INT	0x16 ; keyboard BIOS
		MOV	[LEDS],AL

; PICが一切の割り込みを受け付けないようにする
		MOV	AL,0xff
		OUT	0x21,AL
		NOP ; OUT命令が連続するとうまくいかない機種があるらしい
		OUT	0xa1,AL

		CLI	; CPUレベルでも割り込み禁止

; CPUから1MB以上のメモリにアクセスできるようにA20GATEを設定

		CALL waitkbdout
		MOV	AL,0xd1
		OUT	0x64,AL
		CALL waitkbdout
		MOV	AL,0xdf
		OUT	0x60,AL
		CALL waitkbdout

; プロテクトモード移行

;nasmだとerror: unrecognized directive [INSTRSET]というエラーが出るのでコメントアウト
;[INSTRSET "i486p"] ; 486の命令まで使いたいという記述

		LGDT [GDTR0] ; 暫定GDTを設定
		MOV	EAX,CR0
		AND	EAX,0x7fffffff ; bit31を0にする（ページング禁止）
		OR	EAX,0x00000001 ; bit0を1にする（プロテクトモード移行のため）
		MOV	CR0,EAX
		JMP	pipelineflush

pipelineflush:
		MOV	AX,1*8 ; 読み書き可能セグメント32bit
		MOV	DS,AX
		MOV ES,AX
		MOV	FS,AX
		MOV	GS,AX
		MOV	SS,AX

; bootpackの転送
		MOV	ESI,bootpack ; 転送元
		MOV	EDI,BOTPAK ; 転送先
		MOV	ECX,512*1024/4
		CALL memcpy

; ついでにディスクデータも本来の位置へ転送

; まずはブートセクタから
		MOV	ESI,0x7c00 ; 転送元
		MOV EDI,DSKCAC ; 転送先
		MOV	ECX,512/4
		CALL memcpy

; 残り全部

		MOV	ESI,DSKCAC0+512 ; 転送元
		MOV	EDI,DSKCAC+512 ; 転送先
		MOV	ECX,0
		MOV	CL,BYTE [CYLS]
		IMUL ECX,512*18*2/4 ; シリンダ数からバイト数/4に変換
		SUB	ECX,512/4 ; IPLの分だけ差し引く
		CALL memcpy

; asmheadでしなければいけないことは全部終わったので後はbootpackに任せる

; bootpackの起動
		MOV	EBX, BOTPAK
		MOV	ECX, [EBX+16]
		ADD	ECX,3
		SHR	ECX,2
		JZ	skip
		MOV	ESI,[EBX+20]
		ADD	ESI,EBX
		MOV	EDI,[EBX+12]
		CALL memcpy

skip:
	MOV	ESP,[EBX+12] ; スタック初期値
	JMP	DWORD 2*8:0x0000001b

waitkbdout:
	IN	AL,0x64
	AND	AL,0x02
	JNZ	waitkbdout
	RET

memcpy:
	MOV	EAX,[ESI]
	ADD	ESI,4
	MOV	[EDI],EAX
	ADD	EDI,4
	SUB	ECX,1
	JNZ	memcpy
	RET

	ALIGNB 16

GDT0:
	TIMES	8 DB 0x00
	DW	0xffff,0x0000,0x9200,0x00cf	; 読み書き可能セグメント32bit
	DW	0xffff,0x0000,0x9a28,0x0047	; 実行可能セグメント32bit（bootpack用）	

	DW	0

GDTR0:
	DW	8*3-1
	DD	GDT0

	ALIGNB 16

bootpack:
