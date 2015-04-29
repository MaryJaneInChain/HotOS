TOOLPATH = ./z_tools/
INCPATH  = ../z_tools/haribote/

MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
MAKEFONT = $(TOOLPATH)makefont.exe
RULEFILE = $(INCPATH)haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
COPY     = copy
DEL      = del

default:
	$(MAKE) img

asm:
	$(MAKE) ipl.bin

run:
	$(MAKE) img
	$(MAKE) hankaku.bin
	$(COPY) hotos.img ..\z_tools\qemu\fdimage0.bin
	$(MAKE) -C $(TOOLPATH)qemu


install:
	$(MAKE) img
	$(IMGTOL) w a:hotos.img

img:
	$(MAKE) hotos.img

clean:
	-$(DEL) *.bin
	-$(DEL) *.lst
	-$(DEL) *.gas
	-$(DEL) *.obj
	-$(DEL) bootpack.map
	-$(DEL) bootpack.hrb
	-$(DEL) bootpack.bim
	-$(DEL) bootpack.nas
	-$(DEL) hotos.sys

src_only:
	$(MAKE) clean
	-$(DEL) hotos.img

ipl.bin: ipl.nas Makefile
	$(NASK) ipl.nas ipl.bin

asmhead.bin: asmhead.nas Makefile
	$(NASK) asmhead.nas asmhead.bin

%.gas: %.c Makefile
	$(CC1) -o $*.gas $*.c

%.nas: %.gas Makefile
	$(GAS2NASK) $*.gas $*.nas

%.obj: %.nas Makefile
	$(NASK) $*.nas $*.obj

bootpack.bim: bootpack.obj naskfunc.obj graphic.obj dsctbl.obj int.obj fifo.obj hankaku.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map bootpack.obj graphic.obj dsctbl.obj int.obj fifo.obj naskfunc.obj hankaku.obj
# 3MB+64KB=3136KB

bootpack.hrb: bootpack.bim Makefile
	$(BIM2HRB) bootpack.bim bootpack.hrb 0

hotos.sys: asmhead.bin bootpack.hrb Makefile
	copy /B asmhead.bin+bootpack.hrb hotos.sys

hotos.img: ipl.bin hotos.sys Makefile
	$(EDIMG)	imgin:../z_tools/fdimg0at.tek \
				wbinimg src:ipl.bin len:512 \
				from:0 to:0 \
				copy from:hotos.sys to:@: \
				imgout:hotos.img

naskfunc.bin: naskfunc.nas Makefile
	$(NASK) naskfunc.nas naskfunc.bin

hankaku.bin: hankaku.txt Makefile
	$(MAKEFONT) hankaku.txt hankaku.bin

hankaku.obj: hankaku.bin Makefile
	$(BIN2OBJ) hankaku.bin hankaku.obj _hankaku