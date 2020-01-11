; haribote-ipl
; TAB=4
; AX -- 累加寄存器 -- AL,AH
; CX -- 计数寄存器 -- CL,CH
; DX -- 数据寄存器 -- DL,DH
; BX -- 基址寄存器 -- BL,BH
; SP -- 栈指针寄存器 
; BP -- 基址指针寄存器
; SI -- 源变址寄存器
; DI -- 目的变址寄存器
; ES -- 附加段寄存器（extra segment）
; CS -- 代码段寄存器（code segment）
; SS -- 栈段寄存器（stack segment）
; DS -- 数据段寄存器（data segment）
; FS -- 没有名称（segment part 2）
; GS -- 没有名称（segment part 3）
; IPL -- 启动程序加载器
CYLS	EQU		10				; 和C语言中的#define相同，用于定义常量 EQU(equal 等于)
								
		ORG		0x7c00			; 指明程序的装载位置

; 以下记叙用于标准FAT12格式的软盘的代码，该软盘共有两个磁头，每个磁头80个柱面，每个柱面18个扇面

		JMP		entry
		DB		0x90			; DB（define byte）：往文件中直接写入一个字节的指令
		DB		"ZXWFSTOS"		; 启动区的名称可以是任意的字符串（8字节）
		DW		512				; 每个扇区（sector）的大小（必须为512字节）
		DB		1				; 簇（cluster）的大小（必须为1个扇区）
		DW		1				; FAT的起始位置
		DB		2				; FAT的个数（必须为2）DB：（define word： 2字节）
		DW		224				; 根目录的大小（一般设成224项）
		DW		2880			; 该磁盘的大小（必须是2880扇区）
		DB		0xf0			; 磁盘的种类（必须为0xf0）
		DW		9				; FAT的长度（必须为9扇区）
		DW		18				; 1个磁道（track）有几个扇区（必须为18）
		DW		2				; 磁头数（必须为是2）
		DD		0				; 不使用分区，必须为是0 DD：（define double word： 4个字节）
		DD		2880			; 重写一次磁盘大小
		DB		0,0,0x29		; 意义不明，固定
		DD		0xffffffff		; （可能是）卷标号码
		DB		"HARIBOTEOS "	; 磁盘的名称（11字节）
		DB		"FAT12   "		; 磁盘格式名称（8字节）
		RESB	18				; 先空出18字节 RESB：预约18个字节，并在这18个字节中填充0x00

; 程序本体

entry:
		MOV		AX,0			; 初始化寄存器
		MOV		SS,AX			
		MOV		SP,0x7c00
		MOV		DS,AX

; 读磁盘

		MOV		AX,0x0820
		MOV		ES,AX
		MOV		CH,0			; 柱面0
		MOV		DH,0			; 磁头0
		MOV		CL,2			; 扇区2
readloop:
		MOV		SI,0			; 记录失败次数的寄存器
retry:
		MOV		AH,0x02			; AH=0x02 : 读盘
		MOV		AL,1			; 1个扇区
		MOV		BX,0
		MOV		DL,0x00			; A驱动器
		INT		0x13			; 调用磁盘BIOS
		JNC		next			; 没出错的话就跳转到fin(JNC: jump if not carry)
		ADD		SI,1
		CMP		SI,5			; 设置读取磁盘的最大重复次数
		JAE  	error			; SI >= 5时，跳转到error(JAE：jump if above or equal)
		MOV		AH,0x00
		MOV     DL,0x00			; A驱动器
		INT 	0x13			; 重置驱动器
		JMP		retry
next:
		MOV 	AX,ES			; 把内存地址后移到0x200
		ADD 	AX,0x0020		; 通过AX将ES加上0x0020,相当于将地址后移0x200，详见书p49
		MOV		ES,AX
		ADD		CL,1			; 往CL里加1(把扇区号加1)
		CMP		CL,18			; 比较CL与18
		JBE		readloop		; 如果CL <= 18 跳转至readloop(JBE: jump if below or equal)
		MOV 	CL,1			; 给CL赋值为1,开始读取C0-H1-S1扇区
		ADD		DH,1			; 因为C0-H1-S1扇区位于磁盘背面，所以将DH加一进行翻面
		CMP		DH,2			;
		JB		readloop		; JB：jump if below | DH如果比2小就跳转到readloop
								; 开始读取这个柱面的18个扇面
		MOV		DH,0			; 再次翻面，开始读取
		ADD		CH,1			; 一个柱面读取完成，柱面+1
		CMP		CH,CYLS			; 直到读取10个柱面
		JB		readloop		; 如果没有到10个柱面就继续读取

; 完成了所有指令之后开始执行haribote.sys
		MOV		[0x0ff0],CH		; 把磁盘装载内容的结束地址告诉haribote.sys
		JMP		0xc200			; 跳转到0xc200开始执行haribote.sys



error:
		MOV		SI,msg
putloop:
		MOV		AL,[SI]			; [SI]表示内存地址
		ADD		SI,1			; 给SI加1
		CMP		AL,0			; CMP： campare 比较
		JE		fin				; JE：如果比较结果相等，就跳转，反之不跳转
		MOV		AH,0x0e			; 显示一个文字
		MOV		BX,15			; 指定字符颜色
		INT		0x10			; 调用显卡BIOS
		JMP		putloop
fin:
		HLT						; 让CPU停止，等待指令
		JMP		fin				; 无线循环
msg:
		DB		0x0a, 0x0a		; 换行2次
		DB		"load error"
		DB		0x0a			; 换行
		DB		0

		RESB	0x7dfe-$		; 填写0x00直到0x7dfe

		DB		0x55, 0xaa
