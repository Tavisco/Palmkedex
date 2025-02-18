#include "imgDrawInt.h"
#include "armcalls.h"


struct DrawStateWrapper {
	struct DrawState ds;	//must be first
	uint32_t m68kCallback;
};

static uint32_t read32(const void *fromP)			//read unaligned 32 bit in BE
{
	const uint8_t *from = fromP;
	uint32_t ret = 0;

	ret = (ret << 8) + from[0];
	ret = (ret << 8) + from[1];
	ret = (ret << 8) + from[2];
	ret = (ret << 8) + from[3];

	return ret;
}

static uint16_t read16(const void *fromP)			//read unaligned 16 bit in BE
{
	const uint8_t *from = fromP;
	uint16_t ret = 0;

	ret = (ret << 8) + from[0];
	ret = (ret << 8) + from[1];

	return ret;
}

static void write32(void *dstP, uint32_t val)		//write unaligned 32 bit in LE
{
	uint8_t *dst = dstP;

	dst[0] = val;
	dst[1] = val >> 8;
	dst[2] = val >> 16;
	dst[3] = val >> 24;
}

static void write16(void *dstP, uint16_t val)		//write unaligned 16 bit in LE
{
	uint8_t *dst = dstP;

	dst[0] = val;
	dst[1] = val >> 8;
}

static void prvSwapDs(struct DrawState *dst, const struct DrawState *src)
{
	write32(&dst->b, read32(&src->b));
	write32(&dst->bits, read32(&src->bits));
	write32(&dst->expectedW, read32(&src->expectedW));
	write32(&dst->expectedH, read32(&src->expectedH));
	write32(&dst->clut, read32(&src->clut));
	write16(&dst->actualH, read16(&src->actualH));
	write16(&dst->rowBytes, read16(&src->rowBytes));
	write16(&dst->density, read16(&src->density));
	dst->blitterDensitySupportBits = src->blitterDensitySupportBits;
	dst->depth = src->depth;
	dst->isDirectColor16bppNative = src->isDirectColor16bppNative;
}

static unsigned char pngDrawHdrCbk(struct DrawState *ds, uint32_t w, uint32_t h, struct ColortableEntry *colors, uint32_t numColors, unsigned char isGreyscale)
{
	struct DrawStateWrapper *dsw = (struct DrawStateWrapper*)ds;
	struct DrawState ds68k;
	unsigned char ret;
	struct {
		struct DrawState *ds;
		uint32_t w;
		uint32_t h;
		const struct ColortableEntry *colors;
		uint32_t numColors;
		unsigned char isGreyscale, padding;
	} __attribute__((packed)) params = {
		.ds = (void*)__builtin_bswap32((uintptr_t)&ds68k),
		.w = __builtin_bswap32(w),
		.h = __builtin_bswap32(h),
		.colors = (struct ColortableEntry*)__builtin_bswap32((uintptr_t)colors),
		.numColors = __builtin_bswap32(numColors),
		.isGreyscale = isGreyscale,
	};

	prvSwapDs(&ds68k, ds);
	ret = armCallDo(dsw->m68kCallback, &params, sizeof(params));
	prvSwapDs(ds, &ds68k);

	return ret;
}

int __attribute__((used)) ArmletMain(void *emulStateP, struct ArmParams *pp);
int __attribute__((used)) ArmletMain(void *emulStateP, struct ArmParams *pp)
{
	struct DrawStateWrapper dsw;
	struct DrawState *ds68k;
	int ret;

	armCallsInit(emulStateP, (void*)read32(&pp->call68KFuncP));

	ds68k = (struct DrawState*)read32(&pp->ds);
	prvSwapDs(&dsw.ds, ds68k);
	dsw.m68kCallback = read32(&pp->hdrDecodedF);

	ret = aciDecode(&dsw.ds, (const void*)read32(&pp->data), read32(&pp->dataSz), pngDrawHdrCbk);

	prvSwapDs(ds68k, &dsw.ds);

	//record return value into d0 as well for direct calls
	((int*)emulStateP)[1] = ret;

	return ret;
}

void __attribute((naked, used, section(".vector"), target("arm"))) __entry(void);
void __attribute((naked, used, section(".vector"), target("arm"))) __entry(void)
{
	//gcc will refuse to call a thumb function from this arm entry point no matter what we do
	//so we are forced to do it ourselves if we want to compile for thumb (we do for space)

	asm volatile(
		"1:									\n"
		"	stmfd	sp!, {r10, r11, lr}		\n"
		"	ldr		r10, =1b				\n"
		"	adr		r11, 1b					\n"
		"	ldr		r12, =ArmletMain		\n"
		"	sub		r12, r10				\n"
		"	add		r12, r11				\n"
		"	mov		lr, pc					\n"
		"	bx		r12						\n"
		"	ldmfd	sp!, {r10, r11, lr}		\n"
		"	bx		lr						\n"
	);
}
