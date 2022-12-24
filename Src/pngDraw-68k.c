#include <PalmOS.h>
#include "Src/pngle.h"
#include "pngDrawInt.h"


void on_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4], struct DrawState *ds)
{
	uint_fast16_t r = rgba[0] & 0xf8;
	uint_fast16_t g = rgba[1] & 0xfc;
	uint_fast16_t b = rgba[2] & 0xf8;
	uint_fast16_t color = (r << 8) + (g << 3) + (b >> 3);

	uint16_t *dst = ds->bits + (UInt32)(UInt16)y * (UInt32)(UInt16)ds->rowHalfwords + x;

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
