#ifndef _PNG_DRAW_INT_H_
#define _PNG_DRAW_INT_H_

#include <stdbool.h>
#include <stdint.h>		//avoid PalmOS includes in ARM code

struct BitmapType;

struct DrawState {
	struct BitmapType *b;
	uint16_t *bits;
	
	uint32_t expectedW;
	uint32_t expectedH;
	
	uint16_t rowHalfwords;
	uint16_t density;
	
	uint8_t densitySupportFlags;
};

//callback on size
typedef unsigned char (*PngHdrDecodedCbkF)(struct DrawState *ds, uint32_t width, uint32_t height);

//68k entry
int pngDrawDecode(struct DrawState *ds, const void *data, uint32_t dataSz, PngHdrDecodedCbkF hdrCbk);

struct ArmParams {
	struct DrawState *ds;
	const void *data;
	uint32_t dataSz;
	
	PngHdrDecodedCbkF hdrDecodedF;
};

#endif
