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
LTO				=	-flto
ARMLTO			=	-flto
ARMTYPE			=	-mthumb
COMMON			=	-DPNGLE_NO_GAMMA_CORRECTION -DPNGLE_SKIP_CRC
M68KCOMMON		=	$(COMMON) -Wno-multichar -funsafe-math-optimizations -Os -m68000 -mno-align-int -mpcrel -fpic -fshort-enums
ARMCOMMON		=	$(COMMON) -Os -march=armv4t $(ARMTYPE) -mno-unaligned-access -ffixed-r9 -ffixed-r10 -ffixed-r11 -fomit-frame-pointer -D__ARM__ -ffreestanding -fpic -mthumb-interwork
WARN			=	-Wsign-compare -Wextra -Wall -Wno-unused-parameter -Wno-old-style-declaration -Wno-unused-function -Wno-unused-variable -Wno-error=cpp -Wno-error=switch
LKR				=	Src/68k.lkr
ARMLKR			=	Src/arm.lkr
CCFLAGS			=	$(LTO) $(WARN) $(M68KCOMMON) -I. -ffunction-sections -fdata-sections
LDFLAGS			=	$(LTO) $(WARN) $(M68KCOMMON) -Wl,--gc-sections -Wl,-T $(LKR)
ARMCCFLAGS		=	$(ARMLTO) $(WARN) $(ARMCOMMON) -I. -ffunction-sections -fdata-sections
ARMLDFLAGS		=	$(ARMLTO) $(WARN) $(ARMCOMMON) -Wl,--gc-sections -Wl,-T $(ARMLKR)
SRCS-68k		=   Src/Palmkedex.c Src/Main.c Src/PkmnMain.c Src/PkmnType.c Src/helpers.c Src/miniz.c Src/pngle.c Src/pngDraw.c Src/pngDraw-68k.c
SRCS-arm		=	Src/helpers.c Src/miniz.c Src/pngle.c Src/pngDrawArm.c Src/armcalls.c
RCP				=	Rsc/Palmkedex_Rsc.rcp
SPRITESRCP		=	Rsc/pkmn_sprites.rcp
RSC				=	Src/
TARGET			=	Palmkedex
TARGETSPRITES	=	PalmkedexSprites
CREATOR			=	PKDX
TYPE			=	appl
SPRITECREATOR	=	PKSP
SPRITETYPE		=	pSPR

#add PalmOS SDK
#INCS			+=	-isystem "gccisms"
INCS			+=	-isystem "$(SDK)"
INCS			+=	-isystem "$(SDK)/Core"
INCS			+=	-isystem "$(SDK)/Core/Hardware"
INCS			+=	-isystem "$(SDK)/Core/System"
INCS			+=	-isystem "$(SDK)/Core/UI"
INCS			+=	-isystem "$(SDK)/Dynamic"
INCS			+=	-isystem "$(SDK)/Libraries"
INCS			+=	-isystem "$(SDK)/Libraries/PalmOSGlue"

#leave this alone
OBJS-68k		=	$(patsubst %.S,%.68k.o,$(patsubst %.c,%.68k.o,$(SRCS-68k)))
OBJS-arm		=	$(patsubst %.S,%.arm.o,$(patsubst %.c,%.arm.o,$(SRCS-arm)))
all: $(TARGET).prc $(TARGETSPRITES).prc

$(TARGET).prc: code0001.68k.bin armc0001.arm.bin
	$(PILRC) -ro -o $(TARGET).prc -creator $(CREATOR) -type $(TYPE) -name $(TARGET) -I $(RSC) $(RCP)

%.68k.bin: %.68k.elf
	$(OBJCOPY) -O binary $< $@ -j.vec -j.text -j.rodata

%.arm.bin: %.arm.elf
	$(ARMOBJCOPY) -O binary $< $@ -j.text -j.rodata

%.68k.elf: $(OBJS-68k)
	$(LD) -o $@ $(LDFLAGS) $^

%.arm.elf: $(OBJS-arm)
	$(ARMLD) -o $@ $(ARMLDFLAGS) $^

%.68k.o : %.c Makefile
	$(CC) $(CCFLAGS)  $(INCS) -c $< -o $@

%.arm.o : %.c Makefile
	$(ARMCC) $(ARMCCFLAGS) $(INCS) -c $< -o $@

$(TARGETSPRITES).prc:
	$(PILRC) -ro -o $(TARGETSPRITES).prc -creator $(SPRITECREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) $(SPRITESRCP)

.PHONY: clean
clean:
	rm -rf $(OBJS-68k) $(TARGET).prc $(TARGETSPRITES).prc