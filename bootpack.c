/* 头文件 */

#include <stdio.h>
#include "bootpack.h"

/* 代码主体 */

extern struct FIFO8 keyfifo,mousefifo;
extern struct MOUSE_DEC mdec;

struct MEMMAN *memman;
unsigned int memtotal;
char mcursor[16][16];

void HariMain(void){
	struct BOOTINFO *binfo=(struct BOOTINFO *)ADR_BOOTINFO;
	int i,mx=(binfo->scrnx-16)/2,my=(binfo->scrny-16)/2;
	char s[40],keybuf[32],mousebuf[128];

	init_gdtidt();
	init_pic();
	io_sti();//STI为CLI的逆指令，执行后IF（interrupt flag，中断许可标志位）变为1，接受来自外部设备的中断

	memman=(struct MEMMAN *)MEMMAN_ADDR;
	memtotal=memtest(0x00400000,0xbfffffff);
	memman_init(memman);
	memman_free(memman,0x00001000,0x0009e000);//0x00001000 ~ 0x0009efff
	memman_free(memman,0x00400000,memtotal-0x00400000);

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
//检查内存容量
unsigned int memtest(unsigned int start,unsigned int end){
	char flg486=0;
	unsigned int eflg,cr0,i;
	//确认CPU是386还是486以上的
	eflg=io_load_eflags();
	eflg|=EFLAGS_AC_BIT;//AC-bit=1
	io_store_eflags(eflg);
	eflg=io_load_eflags();
	if((eflg&EFLAGS_AC_BIT)!=0){
		//如果是386，即使设定AC=1，AC的值还会自动回到0
		flg486=1;
	}
	eflg&=~EFLAGS_AC_BIT;//AC-bit=0
	io_store_eflags(eflg);

	if(flg486!=0){
		cr0=load_cr0();
		cr0|=CR0_CACHE_DISABLE;//禁止缓存
		store_cr0(cr0);
	}

	i=memtest_sub(start,end);

	if(flg486!=0){
		cr0=load_cr0();
		cr0&=~CR0_CACHE_DISABLE;//允许缓存
		store_cr0(cr0);
	}

	return i;
}

void memman_init(struct MEMMAN *man){
	man->frees=0;//可用信息数目
	man->maxfrees=0;//用于观察可用状况：frees的最大值
	man->lostsize=0;//释放失败的内存和大小总和
	man->losts=0;//释放失败次数
	return;
}
//报告空余内存大小的总和
unsigned int memman_total(struct MEMMAN *man){
	unsigned int i,t=0;
	for(i=0;i<man->frees;i++){
		t+=man->free[i].size;
	}
	return t;
}
//内存分配
unsigned int memman_alloc(struct MEMMAN *man,unsigned int size){
	unsigned int i,a;
	for(i=0;i<man->frees;i++){
		if(man->free[i].size>=size){
			//找到了足够大的内存
			a=man->free[i].addr;
			man->free[i].addr+=size;
			man->free[i].size-=size;
			if(man->free[i].size==0){
				//如果free[i]变成了0，就减掉一条可用信息
				man->frees--;
				for(;i<man->frees;i++){
					man->free[i]=man->free[i+1];//代入结构体
				}
			}
			return a;
		}
	}
	return 0;//没有可用空间
}
//内存释放，往memman里追加可用内存信息
int memman_free(struct MEMMAN *man,unsigned int addr,unsigned int size){
	int i,j;
	//为便于归纳内存，将free[]按照addr的顺序排列
	for(i=0;i<man->frees;i++){
		if(man->free[i].addr>addr){
			break;
		}
	}
	//free[i-1].addr < addr < free[i].addr
	if(i>0){
		//如果前面有可用内存
		if(man->free[i-1].addr+man->free[i-1].size==addr){
			//可以与前面的可用内存归纳到一起
			man->free[i-1].size+=size;
			if(i<man->frees){
				//如果后面也有可用内存
				if(addr+size==man->free[i].addr){
					//也可以与后面的可用的内存归纳到一起
					man->free[i-1].size+=man->free[i].size;
					//man->free[i]删除，变成0后归纳到前面去
					man->frees--;
					for(;i<man->frees;i++){
						man->free[i]=man->free[i+1];//结构体赋值
					}
				}
			}
			return 0;//成功完成
		}
	}
	//不能与前面的可用空间归纳到一起
	if(i<man->frees){
		//如果后面有可用内存
		if(addr+size==man->free[i].addr){
			//可以与后面的内容归纳到一起
			man->free[i].addr=addr;
			man->free[i].size+=size;
			return 0;//成功完成
		}
	}
	//既不能与前面归纳到一起，也不能与后面归纳到一起
	if(man->frees<MEMMAN_FREES){
		//free[i]之后的，向后移动，腾出一点可用空间
		for(j=man->frees;j>i;j--){
			man->free[j]=man->free[j-1];
		}
		man->frees++;
		if(man->maxfrees>man->frees){
			man->maxfrees=man->frees;//更新最大值
		}
		man->free[i].addr=addr;
		man->free[i].size=size;
		return 0;//成功完成
	}
	//不能往后移动
	man->losts++;
	man->lostsize+=size;
	return -1;//失败
}