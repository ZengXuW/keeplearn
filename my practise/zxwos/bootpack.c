/*
* @Filename: bootpack.c
* @Author: zxw
* @Date:   2020-01-10 19:21:10
* @Last Modified by:   zxw
* @Last Modified time: 2020-01-13 19:52:25
*/
#include "bootpack.h"					 
#include <stdio.h>

struct MOUSE_DEC {
	unsigned char buf[3], phase;
	int x, y, btn;
};

extern struct FIFO8 keyfifo, mousefifo;  /*设立缓冲区*/
void enable_mouse(struct MOUSE_DEC *mdec);
void init_keyboard(void);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data);

void HariMain(void)
{
	struct  BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40], mcursor[256], keybuf[32], mousebuf[128];
	int mx, my, i;
	struct MOUSE_DEC mdec;
	
	/* 
		关于CPU中断的补充知识：
			CPU的中断信号只有一根，所以中断标志位也只有一位
			而不像PIC一样有8位
	*/
	init_gdtidt();
	init_pic();
	io_sti(); /* 执行STI指令，将中断标志置为1 */
	fifo8_init(&keyfifo, 32, keybuf); /*&运算符在这里是取地址运算符*/
	fifo8_init(&mousefifo, 128, mousebuf);

	/* 设置中断屏蔽寄存器 */
	io_out8(PIC0_IMR, 0xf9); /* PIC1键盘中断许可（11111001） */
	io_out8(PIC1_IMR, 0xef); /* 鼠标中断许可（11101111） */

	init_keyboard();
	
	init_palette();
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
	mx = (binfo->scrnx - 16) / 2; /* 画面正中央的坐标计算 */
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	putfonts8_asc(binfo->vram, binfo->scrnx, 31, 50, COL8_FFFFFF, "ZXW FIRST OS.");
	
	enable_mouse(&mdec);

	for (;;) {
		io_cli(); // 首先屏蔽中断
		if(fifo8_status(&keyfifo) + + fifo8_status(&mousefifo) == 0) {
			io_stihlt(); // io_stihlt(): 执行STL指令，然后让CPU执行HLT
		} else {
			if (fifo8_status(&keyfifo) != 0)
			{
				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf(s, "%02X", i);
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
				putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
			} else if (fifo8_status(&mousefifo) != 0) {
				i = fifo8_get(&mousefifo);
				io_sti();
				if (mouse_decode(&mdec, i) != 0) {
					/*数据的三个字节都齐了，把他们显示出来*/
					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01) != 0) {
						s[1] = 'L';
					}
					if ((mdec.btn & 0x02) != 0) {
						s[3] = 'R';
					}
					if ((mdec.btn & 0x04) != 0) {
						s[2] = 'C';
					}
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
					putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
				}
			}
		}
	}
}

#define PORT_KEYDAT				0x0060
#define PORT_KEYSTA				0x0064
#define PORT_KEYCMD				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47

void wait_KBC_sendready(void) {
	/*等待键盘控制电路准备完毕*/
	for(;;) {
		if((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
			break;
		}
	}
	return;
}

void init_keyboard(void) {
	/*初始化键盘控制电路*/
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}

#define KEYCMD_SEND_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4

void enable_mouse(struct MOUSE_DEC *mdec) {
	/*鼠标有效*/
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SEND_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	/*顺利的话，键盘控制会返回ACK(0xfa)*/
	mdec->phase = 0; /*等待鼠标的0xfa*/
	return; 
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
	if (mdec->phase == 0) {
		/*等待鼠标的0xfa的阶段*/
		if (dat == 0xfa) {
			mdec->phase = 1;
		}
		return 0;
	}
	if (mdec->phase == 1) {
		/*等待第一个字节*/
		if ((dat & 0xc8) == 0x08) { 
			/*加上这个条件判断的原因是避免鼠标的偶尔失灵导致的数据丢失*/
			/*如果第一字节正确*/
			mdec->buf[0] = dat;
			mdec->phase = 2;
		}
		return 0;
	}
	if (mdec->phase == 2) {
		/*等待第二个字节*/
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	if (mdec->phase == 3) {
		/*等待第三个字节到达，并在到达后做出处理*/
		mdec->buf[2] = dat;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07;
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];
		if ((mdec->buf[0] & 0x10) != 0) {
			mdec->x |= 0xffffff00;
		}
		if ((mdec->buf[0]) & 0x20 != 0) {
			mdec->y |= 0xffffff00;
		}
		mdec->y = - mdec->y; /*鼠标的y方向与画面符号相反*/
		return 1;
	}

	return -1; /*如果出现鼠标异常*/
}
