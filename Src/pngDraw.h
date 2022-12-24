#ifndef _PNG_DRAW_H_
#define _PNG_DRAW_H_

#include <stdbool.h>
#include <stdint.h>		//avoid PalmOS includes in ARM code

struct BitmapType;
struct DrawState;

//decodes and stores state in "dsP"
void pngDrawAt(struct DrawState **dsP, const void *data, uint32_t dataSz, int16_t x, int16_t y, uint32_t expectedW, uint32_t expectedH);

//delete am already-decoded state
void pngDrawStateFree(struct DrawState *ds);

//redraw an already-decoded state
void pngDrawRedraw(struct DrawState *ds, int16_t x, int16_t y);



#endif
