TOOLCHAIN		?=	/home/tavisco/palm/palmdev_V3/buildtools/toolchain/bin
SDK				?=	/home/tavisco/palm/palmdev_V3/buildtools/palm-os-sdk-master/sdk-5r3/include
PILRC			?=	/home/tavisco/palm/palmdev_V3/buildtools/pilrc3_3_unofficial/bin/pilrc
ARMTOOLCHAIN	?=	/home/tavisco/palm/gcc-arm-10.3-2021.07-x86_64-arm-none-eabi/bin/arm-none-eabi-
CC				=	$(TOOLCHAIN)/m68k-none-elf-gcc
LD				=	$(TOOLCHAIN)/m68k-none-elf-gcc
OBJCOPY			=	$(TOOLCHAIN)/m68k-none-elf-objcopy
ARMCC			=	$(ARMTOOLCHAIN)gcc
ARMLD			=	$(ARMTOOLCHAIN)gcc
ARMOBJCOPY		=	$(ARMTOOLCHAIN)objcopy
COMMON			=	-Wmissing-prototypes -Wstrict-prototypes -Wall -Wextra -Werror
LTO				=	-flto
ARMLTO			=	-flto
ARMTYPE			=	-marm		#shoudl be -mthumb or -marm
M68KCOMMON		=	$(COMMON) -Wno-multichar -funsafe-math-optimizations -Os -m68000 -mno-align-int -mpcrel -fpic -fshort-enums -mshort -fvisibility=hidden -Wno-attributes
ARMCOMMON		=	$(COMMON) -Ofast -march=armv4t $(ARMTYPE) -mno-unaligned-access -ffixed-r9 -ffixed-r10 -ffixed-r11 -fomit-frame-pointer -D__ARM__ -ffreestanding -fpic -mthumb-interwork -Wno-attributes
WARN			=	-Wsign-compare -Wextra -Wall -Wno-unused-parameter -Wno-old-style-declaration -Wno-unused-function -Wno-unused-variable -Wno-error=cpp -Wno-switch  -Wno-implicit-fallthrough
LKR				=	Src/68k.lkr
ARMLKR			=	Src/arm.lkr
CCFLAGS			=	$(LTO) $(WARN) $(M68KCOMMON) -I. -ffunction-sections -fdata-sections
LDFLAGS			=	$(LTO) $(WARN) $(M68KCOMMON) -Wl,--gc-sections -Wl,-T $(LKR)
ARMCCFLAGS		=	$(ARMLTO) $(WARN) $(ARMCOMMON) -I. -ffunction-sections -fdata-sections
ARMLDFLAGS		=	$(ARMLTO) $(WARN) $(ARMCOMMON) -Wl,--gc-sections -Wl,-T $(ARMLKR)
SRCS-68k		=   Src/Palmkedex.c Src/Main.c Src/PkmnMain.c Src/PkmnType.c Src/pokeInfo.c Src/glue.c Src/helpers.c Src/osPatches.c Src/imgDraw.c Src/aciDecode.c Src/aciDecodeAsm68k.S Src/qrcode/qrcodegen.c Src/qrcode/qrcode.c
SRCS-arm		=	Src/helpers.c Src/imgDrawArmlet.c Src/armcalls.c Src/aciDecode.c Src/aciDecodeARM.S
RCP				=	Rsc/Palmkedex_Rsc.rcp
TARGET			=	Palmkedex
TARGETSPRITES	=	SpritePack
CREATOR			=	PKDX
TYPE			=	appl
SPRITETYPE		=	pSPR

#add PalmOS SDK
INCS			+=	-I"gccisms"
INCS			+=	-isystem "$(SDK)"
INCS			+=	-isystem "$(SDK)/Core"
INCS			+=	-isystem "$(SDK)/Core/Hardware"
INCS			+=	-isystem "$(SDK)/Core/System"
INCS			+=	-isystem "$(SDK)/Core/UI"
INCS			+=	-isystem "$(SDK)/Dynamic"
INCS			+=	-isystem "$(SDK)/Libraries"
INCS			+=	-isystem "$(SDK)/Libraries/PalmOSGlue"
INCS			+=	-isystem "$(SDK)/Extensions/ExpansionMgr"

