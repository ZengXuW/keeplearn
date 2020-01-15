/*
* @Filename: bootpack.c
* @Author: zxw
* @Date:   2020-01-10 19:21:10
* @Last Modified by:   zxw
* @Last Modified time: 2020-01-15 17:50:01
*/
#include "bootpack.h"					 
#include <stdio.h>

#define MEMMAN_FREES		4090	/* 大约是32KB */
#define MEMMAN_ADDR			0x003c0000

struct FREEINFO {	/* 可用信息 */
	unsigned int addr, size;
};

struct MEMMAN {	
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);

void HariMain(void)
{
	struct  BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40], mcursor[256], keybuf[32], mousebuf[128];
	int mx, my, i;
	unsigned int memtotal;
	struct MOUSE_DEC mdec;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	
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
	enable_mouse(&mdec);
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000);	/* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	init_palette();
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
	mx = (binfo->scrnx - 16) / 2; /* 画面正中央的坐标计算 */
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf(s, "(%3d, %3d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	putfonts8_asc(binfo->vram, binfo->scrnx, 31, 50, COL8_FFFFFF, "ZXW FIRST OS.");
	
	sprintf(s, "memory %dMB-----free  :  %dKB",
			memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

	for (;;) {
		io_cli(); // 首先屏蔽中断
		if(fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
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
					/*鼠标指针的移动*/
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);/*隐藏鼠标*/
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					}
					if (mx > binfo->scrnx - 16) {
						/*如果超出屏幕边界*/
						mx = binfo->scrnx - 16;
					}
					if (my > binfo->scrny - 16) {
						my = binfo->scrny - 16;
					}
					sprintf(s, "(%3d, %3d)", mx, my);
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15); /*隐藏坐标*/
					putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s); /*显示坐标*/
					putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16); /*显示鼠标*/

				}
			}
		}
	}
}

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

unsigned int memtest(unsigned int start, unsigned int end)
{
	char flg486 = 0;		/*检查是否为486及以上的CPU*/
	unsigned int eflg, cr0, i;

	/*确认CPU是386还是486及以上的*/
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT; /*AC-bit = 1*/
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if ((eflg & EFLAGS_AC_BIT) != 0) { /*如果是386，就算设定AC的值为1，也会自动回到0*/
		flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT; /*AC-bit = 0. (~为取反操作)*/
	io_store_eflags(eflg);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE; /*禁止缓存*/
		store_cr0(cr0);
	}

	i = memtest_sub(start, end); /*内存检查的处理部分*/

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE; /*启用缓存*/
		store_cr0(cr0);
	}

	return i;
}

void memman_init(struct MEMMAN *man)
{
	man->frees = 0;			/*可用信息数目*/
	man->maxfrees = 0;		/*用于观察可用状况, frees的最大值*/
	man->lostsize = 0;		/*释放失败的内存大小总和*/
	man->losts = 0;			/*释放失败次数*/
	return;
}

unsigned int memman_total(struct MEMMAN *man)
/* 报告空域内存的合计 */
{
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; ++i)
	{
		t += man->free[i].size;
	}
	return t;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
/* 分配 */
{
	unsigned int i, a;
	for (i = 0; i < man->frees; ++i)
	{
		if (man->free[i].size >= size) {
			/* 找到足够大的内存 */
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;		/*进行内存分配*/
			if (man->free[i].size == 0) {
				/* 如果free[i]变成了0，就减掉一条可用信息 */
				man->frees--;
				for (; i < man->frees; ++i)
				{
					man->free[i] = man->free[i + 1];	/* 代入结构体 */
				}
			}
			return a;
		}
	}
	return 0;	/* 没有可用空间 */
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
/* 释放 */
{
	int i, j;
	/* 为了便于归纳内存，将free[]按照addr的顺序排列 */
	/* 所以，先决定放在哪里 */
	for (i = 0; i < man->frees; ++i)
	{
		if (man->free[i].addr > addr) {
			break;
		}
	}
	/* free[i - 1].addr < addr < free[i].addr */
	if (i > 0) {
		/* 前面有可用内存 */
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			/* 可以与前面的内存归纳在一起 */
			man->free[i - 1].size += size;
			if (i < man->frees) {
				/* 后面也有可用内存 */
				if (addr + size == man->free[i].addr) {
					/* 与后面的可用内存归纳到一起 */
					man->free[i - 1].size += man->free[i].size;
					/* man->free[i] 删除 */
					/* free[i]变成0之后归纳到前面去 */
					man->frees--;
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i + 1]; /* 结构体赋值 */
					}
				}
			}
			return 0;
		}
	}
	/* 不能与前面的空间归纳到一起 */
	if (i < man->frees) {
		/* 后面还有可用内存 */
		if (addr + size == man->free[i].addr) {
			/* 可以与后面的可用内存归纳到一起 */
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; /* 成功 */
		}
	}
	/* 既不能与前面的空间归纳到一起，也不能与后面的空间归纳到一起 */
	if (man->frees < MEMMAN_FREES) {
		/* free[i]之后的，往后移动，腾出一点空间 */
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees;		/* 更新最大值 */
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0;	/* 成功 */
	}
	/* 不能往后移动 */
	man->losts++;
	man->lostsize += size;
	return -1;	/* 失败 */
}









