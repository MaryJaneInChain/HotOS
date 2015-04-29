//int.c：关于中断

/* 头文件 */

#include <stdio.h>
#include "bootpack.h"

/* 函数定义 */

struct FIFO8 keyfifo;
struct FIFO8 mousefifo;

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

void inthandler21(int *esp){
	//来自PS/2键盘的中断
	unsigned char data;
	io_out8(PIC0_OCW2,0x61);//通知PIC：IRQ-01已经受理完毕
	data=io_in8(PORT_KEYDAT);
	fifo8_put(&keyfifo,data);
	return;
}

void inthandler2c(int *esp){
	//来自PS/2鼠标的中断
	unsigned char data;
	io_out8(PIC1_OCW2,0x64);//通知PIC1：IRQ-12已经受理完毕
	io_out8(PIC0_OCW2,0x62);//通知PIC0：IRQ-02已经受理完毕
	data=io_in8(PORT_KEYDAT);
	fifo8_put(&mousefifo,data);
	return;
}

void inthandler27(int *esp){
	io_out8(PIC0_OCW2,0x67);
	return;
}

void wait_KBC_sendready(void){
	//等待键盘控制电路准备完毕
	for(;;){
		if((io_in8(PORT_KEYSTA)&KEYSTA_SEND_NOTREADY)==0){
			break;
		}
	}
	return;
}

void init_keyboard(void){
	//初始化键盘控制电路
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD,KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT,KBC_MODE);
	return;
}

void enable_mouse(void){
	//激活鼠标
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD,KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT,MOUSECMD_ENABLE);
	return;//顺利的话，键盘控制器会返送回ACK(0xfa)
}