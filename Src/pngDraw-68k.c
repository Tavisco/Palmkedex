#include <PalmOS.h>
#include "Src/pngle.h"
#include "pngDrawInt.h"

#pragma GCC optimize ("O3")


void __attribute__((visibility("hidden"),always_inline)) on_draw(pngle_t *pngle, uint_fast16_t x, uint_fast16_t y, uint_fast16_t vR, uint_fast16_t vG, uint_fast16_t vB, uint_fast16_t vA, struct DrawState *ds)
{
	uint_fast16_t r = vR & 0xf8;
	uint_fast16_t g = vG & 0xfc;
	uint_fast16_t b = vB & 0xf8;
	uint_fast16_t color = (r << 8) + (g << 3) + (b >> 3);
	uint32_t offset;

	//le sigh....gcc refuses to use 16x16->32 multiplication as it should here. The perf cost is large so we make it!
	//the code here does what this line SHOULD do:
	// offset = (uint32_t)(uint16_t)y * (uint32_t)(uint16_t)ds->rowHalfwords;
	asm("move.w %1, %0 \n mulu.w %2, %0" :"=&d"(offset): "d"(y) , "d"(ds->rowHalfwords));
	

	uint16_t *dst = ds->bits + offset + x;

	*dst = color;
}

int pngDrawDecode(struct DrawState *ds, const void *data, uint32_t dataSz)
{
	pngle_t *pngle;
	int ret;

	pngle = pngle_new();
	pngle_set_draw_callback(pngle, ds);
	ret = pngle_feed(pngle, data, dataSz);
	pngle_destroy(pngle);

	return ret;
}
