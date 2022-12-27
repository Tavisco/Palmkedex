#ifndef _IMG_DRAW_INT_H_
#define _IMG_DRAW_INT_H_

#include <stdbool.h>
#include <stdint.h>		//avoid PalmOS includes in ARM code

struct BitmapType;

struct DrawState {
	struct BitmapType *b;
	uint8_t *bits;

	uint32_t expectedW;
	uint32_t expectedH;

	uint16_t rowBytes;
	uint16_t density;

	uint8_t densitySupportFlags;
	uint8_t depth;
};

struct ColortableEntry {
	uint8_t index, r, g, b;		//set "index" as you please got given colors
	uint32_t rfu;
};

//callback on size
typedef unsigned char (*ImgHdrDecodedCbkF)(struct DrawState *ds, uint32_t width, uint32_t height, struct ColortableEntry *colors, uint16_t numColors, unsigned char isGreyscale);

//68k entries
int aciDecode(struct DrawState *ds, const void *data, uint32_t dataSz, ImgHdrDecodedCbkF hdrCbk);
void aciRepack(uint8_t* buffer, uint32_t npixels, uint8_t depth);

struct ArmParams {
	struct DrawState *ds;
	const void *data;
	uint32_t dataSz;

	ImgHdrDecodedCbkF hdrDecodedF;
};

#endif
