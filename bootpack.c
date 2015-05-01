/* 头文件 */

#include <stdio.h>
#include "bootpack.h"

/* 代码主体 */

extern struct FIFO8 keyfifo,mousefifo;
extern struct MOUSE_DEC mdec;

char mcursor[16][16];

void HariMain(void){
	struct BOOTINFO *binfo=(struct BOOTINFO *)ADR_BOOTINFO;
	int i,mx=(binfo->scrnx-16)/2,my=(binfo->scrny-16)/2;
	char s[40],keybuf[32],mousebuf[128];

	init_gdtidt();
	init_pic();
	io_sti();//STI为CLI的逆指令，执行后IF（interrupt flag，中断许可标志位）变为1，接受来自外部设备的中断

	fifo8_init(&keyfifo,32,keybuf);//初始化给键盘使用的缓冲区
	fifo8_init(&mousefifo,128,mousebuf);//初始化给鼠标使用的缓冲区

	//修改了PIC的IMR，以便接受来自键盘和鼠标的中断
	io_out8(PIC0_IMR,0xf9);//11111001
	io_out8(PIC1_IMR,0xef);//11101111

	init_keyboard();

	init_palette(); //设定调色板
	init_mouse_cursor8(mcursor,COL8_008484);

	enable_mouse(&mdec);

	init_screen(binfo->vram,binfo->scrnx,binfo->scrny);

	for(;;){
		io_cli();
		if(fifo8_status(&keyfifo)+fifo8_status(&mousefifo)==0){
			io_stihlt();
		}else{
			if(fifo8_status(&keyfifo)!=0){
				i=fifo8_get(&keyfifo);
				io_sti();
				sprintf(s,"%02X",i);
				boxfill8(binfo->vram,binfo->scrnx,COL8_008484,8,24,binfo->scrnx-8,39);
				putfont8_asc_shadow(binfo->vram,binfo->scrnx,8,24,COL8_FFFFFF,s);
			}else if(fifo8_status(&mousefifo)!=0){
				i=fifo8_get(&mousefifo);
				io_sti();
				if(mouse_decode(&mdec,i)==1){
					//鼠标的三个字节都齐了，显示出来
					sprintf(s,"[lcr,%4d,%4d]",mdec.x,mdec.y);
					//如果鼠标按键被按下，则将相应的字母变为大写
					if((mdec.btn&0x01)!=0){
						s[1]='L';
					}
					if((mdec.btn&0x02)!=0){
						s[3]='R';
					}
					if((mdec.btn&0x04)!=0){
						s[2]='C';
					}
					boxfill8(binfo->vram,binfo->scrnx,COL8_008484,8,40,159,55);
					putfont8_asc_shadow(binfo->vram,binfo->scrnx,8,40,COL8_FFFFFF,s);
					//鼠标指针的移动
					boxfill8(binfo->vram,binfo->scrnx,COL8_008484,mx,my,mx+15,my+15);//隐藏上一帧的鼠标
					mx+=mdec.x;
					my+=mdec.y;
					if(mx<0){
						mx=0;
					}
					if(my<0){
						my=0;
					}
					if(mx>binfo->scrnx-16){
						mx=binfo->scrnx-16;
					}
					if(my>binfo->scrny-16){
						my=binfo->scrny-16;
					}
					sprintf(s,"(%3d, %3d)",mx,my);
					boxfill8(binfo->vram,binfo->scrnx,COL8_008484,160,40,312,55);
					putfont8_asc_shadow(binfo->vram,binfo->scrnx,160,40,COL8_FFFFFF,s);//显示鼠标坐标
					putblock8_8(binfo->vram,binfo->scrnx,16,16,mx,my,mcursor,16);//描绘鼠标
				}
			}
		}
	}
}
