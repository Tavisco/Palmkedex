#include "imgDrawInt.h"
#include "pnoRuntime.h"
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

int __attribute__((used)) ArmletMain(void *emulStateP, struct ArmParams *pp, void *call68kFuncPtr);
int __attribute__((used)) ArmletMain(void *emulStateP, struct ArmParams *pp, void *call68kFuncPtr /* will be garbase if called via dicrect call */)
{
	void *call68kFuncPtrActual = (void*)read32(&pp->call68KFuncP);
	struct DrawState *ds68k;
	struct Context *ctx;
	int ret;

	if (!call68kFuncPtrActual)	//if none provide by68k, use what the system gave us
		call68kFuncPtrActual = call68kFuncPtr;

	armCallsInit(emulStateP, call68kFuncPtrActual);

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
