#ifndef _POKE_IMG_MGR_H_
#define _POKE_IMG_MGR_H_

#include <PalmOS.h>
#include "imgDraw.h"

enum PokeImgSize {
	PokeImgIcon,	//small (40 x40 std res)
	PokeImgSprite,	//large (96x96 std res)
	PokeImgItemIcon,	//item (32x32 std res)
};

//poke index starts at 0. requesting image for poke# 0 is guaranteed to produce a placeholder image, and this may be relied on

void pokeImgMgrCleanup(void);

UInt16 __attribute__((const)) pokeImgGetDim(enum PokeImgSize sz);
void pokeImgGetRect(RectangleType *rect, enum PokeImgSize sz, Coord x, Coord y);
void pokeImgDraw(UInt16 pokeID, enum PokeImgSize sz, Coord x, Coord y);

struct DrawState* pokeImgDecodeOnly(UInt16 pokeID, enum PokeImgSize sz, UInt8 forcedDepth);	//does not hit cache, mus tbe manually freed using imgDrawStateFree()



#endif
