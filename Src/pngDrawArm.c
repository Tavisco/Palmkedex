#include "Src/pngle.h"
#include "pngDrawInt.h"
#include "armcalls.h"


struct DrawStateWrapper {
	struct DrawState ds;	//must be first
	uint32_t m68kCallback;
};

void on_draw(pngle_t *pngle, uint_fast16_t x, uint_fast16_t y, uint_fast16_t vR, uint_fast16_t vG, uint_fast16_t vB, uint_fast16_t vA, struct DrawState *ds)
{
	uint_fast16_t r = vR & 0xf8;
	uint_fast16_t g = vG & 0xfc;
	uint_fast16_t b = vB & 0xf8;
	uint_fast16_t color = (r << 8) + (g << 3) + (b >> 3);

	uint16_t *dst = ds->bits + y * ds->rowHalfwords + x;

	*dst = __builtin_bswap16(color);
}

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
	write16(&dst->rowHalfwords, read16(&src->rowHalfwords));
	write16(&dst->density, read16(&src->density));
	dst->densitySupportFlags = src->densitySupportFlags;
}

static unsigned char pngDrawHdrCbk(struct DrawState *ds, uint32_t w, uint32_t h)
{
	struct DrawStateWrapper *dsw = (struct DrawStateWrapper*)ds;
	struct DrawState ds68k;
	unsigned char ret;
	struct {
		struct DrawState *ds;
		uint32_t w;
		uint32_t h;
	} params = {
		.ds = (void*)__builtin_bswap32((uintptr_t)&ds68k),
		.w = __builtin_bswap32(w),
		.h = __builtin_bswap32(h),
	};
	
	prvSwapDs(&ds68k, ds);
	ret = armCallDo(dsw->m68kCallback, &params, sizeof(params));
	prvSwapDs(ds, &ds68k);
	
	return ret;
}

int __attribute__((used)) ArmletMain(void *emulStateP, struct ArmParams *pp, void *call68KFuncP)
{
	struct DrawStateWrapper dsw;
	struct DrawState *ds68k;
	pngle_t *pngle;
	int ret;

	armCallsInit(emulStateP, call68KFuncP);

	ds68k = (struct DrawState*)read32(&pp->ds);
	prvSwapDs(&dsw.ds, ds68k);
	dsw.m68kCallback = read32(&pp->hdrDecodedF);
	
	pngle = pngle_new();
	if (!pngle)
		return -1;

	pngle_set_draw_callback(pngle, &dsw.ds);
	pngle_set_init_callback(pngle, pngDrawHdrCbk);

	ret = pngle_feed(pngle, (const void*)read32(&pp->data), read32(&pp->dataSz));
	
	pngle_destroy(pngle);
	prvSwapDs(ds68k, &dsw.ds);

	return ret;
}

void __attribute((naked, used, section(".vector"), target("arm"))) __entry(void)
{
	//gcc will refuse to call a thumb functionj from this arm entry point no matter what we do
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
