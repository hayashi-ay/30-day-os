; haribote-os
; TAB=4

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
fin:
		HLT
		JMP		fin