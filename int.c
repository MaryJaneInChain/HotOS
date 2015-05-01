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

void enable_mouse(struct MOUSE_DEC *mdec){
	//激活鼠标
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD,KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT,MOUSECMD_ENABLE);
	//顺利的话，键盘控制器会返送回ACK(0xfa)
	mdec->phase=0;//设置为等待0xfa的阶段
	return;
}

int mouse_decode(struct MOUSE_DEC *mdec,unsigned char dat){
	if(mdec->phase==0){
		//等待鼠标发送0xfa信号的阶段
		if(dat==0xfa){
			mdec->phase=1;//设置为等待鼠标第一个信号的阶段
		}
		return 0;
	}
	if(mdec->phase==1){
		//等待鼠标发送来的第一个信号的阶段
		if((dat&0xc8)==0x08){
			//如果第一字节正确
			mdec->buf[0]=dat;
			mdec->phase=2;
		}
		return 0;
	}
	if(mdec->phase==2){
		//等待鼠标发送来的第二个信号的阶段
		mdec->buf[1]=dat;
		mdec->phase=3;
		return 0;
	}
	if(mdec->phase==3){
		//等待鼠标发送来的第三个信号的阶段
		mdec->buf[2]=dat;
		mdec->phase=1;

		mdec->btn=mdec->buf[0]&0x07;
		mdec->x=mdec->buf[1];
		mdec->y=mdec->buf[2];

		if((mdec->buf[0]&0x10)!=0){
			mdec->x|=0xffffff00;
		}
		if((mdec->buf[0]&0x20)!=0){
			mdec->y|=0xffffff00;
		}

		mdec->y=-mdec->y;//鼠标的y方向与画面符号相反
		return 1;
	}
	return -1;//正常情况下，程序走不到这里来
}