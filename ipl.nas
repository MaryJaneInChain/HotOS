;HotOS
;TAB=4

	CYLS EQU 10				;定义常数CYLS，读入的柱面数
	ORG 0x7c00				;程序装载地址

;以下用于FAT12格式软盘
	JMP entry
	DB 0x90
	DB "HOTOSIPL"
	DW 512
	DB 1
	DW 1
	DB 2
	DW 224
	DW 2880
	DB 0xf0
	DW 9
	DW 18
	DW 2
	DD 0
	DD 2880
	DB 0,0,0x29
	DD 0xffffffff
	DB "HOT-OS     "
	DB "FAT12   "
	RESB 18

entry:						;初始化寄存器
	MOV AX,0
	MOV SS,AX
	MOV SP,0x7c00
	MOV DS,AX

	MOV AX,0x0820
	MOV ES,AX
	MOV CH,0				;柱面0
	MOV DH,0				;磁头0
	MOV CL,2				;扇区2

readloop:
	MOV SI,0				;寄存器，记录读盘失败次数

retry:
	MOV AH,0x02				;AH=0x02：读盘
	MOV AL,1				;1个扇区
	MOV BX,0
	MOV DL 0x00				;A驱动器
	INT 0x13				;调用磁盘BIOS
	JNC next

	ADD SI,1
	CMP SI,5
	JAE error
	MOV AH,0x00				;AH=0x02：重置
	MOV DL,0x00
	INT 0x13
	JMP retry

next:
	MOV AX,ES				;内存地址后移0x200
	ADD AX,0x20
	MOV ES,AX
	ADD CL,1
	CMP CL,18
	JBE readloop

	MOV CL,1
	ADD DH,1
	CMP DH,2
	JB readloop

	MOV DH,0
	ADD CH,1
	CMP CH,CYLS
	JB readloop

	MOV [0x0ff0],CH
	JMP 0xc200

fin:
	HLT						;CPU休眠
	JMP fin

error:
	MOV SI,msg

putloop:
	MOV AL,[SI]
	ADD SI,1
	CMP AL,0

	JE fin
	MOV AH,0x0e				;显示一个文字
	MOV BX,15				;指定字符颜色
	INT 0x10				;调用显卡BIOS函数：0x10
	JMP putloop

msg:
	DB 0x0a,0x0a
	DB "HotOS - Heart of Truth OS"
	DB 0x0a
	DB "Error"
	DB 0x0a
	DB 0

	RESB 0x7dfe-$			;用0x7dfe减去ORG（0x7c00）个0填满余下的区域

	DB 0x55,0xaa