/* 中断关系 */
//======================================================================
//
//		Filename: int.c   
//		Description: 中断关系设定
//		Author: 曾旭王
//  	Mail: zengxuw@foxmail.com
//		Last Modify Time: 2020年01月02日 20:50:56 Run Gump
//		你可以任意转载和使用该代码,但请注明原作者姓名
//
//======================================================================

#include "bootpack.h"
#include <stdio.h>

/* PIC的寄存器 */
/*
	PIC的所有寄存器都是8位的寄存器
	IMR是“imterrupt mask register” -- '中断屏蔽寄存器'
		IMR的8位分别对应8位IRQ信号，如果某一位的值是1，
		则该位所对应的IRQ信号被屏蔽，PIC就忽视该路信号
	ICW是“initial control word” -- '初始化控制化数据'
		ICW有4个,分别编号为1-4，共有4个字节的数据。
		ICW1和ICW4与PIC主板配线方式，中断信号的电气特性等有关
		
		ICW3是关于主-从连接的设定，对主PIC而言，
		第几号IRQ和从PIC相连，使用8位来设定的，如果把这些位
		全部设置为1，那么主PIC就能驱动8个从PIC（最大可能有64个IRQ）
		但我们所用的电脑并不是这样的，所以就设定成00000100。
		另外的，对于PIC来说，该从PIC与主PIC的第几号相连，用3位来设定
		但是从硬件上来说这个设定已经不可改动，所以软件只能配合现有的硬件设定
		
		ICW2决定了IRQ以哪一号中断通知CPU，具体解释见<30天自制操作系统> p119
*/

void init_pic(void)
/* PIC的初始化 */
{
	io_out8(PIC0_IMR, 0xff	); /* 禁止所有中断 */
	io_out8(PIC1_IMR, 0xff	); /* 禁止所有中断 */
	
	io_out8(PIC0_ICW1, 0x11	); /* 边沿触发模式（edge trigger mode） */
	io_out8(PIC0_ICW2, 0x20	); /* IRQ 0-7 由 INT 20-27 接收 */
	io_out8(PIC0_ICW3, 1 << 2); /* PIC1由IRQ2连接 */
	io_out8(PIC0_ICW4, 0x01  ); /* 无缓冲区模式 */
	
	io_out8(PIC1_ICW1, 0x11  ); /* 边沿触发模式（edge trigger mode） */
	io_out8(PIC1_ICW2, 0x28  ); /* iRQ由INT@*_2f接收 */
	io_out8(PIC1_ICW3, 2     ); /* PIC1由IRQ2连接 */
	io_out8(PIC1_ICW4, 0x01  ); /* 无缓冲区模式 */
	
	io_out8(PIC0_IMR,  0xfb  ); /* 11111011 PIC1以外全部禁止 */
	io_out8(PIC1_IMR,  0xff  ); /* 11111111 禁止所有中断 */
	
	return;
}
/* 
	对于一部分机种而言，随着PIC的初始化，会产生IRQ7中断，
	如果不针对该中断处理程序执行STI(设置中断标志位),操作系统的启动就会失败 
*/
void inthandler27(int *esp)
/* 防止来自PIC0的不完整中断的措施 */
/* 在Athlon64X2机器中，由于芯片组情况，此中断在PIC初始化时仅发生一次 */
/* 该中断处理功能对该中断不起作用 */
/* 
   你为什么不用做什么？
   →由于此中断是由PIC初始化期间的电气噪声产生的，
   您不必认真做任何事情。 
*/
{
	io_out8(PIC0_OCW2, 0x67); /* 通知PIC IRQ-07已被接受（请参阅7-1） */
	return;
}