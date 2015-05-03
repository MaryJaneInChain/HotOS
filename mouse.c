//mouse.c：关于鼠标

/* 头文件 */

#include <stdio.h>
#include "bootpack.h"

/* 结构体 */

extern struct FIFO8 mousefifo;
extern struct MOUSE_DEC mdec;

/* 函数定义 */

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

void inthandler2c(int *esp){
	//来自PS/2鼠标的中断
	unsigned char data;
	io_out8(PIC1_OCW2,0x64);//通知PIC1：IRQ-12已经受理完毕
	io_out8(PIC0_OCW2,0x62);//通知PIC0：IRQ-02已经受理完毕
	data=io_in8(PORT_KEYDAT);
	fifo8_put(&mousefifo,data);
	return;
}