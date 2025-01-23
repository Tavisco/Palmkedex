#include "imgDrawInt.h"
#include "armcalls.h"
#include "nanojpg.h"
#include <stdlib.h>


void* jpgExtAlloc(size_t size)
{
	return malloc(size);
}

void* jpgExtRealloc(void* ptr, size_t size)
{
	return realloc(ptr, size);
}

void jpgExtFree(void* ptr)
{
	free(ptr);
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


static unsigned char jpgDrawHdrCbk(uint32_t m68kCallback, struct DrawState *ds68k, uint32_t w, uint32_t h, struct ColortableEntry *colors, uint32_t numColors, unsigned char isGreyscale)
{
	unsigned char ret;
	struct {
		struct DrawState *ds;
		uint32_t w;
		uint32_t h;
		const struct ColortableEntry *colors;
		uint32_t numColors;
		unsigned char isGreyscale, padding;
	} __attribute__((packed)) params = {
		.ds = (void*)__builtin_bswap32((uintptr_t)ds68k),
		.w = __builtin_bswap32(w),
		.h = __builtin_bswap32(h),
		.colors = (struct ColortableEntry*)__builtin_bswap32((uintptr_t)colors),
		.numColors = __builtin_bswap32(numColors),
		.isGreyscale = isGreyscale,
	};

	ret = armCallDo(m68kCallback, &params, sizeof(params));

	return ret;
}

static uint_fast16_t rbg565(uint_fast8_t r, uint_fast8_t g, uint_fast8_t b)
{
	r = (8224 * r) >> 16;
	b = (8224 * b) >> 16;
	g = (16448 * g) >> 16;

	return (((uint_fast16_t)r) << 11) + (((uint_fast16_t)g) << 5) + b;
}

int __attribute__((used)) ArmletMain(void *emulStateP, struct ArmParams *pp);
int __attribute__((used)) ArmletMain(void *emulStateP, struct ArmParams *pp)
{
	struct DrawState *ds68k;
	struct Context *ctx;
	int ret;

	armCallsInit(emulStateP, (void*)read32(&pp->call68KFuncP));

	ds68k = (struct DrawState*)read32(&pp->ds);

	ctx = jpgDecode((const void*)read32(&pp->data), read32(&pp->dataSz));
	if (!ctx)
		ret = -1;
	else {
		if (jpgGetResult(ctx) != JpegResultOK)
			ret = -1;
		else {
			bool isColor = jpgIsColor(ctx);
			uint32_t h = jpgGetHeight(ctx), w = jpgGetWidth(ctx);
			bool doContinue = jpgDrawHdrCbk(read32(&pp->hdrDecodedF), ds68k, w, h, NULL, isColor ? 65536 : 256, !isColor);

			if (doContinue) {

				uint16_t *dst = (uint16_t*)read32(&ds68k->bits);
				uint32_t r, c, rowBytes = read16(&ds68k->rowBytes), strideR, strideG, strideB;

				const uint8_t *srcR = jpgGetImageData(ctx, 0);
				const uint8_t *srcG = jpgGetImageData(ctx, isColor ? 1 : 0);
				const uint8_t *srcB = jpgGetImageData(ctx, isColor ? 2 : 0);
				strideR = jpgGetImageStride(ctx, 0);
				strideG = jpgGetImageStride(ctx, isColor ? 1 : 0);
				strideB = jpgGetImageStride(ctx, isColor ? 2 : 0);

				for (r = 0; r < h; r++) {
					for (c = 0; c < w; c++) {

						//for greyscale, this is suboptimal, but we do not expect that to be a common case
						*dst++ = __builtin_bswap16(rbg565(*srcR++, *srcG++, *srcB++));	//cause we need to produce a BE image...
					}
					dst += rowBytes / 2 - c;
					srcR += strideR - w;
					srcG += strideG - w;
					srcB += strideB - w;
				}
				ret = 0;
			}
			else
				ret = -3;
		}
		jpgFree(ctx);
	}

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
