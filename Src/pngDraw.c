#include <PalmOS.h>
#include "Src/pngle.h"
#include "pngDraw.h"

struct DrawState {
    struct BitmapType *b;
    uint16_t *bits;
    uint16_t rowHalfwords;
};

static struct DrawState* setupDrawState(uint32_t w, uint32_t h) {
	Err err;
	BitmapPtr b = BmpCreate(w, h, 16, NULL, &err);

	// Check if BmpCreate succeeded
	if (b == NULL) {
		if (err == sysErrParamErr)
		{
			ErrFatalDisplay("Sprites not supported on this device as of now! Please uninstall them to use Palmkedex.");
		}
		if (err != sysErrNoFreeResource)
		{
			ErrFatalDisplay("Not enough memory!");
		}
		ErrFatalDisplay("Error creating bitmap!");
		return NULL;
	}

	struct DrawState *ds = (struct DrawState *)MemPtrNew(sizeof(struct DrawState));

	// Check if MemPtrNew succeeded
	if (!ds) {
		BmpDelete(b);
		ErrFatalDisplay("Error allocating memory for draw state!");
		return NULL;
	}

	MemSet(ds, sizeof(struct DrawState), 0);
	UInt16 rowBytes;

	BmpGetDimensions(b, NULL, NULL, &rowBytes);
	ds->rowHalfwords = rowBytes / sizeof(UInt16);
	ds->b = b;
	ds->bits = BmpGetBits(b);

	if (ds->bits == NULL) {
		BmpDelete(b);
		ErrFatalDisplay("Error getting bitmap bits!");
		return NULL;
	}

	return ds;
}

static void finish(struct DrawState *ds, uint32_t x, uint32_t y)
{
	WinDrawBitmap(ds->b, x, y);
	BmpDelete(ds->b);
	MemPtrFree(ds);
}

void on_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4], struct DrawState *ds)
{
	UInt16 r = rgba[0] & 0xf8;
	UInt16 g = rgba[1] & 0xfc;
	UInt16 b = rgba[2] & 0xf8;
	UInt16 color = (r << 8) + (g << 3) + (b >> 3);

	UInt16 *dst = ds->bits + (UInt32)(UInt16)y * (UInt32)(UInt16)ds->rowHalfwords + x;

	*dst = color;
}

void pngDrawStateFree(struct DrawState *ds)
{
	BmpDelete(ds->b);
	MemPtrFree(ds);
}

void pngDrawRedraw(struct DrawState *ds, int16_t x, int16_t y)
{
	WinDrawBitmap(ds->b, x, y);
}

void pngDrawAt(struct DrawState **dsP, const void *data, uint32_t dataSz, int16_t x, int16_t y, uint32_t w, uint32_t h)
{
	struct DrawState *ds;
	pngle_t *pngle;
	int ret;

	// Start the PNG decoding and drawing to memory
	ds = setupDrawState(64, 64);
	ErrFatalDisplayIf(!ds, "Failed to setup DrawState!");

	pngle = pngle_new();
	pngle_set_draw_callback(pngle, ds);

	ret = pngle_feed(pngle, data, dataSz);
	ErrFatalDisplayIf(ret < 0, "Error feeding PNG data!");

	pngle_destroy(pngle);

	pngDrawRedraw(ds, x, y);

	*dsP = ds;
}
