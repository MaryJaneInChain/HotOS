//graphic.c：描绘图像

/* 头文件 */

#include <stdio.h>
#include "bootpack.h"

/* 常量 */

#define COL8_000000 0
#define COL8_FF0000 1
#define COL8_00FF00 2
#define COL8_FFFF00 3
#define COL8_0000FF 4
#define COL8_FF00FF 5
#define COL8_00FFFF 6
#define COL8_FFFFFF 7
#define COL8_C6C6C6 8
#define COL8_840000 9
#define COL8_008400 10
#define COL8_848400 11
#define COL8_000084 12
#define COL8_840084 13
#define COL8_008484 14
#define COL8_848484 15

/* 函数定义 */

extern char mcursor[16][16];

void init_palette(void){
	static unsigned char table_rgb[16*3]={
		0x00,0x00,0x00, //0:黑
		0xff,0x00,0x00, //1:亮红
		0x00,0xff,0x00, //2:亮绿
		0xff,0xff,0x00, //3:亮黄
		0x00,0x00,0xff, //4:亮蓝
		0xff,0x00,0xff, //5:亮紫
		0x00,0xff,0xff, //6:浅亮蓝
		0xff,0xff,0xff, //7:白
		0xc6,0xc6,0xc6, //8:亮灰
		0x84,0x00,0x00, //9:暗红
		0x00,0x84,0x00, //10:暗绿
		0x84,0x84,0x00, //11:暗黄
		0x00,0x00,0x84, //12:暗青
		0x84,0x00,0x84, //13:暗紫
		0x00,0x84,0x84, //14:浅暗蓝
		0x84,0x84,0x84  //15:暗灰
	};
	set_palette(0,15,table_rgb);
	return;
}

void set_palette(int start,int end,unsigned char *rgb){
	int i,eflags;
	eflags=io_load_eflags(); //记录中断许可标志的值
	io_cli(); //将中断许可标志置为0，禁止中断
	io_out8(0x03c8,start);
	for(i=start;i<=end;i++){
		io_out8(0x03c9,rgb[0]/4);
		io_out8(0x03c9,rgb[1]/4);
		io_out8(0x03c9,rgb[2]/4);
		rgb+=3;
	}
	io_store_eflags(eflags); //复原中断许可标志
	return;
}

void boxfill8(unsigned char *vram,int xsize,unsigned char c,int x0,int y0,int x1,int y1){
	int x,y;
	for(y=y0;y<=y1;y++){
		for(x=x0;x<=x1;x++)vram[y*xsize+x]=c;
	}
	return;
}

void init_screen(unsigned char *vram,int xsize,int ysize){
	char s[40];

	sprintf(s,"scrnX = %d  scrnY = %d",xsize,ysize);

	boxfill8(vram,xsize,COL8_008484,0,0,xsize-1,ysize-14);
	boxfill8(vram,xsize,COL8_C6C6C6,0,ysize-13,xsize-1,ysize-13);
	boxfill8(vram,xsize,COL8_FFFFFF,0,ysize-12,xsize-1,ysize-12);
	boxfill8(vram,xsize,COL8_C6C6C6,0,ysize-11,xsize-1,ysize-1);

	putfont8_asc_shadow(vram,xsize,8,8,COL8_FFFFFF,"Heart of Truth OS");
	putfont8_asc_shadow(vram,xsize,8,56,COL8_FFFFFF,s);

	init_mouse_cursor8(mcursor,COL8_008484);

	putblock8_8(vram,xsize,16,16,152,92,mcursor,16);

	return;
}

void putfont8(unsigned char *vram,int xsize,int x,int y,char color,char *font){ //在屏幕上输出单个文字
	int i;
	char *p,d;
	for(i=0;i<16;i++){
		p=vram+(y+i)*xsize+x;
		d=font[i];
		if((d&0x80)!=0)p[0]=color;
		if((d&0x40)!=0)p[1]=color;
		if((d&0x20)!=0)p[2]=color;
		if((d&0x10)!=0)p[3]=color;
		if((d&0x08)!=0)p[4]=color;
		if((d&0x04)!=0)p[5]=color;
		if((d&0x02)!=0)p[6]=color;
		if((d&0x01)!=0)p[7]=color;
	}
	return;
}

void putfont8_asc(unsigned char *vram,int xsize,int x,int y,char color,unsigned char *string){ //在屏幕上输出一个字符串
	extern char hankaku[4096];
	for(;*string!=0x00;string++){
		putfont8(vram,xsize,x,y,color,hankaku+*string*16);
		x+=8;
	}
	return;
}

void putfont8_asc_shadow(unsigned char *vram,int xsize,int x,int y,char color,unsigned char *string){ //在屏幕上输出一个带阴影的字符串
	putfont8_asc(vram,xsize,x+1,y+1,COL8_000000,string);
	putfont8_asc(vram,xsize,x,y,color,string);
}

void init_mouse_cursor8(char *mouse,char bc){ //鼠标指针
	static char cursor[16][16]={
		"*...............",
		"**..............",
		"*0*.............",
		"*00*............",
		"*000*...........",
		"*0000*..........",
		"*00000*.........",
		"*000000*........",
		"*0000000*.......",
		"*00000000*......",
		"*0000******.....",
		"*000*...........",
		"*00*............",
		"*0*.............",
		"**..............",
		"*..............."
	};

	int x,y;
	for(y=0;y<16;y++){
		for(x=0;x<16;x++){
			if(cursor[y][x]=='*'){
				mouse[y*16+x]=COL8_000000;
			}else if(cursor[y][x]=='0'){
				mouse[y*16+x]=COL8_FFFFFF;
			}else{
				mouse[y*16+x]=bc;
			}
		}
	}
	return;
}

void putblock8_8(char *vram,int vxsize,int pxsize,int pysize,int px0,int py0,char *buf,int bxsize){
	int x,y;
	for(y=0;y<pysize;y++){
		for(x=0;x<pxsize;x++){
			vram[(py0+y)*vxsize+(px0+x)]=buf[y*bxsize+x];
		}
	}
	return;
}