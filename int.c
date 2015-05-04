//int.c：关于中断

/* 头文件 */

#include <stdio.h>
#include "bootpack.h"

/* 函数定义 */

struct FIFO8 keyfifo;
struct FIFO8 mousefifo;
struct MOUSE_DEC mdec;

void init_pic(void){
	//初始化PIC
	io_out8(PIC0_IMR,0xff);//禁止所有中断
	io_out8(PIC1_IMR,0xff);//禁止所有中断

	io_out8(PIC0_ICW1,0x11);//边沿触发模式(edge trigger mode)
	io_out8(PIC0_ICW2,0x20);//IRQ 0~7由 INT 20~27 接收
	io_out8(PIC0_ICW3,1<<2);//PIC1由IRQ2连接
	io_out8(PIC0_ICW4,0x01);//无缓冲区模式

	io_out8(PIC1_ICW1,0x11);//边沿触发模式(edge trigger mode)
	io_out8(PIC1_ICW2,0x28);//IRQ 8~15 由 INT 28~2f 接收
	io_out8(PIC1_ICW3,2);//PIC1由IRQ2连接
	io_out8(PIC1_ICW4,0x01);//无缓冲区模式

	io_out8(PIC0_IMR,0xfb);//11111011 PIC1以外全部禁止
	io_out8(PIC1_IMR,0xff);//11111111 禁止所有中断

	return;
}



void inthandler27(int *esp){
	io_out8(PIC0_OCW2,0x67);
	return;
}