#add Sony SDK
INCS			+=	-isystem "$(SDK)/SonySDK/R5.0/Incs"
INCS			+=	-isystem "$(SDK)/SonySDK/R5.0/Incs/System"
INCS			+=	-isystem "$(SDK)/SonySDK/R5.0/Incs/Libraries"

#add Handera SDK 1.05
INCS			+=	-I "$(SDK)/Handera/include"


#leave this alone
OBJS-68k		=	$(patsubst %.S,%.68k.o,$(patsubst %.c,%.68k.o,$(SRCS-68k)))
OBJS-arm		=	$(patsubst %.S,%.arm.o,$(patsubst %.c,%.arm.o,$(SRCS-arm)))
all: $(TARGET).prc $(TARGETSPRITES)-hres.prc $(TARGETSPRITES)-hres-grey.prc $(TARGETSPRITES)-mres.prc $(TARGETSPRITES)-mres-grey.prc $(TARGETSPRITES)-lres.prc $(TARGETSPRITES)-lres-grey.prc $(TARGETSPRITES)-2bpp.prc $(TARGETSPRITES)-1bpp.prc
HFILES			=	$(wildcard Src/*.h)


$(TARGET).prc: code0001.68k.bin armc0001.arm.bin $(RCP).real.rcp
	$(PILRC) -ro -o $(TARGET).prc -creator $(CREATOR) -type $(TYPE) -name $(TARGET) $(RCP).real.rcp

%.rcp.real.rcp: %.rcp Makefile $(HFILES)
	#PilRC's macro abilities are pitiful, so we call upon CPP :)
	cpp -o $@ $< -ISrc -I. -P

%.68k.bin: %.68k.elf
	$(OBJCOPY) -O binary $< $@ -j.vec -j.text -j.rodata

%.arm.bin: %.arm.elf
	$(ARMOBJCOPY) -O binary $< $@ -j.text -j.rodata

%.68k.elf: $(OBJS-68k)
	$(LD) -o $@ $(LDFLAGS) $^

%.arm.elf: $(OBJS-arm)
	$(ARMLD) -o $@ $(ARMLDFLAGS) $^

%.68k.o : %.c Makefile $(HFILES)
	$(CC) $(CCFLAGS)  $(INCS) -c $< -o $@

%.68k.o : %.S Makefile $(HFILES)
	$(CC) $(CCFLAGS)  $(INCS) -c $< -o $@

%.arm.o : %.c Makefile $(HFILES)
	$(ARMCC) $(ARMCCFLAGS) $(INCS) -c $< -o $@

%.arm.o : %.S Makefile $(HFILES)
	$(ARMCC) $(ARMCCFLAGS) $(INCS) -c $< -o $@

$(TARGETSPRITES)-hres.prc:
	$(PILRC) -ro -o $(TARGETSPRITES)-hres.prc -creator $(CREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) Rsc/hres_sprites.rcp

$(TARGETSPRITES)-hres-grey.prc:
	$(PILRC) -ro -o $(TARGETSPRITES)-hres-grey.prc -creator $(CREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) Rsc/hres_grey_sprites.rcp

$(TARGETSPRITES)-mres.prc:
	$(PILRC) -ro -o $(TARGETSPRITES)-mres.prc -creator $(CREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) Rsc/mres_sprites.rcp

$(TARGETSPRITES)-mres-grey.prc:
	$(PILRC) -ro -o $(TARGETSPRITES)-mres-grey.prc -creator $(CREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) Rsc/mres_grey_sprites.rcp

$(TARGETSPRITES)-lres.prc:
	$(PILRC) -ro -o $(TARGETSPRITES)-lres.prc -creator $(CREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) Rsc/lres_sprites.rcp

$(TARGETSPRITES)-lres-grey.prc:
	$(PILRC) -ro -o $(TARGETSPRITES)-lres-grey.prc -creator $(CREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) Rsc/lres_grey_sprites.rcp

$(TARGETSPRITES)-2bpp.prc:
	$(PILRC) -ro -o $(TARGETSPRITES)-2bpp.prc -creator $(CREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) Rsc/2bpp_sprites.rcp

$(TARGETSPRITES)-1bpp.prc:
	$(PILRC) -ro -o $(TARGETSPRITES)-1bpp.prc -creator $(CREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) Rsc/1bpp_sprites.rcp

.PHONY: clean
clean:
	rm -rf $(OBJS-68k) $(OBJS-arm) $(TARGET).prc $(TARGETSPRITES).prc
