#include "UiResourceIDs.h"
#include "pokeImageMgr.h"
#include "BUILD_TYPE.h"
#include "Palmkedex.h"
#include "imgDraw.h"
#include <VFSMgr.h>


#define POKE_ICON_SIZE						40
#define POKE_IMAGE_SIZE						96
#define ITEM_ICON_SIZE						32

enum ImageSourceType {
	ImageSourceTypeNotYetSet = 0,			//not known -- will try
	ImageSourceTypeNone,					//known to not exist. set to avoid more retries
	ImageSourceTypeInternal,
	ImageSourceTypeVfs,
};

struct ImageSource {
	enum ImageSourceType typ;
	enum PokeImgSize sz;
	UInt32 resType;
	union {
		struct {
			DmOpenRef db;
		} internal;
		struct {
			UInt16 vrn;
			FileRef file;
		} vfs;
	} data;
};

struct PokeImgMgrGlobals {

	struct DrawState *curSprite;
	UInt16 curPoke;

	Boolean vfsCapable;
	struct ImageSource srcSprites;
	struct ImageSource srcIcons;
	DmOpenRef itemDb;
};

static Err pokeImgPrvVolumeUnmountedNotif(SysNotifyParamType *notifyParamsP)
{
	//something got unmounted. if it was a card we were using, reset all sources depending on it. they will retry later
	struct PokeImgMgrGlobals *g = (struct PokeImgMgrGlobals*)notifyParamsP->userDataP;
	UInt16 deadVRN = (UInt16)(UInt32)notifyParamsP->notifyDetailsP;

	if (g->vfsCapable) {

		if (g->srcSprites.typ == ImageSourceTypeVfs && g->srcSprites.data.vfs.vrn == deadVRN)
			g->srcSprites.typ = ImageSourceTypeNotYetSet;

		if (g->srcIcons.typ == ImageSourceTypeVfs && g->srcIcons.data.vfs.vrn == deadVRN)
			g->srcIcons.typ = ImageSourceTypeNotYetSet;
	}

	return errNone;
}

static struct PokeImgMgrGlobals* __attribute__((const)) getGlobals(void)
{
	struct PokeImgMgrGlobals **gP = (struct PokeImgMgrGlobals**)globalsSlotPtr(GLOBALS_SLOT_IMG_MGR);
	struct PokeImgMgrGlobals *g = *gP;

	if (!g) {

		g = MemPtrNew(sizeof(*g));
		MemSet(g, sizeof(*g), 0);

		g->srcSprites.sz = PokeImgSprite;
		g->srcIcons.sz = PokeImgIcon;

		#ifdef VFS_DATA_SUPPORT
			Boolean isSony;
			UInt16 cardNo;
			LocalID LID;
			UInt32 val;

			//we need:
			// * Notif manager (to be told when cards are unmounted)
			// * VFS manager for ... files
			// * OS2.0+ for StrPrintF
			// * OS3.0+ for WinSCreenMode

			//NOTES:
			// vfsMgrVersionNum is 200, sony early devices set it to "1" instead
			//  they implement all but VFSImportDatabaseFromFileCustom/VFSExportDatabaseToFileCustom
			isSony = errNone == FtrGet(sysFtrCreator, sysFtrNumOEMCompanyID, &val) &&
								val == 'sony';

			g->vfsCapable = errNone == FtrGet(sysFtrCreator, sysFtrNumNotifyMgrVersion, &val) &&
								val &&
								errNone == FtrGet(sysFileCVFSMgr, vfsFtrIDVersion, &val) &&
								(val >= vfsMgrVersionNum || (isSony && val == 1)) &&
								errNone == FtrGet(sysFtrCreator, sysFtrNumROMVersion, &val) &&
								val >= sysMakeROMVersion(3,0,0,sysROMStageDevelopment, 0) &&
								errNone == SysCurAppDatabase(&cardNo, &LID) &&
								errNone == SysNotifyRegister(cardNo, LID, sysNotifyVolumeUnmountedEvent, pokeImgPrvVolumeUnmountedNotif, sysNotifyNormalPriority, g);
		#endif

		*gP = g;
	}
	return g;
}

#ifdef VFS_DATA_SUPPORT
	static FileRef pokeImgPrvOpenVfsDataset(UInt16 vrn, const char *baseName, const char *densityName, UInt8 depth)
	{
		char filename[64];
		FileRef file;


		StrPrintF(filename, "/PALM/PROGRAMS/PALMKEDEX/%s-%s-%ubpp.prc", baseName, densityName, depth);
		if (errNone != VFSFileOpen(vrn, filename, vfsModeRead, &file))
			file = 0;

		return file;
	}
#endif

