#ifndef _IMG_DRAW_H_
#define _IMG_DRAW_H_

#include <stdbool.h>
#include <stdint.h>		//avoid PalmOS includes in ARM code

struct BitmapType;
struct DrawState;

//decodes and stores state in "dsP"
bool imgDecode(struct DrawState **dsP, const void *data, uint32_t dataSz, uint32_t expectedW, uint32_t expectedH, uint8_t decodeAtThisDepth /* 0 for whatever screen is */);

//delete am already-decoded state
void imgDrawStateFree(struct DrawState *ds);

//redraw an already-decoded state
void imgDrawRedraw(struct DrawState *ds, int16_t x, int16_t y);

//do not use this unless you really need it
const void* imgGetBits(struct DrawState *ds);

#endif
