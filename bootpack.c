/* 头文件 */

#include <stdio.h>
#include "bootpack.h"

/* 代码主体 */

extern struct FIFO8 keyfifo;

void HariMain(void){
	struct BOOTINFO *binfo=(struct BOOTINFO *)ADR_BOOTINFO;
	int i;
	char s[40],mcursor[256],keybuf[32];

	init_gdtidt();
	init_pic();
	io_sti();//STI为CLI的逆指令，执行后IF（interrupt flag，中断许可标志位）变为1，接受来自外部设备的中断

	init_palette(); //设定调色板
	init_screen(binfo->vram,binfo->scrnx,binfo->scrny);
	//修改了PIC的IMR，以便接受来自键盘和鼠标的中断
	io_out8(PIC0_IMR,0xf9);//11111001
	io_out8(PIC1_IMR,0xef);//11101111

	fifo8_init(&keyfifo,32,keybuf);//初始化给键盘使用的缓冲区

	for(;;){
		io_cli();
		if(fifo8_status(&keyfifo)==0){
			io_stihlt();
		}else{
			i=fifo8_get(&keyfifo);
			io_sti();
			sprintf(s,"%02X",i);
			boxfill8(binfo->vram,binfo->scrnx,COL8_008484,8,8,binfo->scrnx-8,24);
			putfont8_asc_shadow(binfo->vram,binfo->scrnx,8,8,COL8_FFFFFF,s);
		}
	}
}
