/* GDT/IDT descriptor table	*/
//======================================================================
//
//		Filename: dsctbl.c   
//		Description: GDT/IDT descriptor table
//		Author: 曾旭王
//  	Mail: zengxuw@foxmail.com
//		Last Modify Time: 2020年01月02日 20:50:56 Run Gump
//		你可以任意转载和使用该代码,但请注明原作者姓名
//
//======================================================================
#include "bootpack.h"

void init_gdtidt(void)
{
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;	// gdt被赋值为0x00270000，就是说要将0x270000 - 0x27ffff设为GDT
	struct GATE_DESCRIPTOR    *idt = (struct GATE_DESCRIPTOR    *) ADR_IDT;	// 同理， IDT被设置为0x2f800 - 0x26ffff
	int i;

	/* GDT initialization */
	for (i = 0; i <= LIMIT_GDT / 8; i++) {
		set_segmdesc(gdt + i, 0, 0, 0); // C语言在进行指针的加法运算的时候，每加一，地址增加为8
										// 这样就完成了8192个段的设置，并将它们的上限（limit）和基址（base），访问权限都设为0
	}
	set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, AR_DATA32_RW);	// 对第一个段进行初始化，上限值为0xffffffff(4GB),地址为0，表示的是CPU所能管理的全部内存本身
	set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);  // 对第二个段进行初始化，大小为512KB，地址为0x280000，这段地址是为bootpack.hrb而准备的
	load_gdtr(LIMIT_GDT, ADR_GDT);	// 在C语言中，不能给GDTR赋值，所以要使用汇编

	/* IDT initialization */
	for (i = 0; i <= LIMIT_IDT / 8; i++) {
		set_gatedesc(idt + i, 0, 0, 0);
	}
	load_idtr(LIMIT_IDT, ADR_IDT);
	
	set_gatedesc(idt + 0x21, (int) asm_inthandler21, 2 * 8, AR_INTGATE32); // 这里的2*8表示asm_inthandler21 段号为2，乘以8是因为低3位有别的意义，这里的低3位必须是0
	set_gatedesc(idt + 0x27, (int) asm_inthandler27, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x2c, (int) asm_inthandler2c, 2 * 8, AR_INTGATE32);
	
	return;
}
/*	关于段的各个知识点 */
/* 
	段上限：它表示一个段有多少个字节，段上限最大为4GB，也就是一个32位的数值，
	如果直接放进去，这个数值本身就要占用4个字节，加上base字段的4个字节
	一共就需要8个字节，这就把整个结构体就占满了，这样一来，就没有地方保存段的
	管理信息，因此段的上限只能使用20位，这样一来，段上限最大也就只能指定要1MB
	为止，Inter规定在段的属性中加入了一个标志位，叫做Gbit。这个标志位是1的时候
	，limit的单位不解释为字节（Byte），而解释成页（page）。而一页等于4KB，这样
	4KB * 1MB = 4GB，另外值得注意的一个点是：
	这20位的段上限分别写到limit_low和limit_high里,看起来它们好像是总共有3个字节
	(limit_low为2个字节,limit_high为1个字节),即24位,但实际上段的属性会写入limit_high
	的高4位，所以最后段上限还是只有20位
	
	段属性: 段属性又称为“段的访问权属性”,在程序中用变量名access_right或ar来表示，因为
	12位段属性中的高4位放在limit_high的高4位里，所以程序里又有意把ar当作如下的16位
	构成来处理:
	xxxx0000xxxxxxxx
	其中ar的高4位被称为“扩展访问权”,这4位是由“GD00”构成的，其中G是Gbit的标志位,D是指段的模式
	(其中1是指32位模式，0是16位模式)这里出现的16位模式，主要用于运行80826的程序，
	不能用于调用BIOS，所以，除了运行80826程序之外，通常都使用D=1的模式
	对于段属性的低8位，有以下几种格式:
	0000 0000 (0x00)：	未使用的记录表(descriptor table)
	1001 0010 (0x92)：	系统专用，可读写，不可执行
	1001 1010 (0x9a):	系统专用，可执行，可读不可写
	1111 0010 (0xf3):	应用程序用，可读写的段，不可执行
	1111 1010 (0xfa):	应用程序用，可执行的段，可读不可写
*/
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar) // base为段基址(32位)(base中有low(2字节)和mid(1字节)与high(1字节)) base分为三段的原因是要与80286时代的计算机兼容
{
	if (limit > 0xfffff) {
		ar |= 0x8000; /* G_bit = 1 */
		limit /= 0x1000;
	}
	sd->limit_low    = limit & 0xffff;
	sd->base_low     = base & 0xffff;
	sd->base_mid     = (base >> 16) & 0xff; // >>右移位运算符， 00101100 >> 3 = 00000101
	sd->access_right = ar & 0xff;
	sd->limit_high   = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
	sd->base_high    = (base >> 24) & 0xff;
	return;
}

void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar)
{
	gd->offset_low   = offset & 0xffff;
	gd->selector     = selector;
	gd->dw_count     = (ar >> 8) & 0xff;
	gd->access_right = ar & 0xff;
	gd->offset_high  = (offset >> 16) & 0xffff;
	return;
}