static void pokeImgPrvSourceRefresh(struct ImageSource *src)
{
	struct PokeImgMgrGlobals *g = getGlobals();

	if (src->typ == ImageSourceTypeNotYetSet) {

		const char *fileName;
		UInt32 dbType;

		switch (src->sz) {
			case PokeImgIcon:
				dbType = ICON_RESOURCE_DB;
				src->resType = ICON_RESOURCE_TYPE;
				fileName = "IconPack";
				break;

			case PokeImgSprite:
				dbType = SPRITE_RESOURCE_DB;
				src->resType = SPRITE_RESOURCE_TYPE;
				fileName = "SpritePack";
				break;

			default:
				return;
		}

		//internal always wins
		src->data.internal.db = DmOpenDatabaseByTypeCreator(dbType, appFileCreator, dmModeReadOnly);
		if (src->data.internal.db) {

			src->typ = ImageSourceTypeInternal;
			return;
		}

		#ifdef VFS_DATA_SUPPORT
			//try VFS
			if (g->vfsCapable) {

				static const UInt8 searchOrderNULL[] = {IMGDRAW_BLITTER_DENSITY_1x, 0};	//everyone can blit low-res

				UInt8 i, j, k, searchDepthMax, supportedDensities = imgDrawGetSupportedBitmapDensities(), bestDensity = imgDrawGetPreferredBitmapDensity();
				const UInt8 *searchOrder = searchOrderNULL;
				UInt32 hwSupportedDepths, vri;
				FileRef file = 0;
				UInt16 vrn;

				struct SearchParam {
					char densityName[5];
					UInt8 densityMask;
					UInt8 searchOrder[9];
					UInt32 foundDepths;
				} searches[] = {
					{"lres", IMGDRAW_BLITTER_DENSITY_1x, {IMGDRAW_BLITTER_DENSITY_1x, IMGDRAW_BLITTER_DENSITY_2x, IMGDRAW_BLITTER_DENSITY_3x, IMGDRAW_BLITTER_DENSITY_4x, IMGDRAW_BLITTER_DENSITY_1_5x, 0}, 0},
					{"mres", IMGDRAW_BLITTER_DENSITY_1_5x, {IMGDRAW_BLITTER_DENSITY_1_5x, IMGDRAW_BLITTER_DENSITY_3x, IMGDRAW_BLITTER_DENSITY_1x, IMGDRAW_BLITTER_DENSITY_2x, IMGDRAW_BLITTER_DENSITY_4x, 0}, 0},
					{"hres", IMGDRAW_BLITTER_DENSITY_2x, {IMGDRAW_BLITTER_DENSITY_2x, IMGDRAW_BLITTER_DENSITY_1x, IMGDRAW_BLITTER_DENSITY_4x, IMGDRAW_BLITTER_DENSITY_3x, IMGDRAW_BLITTER_DENSITY_1_5x, 0}, 0},
					{"3X", IMGDRAW_BLITTER_DENSITY_3x, {IMGDRAW_BLITTER_DENSITY_3x, IMGDRAW_BLITTER_DENSITY_1_5x, IMGDRAW_BLITTER_DENSITY_1x, IMGDRAW_BLITTER_DENSITY_2x, IMGDRAW_BLITTER_DENSITY_4x, 0}, 0},
					{"4X", IMGDRAW_BLITTER_DENSITY_4x, {IMGDRAW_BLITTER_DENSITY_4x, IMGDRAW_BLITTER_DENSITY_2x, IMGDRAW_BLITTER_DENSITY_1x, IMGDRAW_BLITTER_DENSITY_3x, IMGDRAW_BLITTER_DENSITY_1_5x, 0}, 0},
				};

				if (errNone != WinScreenMode(winScreenModeGetSupportedDepths, NULL, NULL, &hwSupportedDepths, NULL))
					hwSupportedDepths = 0;

				if (hwSupportedDepths & 0x8080)		//color devices can do it all
					searchDepthMax = 16;
				else if (hwSupportedDepths & 0x0008)	//16 grey devices
					searchDepthMax = 4;
				else if (hwSupportedDepths & 0x0002)	//4 grey devices
					searchDepthMax = 2;
				else if (hwSupportedDepths & 0x0001)	//b&w only devices
					searchDepthMax = 1;
				else
					searchDepthMax = 0;

				//scan to see ALL we can find, this is done sequentially per-card for speed
				for (vri = 0; vri != vfsIteratorStop && errNone == VFSVolumeEnumerate(&vrn, &vri);) {

					for (i = 0; i < sizeof(searches) / sizeof(*searches); i++) {


						//skip if we cannot draw this density
						if (!(supportedDensities & searches[i].densityMask))
							continue;

						for (j = 1; j <= searchDepthMax; j <<= 1) {

							file = pokeImgPrvOpenVfsDataset(vrn, fileName, searches[i].densityName, j);
							if (file) {
								searches[i].foundDepths |= 1UL << j;
								VFSFileClose(file);
								file = 0;
							}
						}
					}
				}

				//Pick the best option. We might have no perfect option, so we pick based on the following order:
				// 1. Pick correct density
				// 2. if unable, go for integer fractions (1/2, 1/3) so that upscaling is easy, start with least upscale factor
				// 3. if unable, go for integer multiples (1x, 2x), so that downscaling does not suck too much, start with least downscale factor
				// 4. if unable, pick low-res
				// 5. is unable, pick highest density we can draw
				// 6. then in that density, pick the higest color depth
				//this works since our search was already filtered for what we CAN draw. This doe snot account for some device being able to do,
				// for example, hi-res in greyscale but low res in color (due to a deficit of vram). No such device exists, and also PalmOS would
				// have no way to tell us this info anyways.
				// alsoif any card disappear during this process, we'll try to find files no longer present and end up with no images. This will
				// not crash and is deemed acceptable


				//find our search order
				for (i = 0; i < sizeof(searches) / sizeof(*searches); i++) {
					if (searches[i].densityMask == bestDensity)
						searchOrder = searches[i].searchOrder;
				}

				//follow it until we find a non-empty result set
				for (i = 0; searchOrder[i] && !file; i++) {
					for (j = 0; j < sizeof(searches) / sizeof(*searches) && !file; j++) {

						if (searches[j].densityMask != searchOrder[i])
							continue;

						for (k = searchDepthMax; k && !file; k >>= 1) {
							if (searches[j].foundDepths & (1UL << k)) {		//if we had seen this combo of depth and density, go find it...
								for (vri = 0; !file && vri != vfsIteratorStop && errNone == VFSVolumeEnumerate(&vrn, &vri);)
									file = pokeImgPrvOpenVfsDataset(vrn, fileName, searches[j].densityName, k);
							}
						}
					}
				}

				if (file) {
					src->data.vfs.file = file;
					src->typ = ImageSourceTypeVfs;
					return;
				}
			}

		#endif

		//we got this far - there is no image. record this to not try again
		src->typ = ImageSourceTypeNone;
	}
}

