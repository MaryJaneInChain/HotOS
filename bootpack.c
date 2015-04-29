/* 头文件 */

#include <stdio.h>
#include "bootpack.h"

/* 代码主体 */

extern struct KEYBUF keybuf;

void HariMain(void){
	struct BOOTINFO *binfo=(struct BOOTINFO *)ADR_BOOTINFO;
	int i;
	char s[40];

	init_gdtidt();
	init_pic();
	io_sti();//STI为CLI的逆指令，执行后IF（interrupt flag，中断许可标志位）变为1，接受来自外部设备的中断

	init_palette(); //设定调色板
	init_screen(binfo->vram,binfo->scrnx,binfo->scrny);
	//修改了PIC的IMR，以便接受来自键盘和鼠标的中断
	io_out8(PIC0_IMR,0xf9);//11111001
	io_out8(PIC1_IMR,0xef);//11101111

	for(;;){
		io_cli();
		if(keybuf.flag==0){
			io_stihlt();
		}else{
			i=keybuf.data;
			keybuf.flag=0;
			io_sti();
			sprintf(s,"%02X",i);
			boxfill8(binfo->vram,binfo->scrnx,COL8_008484,8,8,binfo->scrnx-8,24);
			putfont8_asc_shadow(binfo->vram,binfo->scrnx,8,8,COL8_FFFFFF,s);
		}
	}
}
