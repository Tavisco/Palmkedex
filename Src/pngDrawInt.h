#ifndef _PNG_DRAW_INT_H_
#define _PNG_DRAW_INT_H_

#include <stdint.h>		//avoid PalmOS includes in ARM code

struct BitmapType;

struct DrawState {
    struct BitmapType *b;
    uint16_t *bits;
    uint16_t rowHalfwords;
};

//68k entry
int pngDrawDecode(struct DrawState *ds, const void *data, uint32_t dataSz);

struct ArmParams {
	struct DrawState *ds;
	const void *data;
	uint32_t dataSz;
};

#endif
