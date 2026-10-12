#include "UiResourceIDs.h"
#include "pokeImageMgr.h"
#include "Palmkedex.h"


#define POKE_ICON_SIZE						40
#define POKE_IMAGE_SIZE						96
#define ITEM_ICON_SIZE						32

struct PokeImgMgrGlobals {

	UInt16 curPoke;
	struct DrawState *curSprite;

	DmOpenRef spriteDb;
	DmOpenRef iconDb;
	DmOpenRef itemDb;
};

static struct PokeImgMgrGlobals* __attribute__((const)) getGlobals(void)
{
	struct PokeImgMgrGlobals **gP = (struct PokeImgMgrGlobals**)globalsSlotPtr(GLOBALS_SLOT_IMG_MGR);
	struct PokeImgMgrGlobals *g = *gP;

	if (!g) {

		g = MemPtrNew(sizeof(*g));
		MemSet(g, sizeof(*g), 0);

		*gP = g;
	}
	return g;
}

static DmOpenRef pokeImgPrvOpenDb(enum PokeImgSize sz)
{
	struct PokeImgMgrGlobals *g = getGlobals();
	DmOpenRef *refP, db;
	UInt32 dbType;

	switch (sz) {
		case PokeImgIcon:
			refP = &g->iconDb;
			dbType = ICON_RESOURCE_DB;
			break;

		case PokeImgSprite:
			refP = &g->spriteDb;
			dbType = SPRITE_RESOURCE_DB;
			break;

		case PokeImgItemIcon:
			refP = &g->itemDb;
			dbType = ITEM_RESOURCE_DB;
			break;

		default:
			return NULL;
	}

	if (!*refP) {

		*refP = DmOpenDatabaseByTypeCreator(dbType, appFileCreator, dmModeReadOnly);
	}

	return *refP;
}

static MemHandle pokeImgPrvGetImgRes(UInt16 pokeID, enum PokeImgSize sz)
{
	DmOpenRef db = pokeImgPrvOpenDb(sz);
	UInt32 resType;
	MemHandle mh;
	UInt16 idx;

	if (!db)
		return NULL;

	switch (sz) {
		case PokeImgIcon:	resType = ICON_RESOURCE_TYPE;	break;
		case PokeImgSprite:	resType = SPRITE_RESOURCE_TYPE;	break;
		case PokeImgItemIcon:	resType = ITEM_RESOURCE_TYPE;	break;
		default:			return NULL;
	}

	idx = DmFindResource(db, resType, pokeID, NULL);
	if (idx == 0xffffu)
		return NULL;

	return DmGetResourceIndex(db, idx);
}

void pokeImgMgrCleanup(void)
{
	struct PokeImgMgrGlobals *g = getGlobals();

	if (g->curSprite) {
		imgDrawStateFree(g->curSprite);
		g->curSprite = NULL;
	}

	if (g->spriteDb) {

		DmCloseDatabase(g->spriteDb);
		g->spriteDb = NULL;
	}
	if (g->iconDb) {

		DmCloseDatabase(g->iconDb);
		g->iconDb = NULL;
	}
	if (g->itemDb) {

		DmCloseDatabase(g->itemDb);
		g->itemDb = NULL;
	}
	MemPtrFree(g);
	*globalsSlotPtr(GLOBALS_SLOT_IMG_MGR) = NULL;
}

static UInt16 pokeImgGetDimUnscaled(enum PokeImgSize sz)
{
	UInt16 ret;

	switch (sz) {
		case PokeImgIcon:	ret = POKE_ICON_SIZE;	break;
		case PokeImgSprite:	ret =  POKE_IMAGE_SIZE;	break;
		case PokeImgItemIcon:	ret = ITEM_ICON_SIZE;	break;
		default:			return 0;
	}

	return ret;
}

UInt16 pokeImgGetDim(enum PokeImgSize sz)
{
	UInt16 ret = pokeImgGetDimUnscaled(sz);

	//unlike Sony and Palm, Handera hires deals with standard coords, just bigger screen, thus this is necessary
	if (isHanderaHiRes())
		ret = ret * 3 / 2;

	return ret;
}

void pokeImgGetRect(RectangleType *rect, enum PokeImgSize sz, Coord x, Coord y)
{
	rect->topLeft.x = x;
	rect->topLeft.y = y;
	rect->extent.x = rect->extent.y = pokeImgGetDim(sz);
}

struct DrawState* pokeImgDecodeOnly(UInt16 pokeID, enum PokeImgSize sz, UInt8 forcedDepth)
{
	UInt16 expectedDim = pokeImgGetDimUnscaled(sz);
	struct DrawState *ds = NULL;
	MemHandle mh;

	mh = pokeImgPrvGetImgRes(pokeID, sz);
	if (mh) {

		if (!imgDecode(&ds, MemHandleLock(mh), MemHandleSize(mh), expectedDim, expectedDim, forcedDepth))
			ds = NULL;
		MemHandleUnlock(mh);
		DmReleaseResource(mh);
	}
	if (sz == PokeImgItemIcon) {
		struct PokeImgMgrGlobals *g = getGlobals();

		// ItemsPack also contains DESC, INFO and INDX resources. Keep it out
		// of the resource search path used for Pokemon descriptions and names.
		if (g->itemDb) {
			DmCloseDatabase(g->itemDb);
			g->itemDb = NULL;
		}
	}
	return ds;
}

void pokeImgDraw(UInt16 pokeID, enum PokeImgSize sz, Coord x, Coord y)
{
	struct PokeImgMgrGlobals *g = getGlobals();
	struct DrawState *ds = NULL;
	UInt16 missingResID;
	MemHandle mh;

	if (sz == PokeImgSprite && g->curSprite) {

		if (g->curPoke == pokeID) {

			imgDrawRedraw(g->curSprite, x, y);
			return;
		}
		imgDrawStateFree(g->curSprite);
		g->curSprite = NULL;
	}

	switch (sz) {
		case PokeImgIcon:	missingResID = BmpMissingIcon;		break;
		case PokeImgItemIcon:	missingResID = BmpMissingIcon;	break;
		case PokeImgSprite:	missingResID = BmpMissingPokemon;	break;
		default:			return;
	}

	ds = pokeImgDecodeOnly(pokeID, sz, 0);
	if (ds) {
		imgDrawRedraw(ds, x, y);

		if (sz == PokeImgSprite) {
			g->curSprite = ds;
			g->curPoke = pokeID;
		}
		else {

			imgDrawStateFree(ds);
		}
	}
	else {

		mh = DmGetResource(bitmapRsc, missingResID);
		WinDrawBitmap(MemHandleLock(mh), x, y);
		MemHandleUnlock(mh);
		DmReleaseResource(mh);
	}
}
