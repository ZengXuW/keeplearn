/*
* @Filename: fifo.c
* @Author: zxw
* @Date:   2020-01-10 19:21:11
* @Last Modified by:   zxw
* @Last Modified time: 2020-01-10 23:38:15
*/


#include "bootpack.h"

#define FLAGS_OVERRUN		0x0001

/* 缓冲区总字节数为size， free为缓冲区中没有数据的字节数 */
/* p为下一个数据写入地址， q为下一个数据读入地址 */
/*缓冲区初始化函数*/
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf) {
	/* FIFO init */
	fifo->size = size;
	fifo->buf = buf;
	fifo->free = size; /* 初始全部为空 */
	fifo->p = 0; /* 写入位置 */
	fifo->q = 0; /* 读取位置 */
	fifo->q = 0; /* 读取位置 */
	return;
}

/*缓冲区写入函数*/
int fifo8_put(struct FIFO8 *fifo, unsigned char data) {
	/* 将数据发送到FIFO并存储 */
	if (fifo->free == 0) {
		// 空余没有了，溢出
		fifo->flags |= FLAGS_OVERRUN;
		return -1;
	}
	fifo->buf[fifo->p] = data;
	fifo->p++;
	if (fifo->p == fifo->size) {
		fifo->p = 0;
	}
	fifo->free--;
	return 0;
}

/*缓冲区读取函数*/
int fifo8_get(struct FIFO8 *fifo) {
	/*从缓冲区读取一个数据*/
	int data;
	if (fifo->free == fifo->size)
	{
		/*缓冲区中没有数据*/
		return -1;
	}
	data = fifo->buf[fifo->q];
	fifo->q++;
	if (fifo->q == fifo->size)
	{
		fifo->q = 0;
	}
	fifo->free++;
	return data;
}

/*报告一下到底积攒了多少缓冲数据*/
int fifo8_status(struct FIFO8 *fifo) {
	return fifo->size - fifo->free;
}