#ifndef _PNG_DRAW_H_
#define _PNG_DRAW_H_

#include <stdbool.h>
#include <stdint.h>		//avoid PalmOS includes in ARM code

struct BitmapType;
struct DrawState;

#define PNG_DOUBLE_DENSITY_SUPPORTED		1		//sonyHR only supports double
#define PNG_VARIOUS_DENSITIES_SUPPORTED		2		//palmHR supports various

//decodes and stores state in "dsP"
void pngDrawAt(struct DrawState **dsP, const void *data, uint32_t dataSz, int16_t x, int16_t y, uint32_t expectedW, uint32_t expectedH, uint8_t densitysupportFlags);

//delete am already-decoded state
void pngDrawStateFree(struct DrawState *ds);

//redraw an already-decoded state
void pngDrawRedraw(struct DrawState *ds, int16_t x, int16_t y);



#endif
