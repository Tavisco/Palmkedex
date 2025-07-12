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
ARMTYPE			=	-mthumb		#shoudl be -mthumb or -marm
M68KCOMMON		=	$(COMMON) -Wno-multichar -funsafe-math-optimizations -Os -m68000 -mno-align-int -mpcrel -fpic -fshort-enums -mshort -fvisibility=hidden -Wno-attributes -g -ggdb3
ARMCOMMON		=	$(COMMON) -Ofast -march=armv4t $(ARMTYPE) -mno-unaligned-access -ffixed-r9 -ffixed-r10 -ffixed-r11 -fomit-frame-pointer -D__ARM__ -ffreestanding -fpic -mthumb-interwork -Wno-attributes
WARN			=	-Wsign-compare -Wextra -Wall -Wno-unused-parameter -Wno-old-style-declaration -Wno-unused-function -Wno-unused-variable -Wno-error=cpp -Wno-switch  -Wno-implicit-fallthrough
LKR				=	Src/68k.lkr
ARMLKR			=	Src/arm.lkr
CCFLAGS			=	$(LTO) $(WARN) $(M68KCOMMON) -I. -ffunction-sections -fdata-sections
LDFLAGS			=	$(LTO) $(WARN) $(M68KCOMMON) -Wl,--gc-sections -Wl,-T $(LKR)
ARMCCFLAGS		=	$(ARMLTO) $(WARN) $(ARMCOMMON) -I. -ffunction-sections -fdata-sections -nolibc
ARMLDFLAGS		=	$(ARMLTO) $(WARN) $(ARMCOMMON) -Wl,--gc-sections -Wl,-T $(ARMLKR)
SRCS-68k		=   Src/Palmkedex.c Src/Main.c Src/PkmnMain.c Src/PkmnType.c Src/pokeInfo.c Src/glue.c Src/helpers.c Src/osPatches.c Src/imgDraw.c Src/aciDecode.c Src/aciDecodeAsm68k.S Src/qrcode/qrcode.c Src/GridMain.c Src/preferences.c
SRCS-armc0001	=	Src/helpers.c Src/armcalls.c Src/aciDrawArmlet.c Src/aciDecode.c Src/aciDecodeARM.c
SRCS-armc0002	=	Src/helpers.c Src/armcalls.c Src/jpgDrawArmlet.c Src/nanojpg.c
RCP				=	Rsc/Palmkedex_Rsc.rcp
TARGET			=	Palmkedex
TARGETSPRITES	=	SpritePack
TARGETICONS		=	IconPack
TARGETITEMS		=	ItemsPack
CREATOR			=	PKDX
TYPE			=	appl
SPRITETYPE		=	pSPR
ICONTYPE		=	pICR
ITEMTYPE		=	ITEM