static void pokeImgPrvSourceClose(struct ImageSource *src)
{
	switch (src->typ) {
		case ImageSourceTypeNotYetSet:
		case ImageSourceTypeNone:
		default:
			break;

		case ImageSourceTypeInternal:
			DmCloseDatabase(src->data.internal.db);
			src->data.internal.db = NULL;
			break;

		case ImageSourceTypeVfs:
			VFSFileClose(src->data.vfs.file);
			src->data.vfs.file = 0;
			break;
	}
	src->typ = ImageSourceTypeNotYetSet;
}

static MemHandle pokeImgPrvGetImgData(UInt16 pokeID, enum PokeImgSize sz, Boolean *wasAllocatedP)
{
	struct PokeImgMgrGlobals *g = getGlobals();
	struct ImageSource *src;
	MemHandle mh;
	UInt16 idx;

	switch (sz) {
		case PokeImgIcon:
			src = &g->srcIcons;
			break;

		case PokeImgSprite:
			src = &g->srcSprites;
			break;

		case PokeImgItemIcon:
			if (!g->itemDb)
				g->itemDb = DmOpenDatabaseByTypeCreator(ITEM_RESOURCE_DB, appFileCreator, dmModeReadOnly);
			if (!g->itemDb)
				return NULL;

			idx = DmFindResource(g->itemDb, ITEM_RESOURCE_TYPE, pokeID, NULL);
			if (idx == 0xffffu)
				return NULL;

			*wasAllocatedP = false;
			return DmGetResourceIndex(g->itemDb, idx);

		default:
			return NULL;
	}

	pokeImgPrvSourceRefresh(src);

	switch (src->typ) {
		case ImageSourceTypeInternal:

			idx = DmFindResource(src->data.internal.db, src->resType, pokeID, NULL);
			if (idx == 0xffffu)
				return NULL;

			*wasAllocatedP = false;
			return DmGetResourceIndex(src->data.internal.db, idx);

		case ImageSourceTypeVfs:

			if (errNone == VFSFileDBGetResource(src->data.vfs.file, src->resType, pokeID, &mh)) {
				*wasAllocatedP = true;
				return mh;
			}
			return NULL;

		default:
			return NULL;
	}

	return NULL;
}

void pokeImgMgrCleanup(void)
{
	struct PokeImgMgrGlobals *g = getGlobals();

	if (g->vfsCapable) {

		UInt16 cardNo;
		LocalID LID;

		if (errNone == SysCurAppDatabase(&cardNo, &LID))
			(void)SysNotifyUnregister(cardNo, LID, sysNotifyVolumeUnmountedEvent, sysNotifyNormalPriority);

		g->vfsCapable = false;	//in case there is one in flight
	}

	if (g->curSprite) {
		imgDrawStateFree(g->curSprite);
		g->curSprite = NULL;
	}

	pokeImgPrvSourceClose(&g->srcSprites);
	pokeImgPrvSourceClose(&g->srcIcons);

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
	Boolean wasAllocated;
	MemHandle mh;

	mh = pokeImgPrvGetImgData(pokeID, sz, &wasAllocated);
	if (mh) {

		if (!imgDecode(&ds, MemHandleLock(mh), MemHandleSize(mh), expectedDim, expectedDim, forcedDepth))
			ds = NULL;

		if (wasAllocated) {

			MemHandleFree(mh);
		}
		else {

			MemHandleUnlock(mh);
			DmReleaseResource(mh);
		}
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
