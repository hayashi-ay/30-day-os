; haribote-os
; TAB=4
CYLS	EQU	10
		ORG 0x7c00 ; 機械語がメモリのどこに読み込まれるかを指示

; FAT12フォーマットフロッピーディスクのための記述
		JMP		entry
		DB		0x90
		DB		"HARIBOTE"
		DW		512 ; 1セクタの大きさ。フロッピーディスクは512バイト。
		DB		1
		DW		1
		DB		2
		DW		224
		DW		2880
		DB		0xf0
		DW		9
		DW		18
		DW		2
		DD		0
		DD		2880
		DB		0,0,0x29
		DD		0xffffffff
		DB		"HARIBOTEOS "
		DB		"FAT12   "
		TIMES	18 DB 0x00

; プログラム本体
entry:
		MOV		AX,0 ; レジスタ初期化
		MOV		SS,AX
		MOV		SP,0x7c00
		MOV		DS,AX

; ディスクを読む
		MOV		AX,0x0820
		MOV		ES,AX
		MOV		CH,0 ; シリンダ0
		MOV		DH,0 ; ヘッド0
		MOV		CL,2 ; セクタ2

readloop:
		MOV		SI,0 ; 失敗回数を数えるレジスタ

retry:
		MOV		AH,0x02 ; ディスク読込み
		MOV		AL,1 ; 1セクタ
		MOV		BX,0
		MOV		DL,0x00 ; Aドライブ
		INT		0x13 ; BIOS呼び出し
		JNC		next
		ADD		SI,1
		CMP		SI,5
		JAE		error ; SI >= 5 だったらerrorへ
		MOV		AH,0x00
		MOV		DL,0x00
		INT		0x13 ; システムのリセット
		JMP		retry

next:
		MOV		AX,ES
		ADD		AX, 0x0020
		MOV		ES,AX ; ADD ES, 0x0020という命令がないのでこの書き方をする必要がある
		ADD		CL,1
		CMP		CL,18 ; CL <= 18だったらreadloopへ
		JBE		readloop
		MOV		CL,1
		ADD		DH,1
		CMP		DH,2
		JB		readloop ; DH < 2だったらreadloopへ
		MOV		DH,0
		ADD		CH,1
		CMP		CH,CYLS
		JB		readloop ; CH < SYLSだったらreadloopへ

		JMP		0xc200

fin:
		HLT
		JMP		fin

error:
		MOV		SI,msg

putloop:
		MOV		AL,[SI]
		ADD		SI,1
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e
		MOV		BX,15
		INT		0x10
		JMP		putloop

; メッセージ部分
msg:
		DB		0x0a, 0x0a
		DB		"load error"
		DB		0x0a
		DB		0
		; RESB	0x7dfe-$ ではnasmだとエラーになる
		; $ - $$ : 現在位置からこのセクションの開始位置までのサイズ(使用済み容量) 
		; 0x1fe : ブートセクタの終わりアドレス(510バイト)
		; 0x1fe - ($ - $$) : 残りの容量
		TIMES 	0x1fe-($-$$) DB 0x00
		DB		0x55, 0xaa ; ブートセクタは「55 AA」で終わっている必要がある