ARM_PIECES		=	armc0001 armc0002

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
BINS-arm		=	$(addsuffix .arm.bin,$(ARM_PIECES))
all: $(TARGET).prc $(TARGETSPRITES)-hres-4bpp.prc $(TARGETSPRITES)-hres-16bpp.prc $(TARGETSPRITES)-mres-1bpp.prc $(TARGETSPRITES)-mres-2bpp.prc $(TARGETSPRITES)-mres-4bpp.prc $(TARGETSPRITES)-mres-16bpp.prc $(TARGETSPRITES)-lres-1bpp.prc $(TARGETSPRITES)-lres-2bpp.prc $(TARGETSPRITES)-lres-4bpp.prc $(TARGETSPRITES)-lres-16bpp.prc $(TARGETSPRITES)-3x-colors.prc $(TARGETSPRITES)-3x-grayscale.prc $(TARGETICONS)-lres-16bpp.prc $(TARGETICONS)-lres-4bpp.prc $(TARGETICONS)-lres-2bpp.prc $(TARGETICONS)-lres-1bpp.prc $(TARGETICONS)-mres-16bpp.prc $(TARGETICONS)-mres-4bpp.prc $(TARGETICONS)-mres-2bpp.prc $(TARGETICONS)-mres-1bpp.prc $(TARGETICONS)-hres-16bpp.prc $(TARGETICONS)-hres-4bpp.prc $(TARGETICONS)-3x-colors.prc $(TARGETICONS)-3x-grayscale.prc $(TARGETITEMS)-1bpp.prc $(TARGETITEMS)-2bpp.prc $(TARGETITEMS)-4bpp.prc $(TARGETITEMS)-16bpp.prc
HFILES			=	$(wildcard Src/*.h)


$(TARGET).prc: code0001.68k.bin $(BINS-arm) $(RCP).real.rcp
	$(PILRC) -ro -o $(TARGET).prc -creator $(CREATOR) -type $(TYPE) -name $(TARGET) $(RCP).real.rcp

#begin grey magic: auto-renerate rules for each arm target in $(ARM_PIECES) out of sources in SRCS-<EACH_ITEM>
define ARM_ELF_RULE

OBJS-$(1)		:= $(patsubst %.S,%.arm.o,$(patsubst %.c,%.arm.o,$(SRCS-$(1))))

$(1).arm.elf: $$(OBJS-$(1))
	$(ARMLD) -o $$@ $$(ARMLDFLAGS) $$^

endef

$(eval \
        $(foreach _chunk,$(ARM_PIECES), \
                $(call ARM_ELF_RULE,$(_chunk)) \
        ) \
	)
#end grey magic

%.rcp.real.rcp: %.rcp Makefile $(HFILES)
	# PilRC's macro abilities are pitiful, so we call upon CPP :)
	cpp -o $@ $< -ISrc -I. -P

%.68k.bin: %.68k.elf
	$(OBJCOPY) -O binary $< $@ -j.vec -j.text -j.rodata

%.arm.bin: %.arm.elf
	$(ARMOBJCOPY) -O binary $< $@ -j.text -j.rodata

%.68k.elf: $(OBJS-68k)
	$(LD) -o $@ $(LDFLAGS) $^

%.68k.o : %.c Makefile $(HFILES)
	$(CC) $(CCFLAGS) $(INCS) -c $< -o $@

%.68k.o : %.S Makefile $(HFILES)
	$(CC) $(CCFLAGS) $(INCS) -c $< -o $@

%.arm.o : %.c Makefile $(HFILES)
	$(ARMCC) $(ARMCCFLAGS) $(INCS) -c $< -o $@

%.arm.o : %.S Makefile $(HFILES)
	$(ARMCC) $(ARMCCFLAGS) $(INCS) -c $< -o $@

$(TARGETSPRITES)-hres-4bpp.prc:
	$(PILRC) -ro -o $(TARGETSPRITES)-hres-4bpp.prc -creator $(CREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) Rsc/sprites_hres_4bpp.rcp

$(TARGETSPRITES)-hres-16bpp.prc:
	$(PILRC) -ro -o $(TARGETSPRITES)-hres-16bpp.prc -creator $(CREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) Rsc/sprites_hres_16bpp.rcp

$(TARGETSPRITES)-mres-1bpp.prc:
	$(PILRC) -ro -o $(TARGETSPRITES)-mres-1bpp.prc -creator $(CREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) Rsc/sprites_mres_1bpp.rcp

$(TARGETSPRITES)-mres-2bpp.prc:
	$(PILRC) -ro -o $(TARGETSPRITES)-mres-2bpp.prc -creator $(CREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) Rsc/sprites_mres_2bpp.rcp

$(TARGETSPRITES)-mres-4bpp.prc:
	$(PILRC) -ro -o $(TARGETSPRITES)-mres-4bpp.prc -creator $(CREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) Rsc/sprites_mres_4bpp.rcp

$(TARGETSPRITES)-mres-16bpp.prc:
	$(PILRC) -ro -o $(TARGETSPRITES)-mres-16bpp.prc -creator $(CREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) Rsc/sprites_mres_16bpp.rcp

$(TARGETSPRITES)-lres-1bpp.prc:
	$(PILRC) -ro -o $(TARGETSPRITES)-lres-1bpp.prc -creator $(CREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) Rsc/sprites_lres_1bpp.rcp

$(TARGETSPRITES)-lres-2bpp.prc:
	$(PILRC) -ro -o $(TARGETSPRITES)-lres-2bpp.prc -creator $(CREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) Rsc/sprites_lres_2bpp.rcp

$(TARGETSPRITES)-lres-4bpp.prc:
	$(PILRC) -ro -o $(TARGETSPRITES)-lres-4bpp.prc -creator $(CREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) Rsc/sprites_lres_4bpp.rcp

$(TARGETSPRITES)-lres-16bpp.prc:
	$(PILRC) -ro -o $(TARGETSPRITES)-lres-16bpp.prc -creator $(CREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) Rsc/sprites_lres_16bpp.rcp

$(TARGETSPRITES)-3x-colors.prc:
	$(PILRC) -ro -o $(TARGETSPRITES)-3x-colors.prc -creator $(CREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) Rsc/sprites_3x_colors.rcp

$(TARGETSPRITES)-3x-grayscale.prc:
	$(PILRC) -ro -o $(TARGETSPRITES)-3x-grayscale.prc -creator $(CREATOR) -type $(SPRITETYPE) -name $(TARGETSPRITES) Rsc/sprites_3x_grayscale.rcp

$(TARGETICONS)-lres-16bpp.prc:
	$(PILRC) -ro -o $(TARGETICONS)-lres-16bpp.prc -creator $(CREATOR) -type $(ICONTYPE) -name $(TARGETICONS) Rsc/icons_lres_16bpp.rcp

$(TARGETICONS)-lres-4bpp.prc:
	$(PILRC) -ro -o $(TARGETICONS)-lres-4bpp.prc -creator $(CREATOR) -type $(ICONTYPE) -name $(TARGETICONS) Rsc/icons_lres_4bpp.rcp

$(TARGETICONS)-lres-2bpp.prc:
	$(PILRC) -ro -o $(TARGETICONS)-lres-2bpp.prc -creator $(CREATOR) -type $(ICONTYPE) -name $(TARGETICONS) Rsc/icons_lres_2bpp.rcp

$(TARGETICONS)-lres-1bpp.prc:
	$(PILRC) -ro -o $(TARGETICONS)-lres-1bpp.prc -creator $(CREATOR) -type $(ICONTYPE) -name $(TARGETICONS) Rsc/icons_lres_1bpp.rcp

$(TARGETICONS)-mres-16bpp.prc:
	$(PILRC) -ro -o $(TARGETICONS)-mres-16bpp.prc -creator $(CREATOR) -type $(ICONTYPE) -name $(TARGETICONS) Rsc/icons_mres_16bpp.rcp

$(TARGETICONS)-mres-4bpp.prc:
	$(PILRC) -ro -o $(TARGETICONS)-mres-4bpp.prc -creator $(CREATOR) -type $(ICONTYPE) -name $(TARGETICONS) Rsc/icons_mres_4bpp.rcp

$(TARGETICONS)-mres-2bpp.prc:
	$(PILRC) -ro -o $(TARGETICONS)-mres-2bpp.prc -creator $(CREATOR) -type $(ICONTYPE) -name $(TARGETICONS) Rsc/icons_mres_2bpp.rcp

$(TARGETICONS)-mres-1bpp.prc:
	$(PILRC) -ro -o $(TARGETICONS)-mres-1bpp.prc -creator $(CREATOR) -type $(ICONTYPE) -name $(TARGETICONS) Rsc/icons_mres_1bpp.rcp

$(TARGETICONS)-hres-16bpp.prc:
	$(PILRC) -ro -o $(TARGETICONS)-hres-16bpp.prc -creator $(CREATOR) -type $(ICONTYPE) -name $(TARGETICONS) Rsc/icons_hres_16bpp.rcp

$(TARGETICONS)-hres-4bpp.prc:
	$(PILRC) -ro -o $(TARGETICONS)-hres-4bpp.prc -creator $(CREATOR) -type $(ICONTYPE) -name $(TARGETICONS) Rsc/icons_hres_4bpp.rcp

$(TARGETICONS)-3x-colors.prc:
	$(PILRC) -ro -o $(TARGETICONS)-3x-colors.prc -creator $(CREATOR) -type $(ICONTYPE) -name $(TARGETICONS) Rsc/icons_3x_colors.rcp

$(TARGETICONS)-3x-grayscale.prc:
	$(PILRC) -ro -o $(TARGETICONS)-3x-grayscale.prc -creator $(CREATOR) -type $(ICONTYPE) -name $(TARGETICONS) Rsc/icons_3x_grayscale.rcp

$(TARGETITEMS)-1bpp.prc:
	$(PILRC) -ro -o $(TARGETITEMS)-1bpp.prc -creator $(CREATOR) -type $(ITEMTYPE) -name $(TARGETITEMS) Rsc/items_lres_1bpp.rcp

$(TARGETITEMS)-2bpp.prc:
	$(PILRC) -ro -o $(TARGETITEMS)-2bpp.prc -creator $(CREATOR) -type $(ITEMTYPE) -name $(TARGETITEMS) Rsc/items_lres_2bpp.rcp

$(TARGETITEMS)-4bpp.prc:
	$(PILRC) -ro -o $(TARGETITEMS)-4bpp.prc -creator $(CREATOR) -type $(ITEMTYPE) -name $(TARGETITEMS) Rsc/items_lres_4bpp.rcp

$(TARGETITEMS)-16bpp.prc:
	$(PILRC) -ro -o $(TARGETITEMS)-16bpp.prc -creator $(CREATOR) -type $(ITEMTYPE) -name $(TARGETITEMS) Rsc/items_lres_16bpp.rcp

.PHONY: clean
clean:
	rm -rf $(OBJS-68k) $(OBJS-arm) $(TARGET).prc $(RCP).real.rcp *.68k.elf *.68k.bin *.arm.bin
