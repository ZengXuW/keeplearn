!cons_nt.bat	// 启动bat
asmhead.nas	// 汇编
ipl10.nas		// 启动前的初始化汇编
Makefile		// 文件生成路径与方法
naskfunc.nas	// 由汇编编写的函数，供C语言操作系统底层
graphic.c 		// 为关于显示描画的处理
dsctlb.c 		// 关于GDT，IDT等descriptor table的处理
bootpack.c	 	// 其他处理
mouse.c     	// 鼠标处理函数
keyboard.c   	// 键盘处理函数
memory.c 		// 内存处理函数
fifo.c 			// 缓冲区处理函数
bootpack.h  	// 各种函数的声明头文件
int.c 			// 中断处理函数


Some of us get dipped in flat, some in satin, some in gloss.

But every once in a while you find someone who's iridescent, and when you do,
nothing will ever compare.

#         ┌─┐       ┌─┐
#      ┌──┘ ┴───────┘ ┴──┐
#      │                 │
#      │       ───       │
#      │  ─┬┘       └┬─  │
#      │                 │
#      │       ─┴─       │
#      │                 │
#      └───┐         ┌───┘
#          │         │
#          │         │
#          │         │
#          │         └──────────────┐
#          │                        │
#          │                        ├─┐
#          │                        ┌─┘
#          │                        │
#          └─┐  ┐  ┌───────┬──┐  ┌──┘
#            │ ─┤ ─┤       │ ─┤ ─┤
#            └──┴──┘       └──┴──┘
#                神兽保佑
#                代码无BUG!