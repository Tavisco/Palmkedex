#include "BUILD_TYPE.h"

#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS
#include <PalmOS.h>
#include "imgDrawInt.h"
#include "Palmkedex.h"
#include "osPatches.h"
#include "imgDraw.h"
#include "osExtra.h"
#ifdef HANDERA_SUPPORT
#include "myTrg.h"
#endif
#ifdef SONY_HIRES_SUPPORT
#include <SonyCLIE.h>
#endif
#ifdef ARM_PROCESSOR_SUPPORT
#include <PceNativeCall.h>
#endif

#include "glue.h"


#define BLITTER_CAN_DRAW_1x					0x01	//x is compares to standard not compared to screen itself
#define BLITTER_CAN_DRAW_1_5x				0x02
#define BLITTER_CAN_DRAW_2x					0x04
#define BLITTER_CAN_DRAW_3x					0x08
#define BLITTER_CAN_DRAW_4x					0x10


static UInt8 getSupportedBitmapDensities(void)
{
	if (isHighDensitySupported()) {

		UInt16 density = 0;
		UInt8 ret = 0;

		while (errNone == WinGetSupportedDensity(&density) && density) switch (density) {

			case kDensityLow:			ret |= BLITTER_CAN_DRAW_1x;		break;
			case kDensityOneAndAHalf:	ret |= BLITTER_CAN_DRAW_1_5x;	break;
			case kDensityDouble:		ret |= BLITTER_CAN_DRAW_2x;		break;
			case kDensityTriple:		ret |= BLITTER_CAN_DRAW_3x;		break;
			case kDensityQuadruple:		ret |= BLITTER_CAN_DRAW_4x;		break;
		}

		return ret;
	}
	else if (isSonyHiResSupported()) {

		return BLITTER_CAN_DRAW_1x | BLITTER_CAN_DRAW_2x;
	}
#ifdef HANDERA_SUPPORT
	else if (isHanderaHiRes()) {

		VgaRotateModeType curRot;
		VgaScreenModeType curMod;

		VgaGetScreenMode(&curMod, &curRot);

		if (curMod == screenMode1To1)	//1:1 mode can draw native or magnified
			return BLITTER_CAN_DRAW_1x | BLITTER_CAN_DRAW_1_5x;
		else							//auto-magnified mode can only draw magified
			return BLITTER_CAN_DRAW_1x;
	}
#endif
	else {

		return BLITTER_CAN_DRAW_1x;
	}
}

void imgDrawStateFree(struct DrawState *ds)
{
	if (ds->b) {
		if (ds->depth < 8)
			MemPtrFree(ds->b);	//we allocated it manually - free it so too
		else
			BmpDelete(ds->b);
	}
	if (ds->clut)
		MemPtrFree(ds->clut);
	MemPtrFree(ds);
}

void imgDrawRedraw(struct DrawState *ds, int16_t x, int16_t y)
{
	//if we get here, we KNOW the density of the image is one our blitter(s) can draw, but that does not mean that it is easy or simple
	//special handling is needed for each kind of high-resolution screens

	if (isHighDensitySupported()) {

		if (ds->density == kDensityLow)
			WinDrawBitmap(ds->b, x, y);
		else {

			BitmapPtr b3 = (BitmapPtr)BmpCreateBitmapV3(ds->b, ds->density, ds->bits, NULL);
			if (b3) {

				WinDrawBitmap(b3, x, y);
				BmpDelete(b3);
			}
		}
	}
#ifdef SONY_HIRES_SUPPORT
	else if (isSonyHiResSupported()) {

		if (ds->density == kDensityLow)
			WinDrawBitmap(ds->b, x, y);
		else {	//only 2x possible here

			UInt16 hrLibRef;

			if (errNone == SysLibFind(sonySysLibNameHR, &hrLibRef) && hrLibRef != 0xffff)
				HRWinDrawBitmap(hrLibRef, ds->b, x * 2, y * 2);
		}
	}
#endif
#ifdef HANDERA_SUPPORT
	else if (isHanderaHiRes()) {

		VgaRotateModeType curRot;
		VgaScreenModeType curMod;

		VgaGetScreenMode(&curMod, &curRot);

		if (curMod == screenMode1To1) {

			osPatchesDrawingInterceptionStateSet(false);
			if (ds->density == kDensityLow) {

				VgaWinDrawBitmapExpanded(ds->b, x, y);
			}
			else {

				WinDrawBitmap(ds->b, x, y);
			}
			osPatchesDrawingInterceptionStateSet(true);
		}
		else {

			WinDrawBitmap(ds->b, x, y);
		}
	}
#endif
	else {

		WinDrawBitmap(ds->b, x, y);
	}
}

static unsigned char imgDrawHdrCbk(struct DrawState *ds, uint32_t w, uint32_t h, struct ColortableEntry *colors, uint16_t numColors, unsigned char isGreyscale)
{
	UInt32 curDepth, romVersion;
	Boolean colorSupport;
	Err err;

	ds->actualH = h;

#ifdef MORE_THAN_1BPP_SUPPORT
	if (errNone != FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion) || romVersion < sysMakeROMVersion(3,0,0,sysROMStageDevelopment,0)) {

		colorSupport = false;
		curDepth = 1;
	}
	else if (errNone != WinScreenMode(winScreenModeGet, NULL, NULL, &curDepth, &colorSupport)) {

		colorSupport = false;
	}
#else
	curDepth = 1;
	colorSupport = false;
#endif

	//honour requested depth
	if (ds->depth)
		curDepth = ds->depth;

	//check for nonzero exact integer or 1/2 multiple of size, same for W & H
	if (!w || !h || w * 2 % ds->expectedW || h * 2 % ds->expectedW || w * 2 / ds->expectedW != h * 2 / ds->expectedW)
		return false;

	//see WHICH multiple it is, along the way, verify we support & expect that density
	switch (w * 2 / ds->expectedW) {
		case 2:	//expected size
			if (!(ds->blitterDensitySupportBits & BLITTER_CAN_DRAW_1x))
				return false;
			ds->density = kDensityLow;
			break;

		case 3:	//1.5 the size
			if (!(ds->blitterDensitySupportBits & BLITTER_CAN_DRAW_1_5x))
				return false;
			ds->density = kDensityOneAndAHalf;
			break;

		case 4:	//2x the size
			if (!(ds->blitterDensitySupportBits & BLITTER_CAN_DRAW_2x))
				return false;
			ds->density = kDensityDouble;
			break;

		case 6:	//3x the density
			if (!(ds->blitterDensitySupportBits & BLITTER_CAN_DRAW_3x))
				return false;
			ds->density = kDensityTriple;
			break;

		case 8:	//4x the density
			if (!(ds->blitterDensitySupportBits & BLITTER_CAN_DRAW_4x))
				return false;
			ds->density = kDensityQuadruple;
			break;

		default:
			return false;
	}

	/*
		We need to handle the incoming color table info we do so by setting the system palette as needed
		and adjusting the indices as well. They come in already sorted from most to least common.

		Palm OS Programmer's Companion, Volume I, "Color and Grayscale Support" quoth:
			216 color "Web-safe" palette, which includes all
			combinations of red, green, and blue at these levels: 0x00,
			0x33, 0x66, 0x99, 0xCC, and 0xFF. Also, it includes all 16 gray
			shades at these levels: 0x00, 0x11, 0x22, ... 0xFF. Finally, it
			includes these extra named HTML colors: 0xC0C0C0 (silver),
			0x808080 (gray), 0x800000 (maroon), 0x800080 (purple),
			0x008000 (green), and 0x008080 (teal). The remaining 24
			entries (indexes 0xE7 through 0xFE) are unspecified and
			filled with black. These entries may be defined by an
			application.

		On greyscale devices we do not bother with screen colortables at all, we convert all color entries
		to proper grey, and then later re-pack the image pixels to proper depth
	*/
	if (colorSupport) {

		if (curDepth == 16) {	//emit 16bpp image

			UInt16 i;

			//alloc the 16bpp CLUT
			ds->clut = MemPtrNew(numColors * sizeof(UInt16));
			if (!ds->clut)
				return false;

			//calculate 16bpp values
			for (i = 0; i < numColors; i++)
				ds->clut[i] = (((UInt16)colors[i].r & 0xf8) << 8) + (((UInt16)colors[i].g & 0xfc) << 3) + (colors[i].b >> 3);
		}
		else {					//emit 8bpp image

			UInt16 i, j, palSize = 256, nextFreeColor = 0xE7, lastFreeColor = 0xFE;
			struct RGBColorType *clut;

			if (curDepth != 8) {

				ErrAlertCustom(0, "Your device appears to support color, but 8bit depth is not supported.", NULL, NULL);
				return false;
			}

			clut = MemPtrNew(sizeof(RGBColorType) * palSize);
			if (!clut) {
				ErrAlertCustom(0, "alloc fail", 0, 0);
				return false;
			}
			//set to default and get it
			if (errNone != WinPalette(winPaletteSetToDefault, 0, palSize, NULL) || errNone != WinPalette(winPaletteGet, 0, palSize, clut)) {
				ErrAlertCustom(0, "palette fail", 0, 0);
				MemPtrFree(clut);
				return false;
			}

			//use user clut entries (0xE7..0xFE) and then pass 0 for the rest, do not bother finding best possible match
			for (i = 0; i < numColors; i++) {
				if (nextFreeColor <= lastFreeColor) { 		//else if there is space, add it

					clut[nextFreeColor].r = colors[i].r;
					clut[nextFreeColor].g = colors[i].g;
					clut[nextFreeColor].b = colors[i].b;
					colors[i].index = nextFreeColor++;
				}
				else
					colors[i].index = 0;
			}
			err = WinPalette(winPaletteSet, 0, palSize, clut);
			MemPtrFree(clut);
			if (err != errNone) {
				ErrAlertCustom(0, "palette set fail", 0, 0);
				return false;
			}
		}
		ds->b = BmpCreate(w, h, curDepth, NULL, &err);
		if (!ds->b) {
			ErrAlertCustom(err, "Cannot create bitmap", NULL, NULL);
			return false;
		}
		BmpGlueGetDimensions(ds->b, NULL, NULL, &ds->rowBytes);
		ds->bits = BmpGetBits(ds->b);
		ds->depth = curDepth;
	}
	else {		//our device does not support color and is thus in greyscale mode

		UInt16 i, nScreenColorsM1 = (1 << curDepth) - 1, realStride, virtualStride;
		struct BitmapTypeV1 *bmp1;

		switch (curDepth) {

			case 1:
			case 2:
			case 4:
				break;

			default:
				ErrAlertCustom(0, "Current screen depth is not a supported one", NULL, NULL);
				return false;
		}

		//convert each clut entry to proper shade of grey, save that as the desired index, that way we can then easily pack them
		for (i = 0; i < numColors; i++) {

			UInt16 grey;

			grey = 76 * colors[i].r + 151 * colors[i].g + 29 * colors[i].b;
			grey = (grey + 128) >> 8;
			grey = (grey << curDepth) - grey;		// grey *= (1 << curDepth) - 1
			colors[i].index = nScreenColorsM1 - ((grey + 128) >> 8);
		}

		//for 1bpp, we'll create a V0 bitmap, for all others - a V1. The headers are the same size, and fields overlap (by design)
		//we make a bitmap with enough space for a byte per pixel because our decodedr produces that. We then repack the data and
		//shrink the allocation. To mak erepacking easier, we prefer the stride *in pixels* of both arrays to be the same. This
		//allows the repacking process to process data in convenient chunks
		realStride = ((w * curDepth) + 15) / 16 * 2;
		virtualStride = realStride * 8 / curDepth;
		bmp1 = MemPtrNew(sizeof(struct BitmapTypeV1) + virtualStride * h);	//enough space for 8bpp, will shrink later - our decoder emits 8bpp
		if (!bmp1)
			return false;
		MemSet(bmp1, sizeof(*bmp1), 0);
		bmp1->width = w;
		bmp1->height = h;
		bmp1->rowBytes = ((w * curDepth) + 15) / 16 * 2;

		if (curDepth != 1) {

			bmp1->pixelSize = curDepth;
			bmp1->version = 1;
		}
		ds->b = (BitmapPtr)bmp1;
		ds->bits = (UInt8*)(bmp1 + 1);
		ds->rowBytes = virtualStride;
		ds->depth = curDepth;
	}

	return true;
}

#ifdef ARM_PROCESSOR_SUPPORT
static void* imgDecodePrvGet68kCallFunc(void)
{
	void **slot = globalsSlotPtr(GLOBALS_SLOT_PCE_CALL_FUNC);

	if (!*slot) {
		MemHandle armH;

		*slot = (void*)PceNativeCall((NativeFuncType*)MemHandleLock(armH = DmGetResource('armc', 0)), NULL);
		MemHandleUnlock(armH);
		DmReleaseResource(armH);
	}

	return *slot;
}

static int __attribute__((noinline)) directArmCall(void *func, void *param)
{
	UInt16 call[7];
	volatile UInt32 *pp = (volatile UInt32*)(((((UInt32)call) & 2) ? call + 1 : call));

	pp[0] = 0x4e4fa7ff;
	pp[1] = __builtin_bswap32((UInt32)func);
	pp[2] = __builtin_bswap32((UInt32)param);

	return ((long (*)(void))pp)();
}
#endif

static int imgDecodeCall(struct DrawState *ds, const void *data, uint32_t dataSz)
{
    UInt32 processorType, result, romVersion, companyID;
    int ret;

#ifdef ARM_PROCESSOR_SUPPORT
	if (errNone == FtrGet(sysFileCSystem, sysFtrNumProcessorID, &processorType) && sysFtrNumProcessorIsARM(processorType)) {
		MemHandle armH;

		struct ArmParams p = {
			.ds = ds,
			.data = data,
			.dataSz = dataSz,
			.hdrDecodedF = imgDrawHdrCbk,
			.call68KFuncP = imgDecodePrvGet68kCallFunc(),
		};

		if (errNone == FtrGet (sysFtrCreator, sysFtrNumOEMCompanyID, &companyID) && companyID == 'stap') {
			ret = PceNativeCall(MemHandleLock(armH = DmGetResource('armc', 1)), &p);
		}
		else{
			ret = directArmCall(MemHandleLock(armH = DmGetResource('armc', 1)), &p);
		}
		
		MemHandleUnlock(armH);
		DmReleaseResource(armH);
	}
	else
#endif
		ret = aciDecode(ds, data, dataSz, imgDrawHdrCbk);


	//repack
	if (ret >= 0) {
		if (ds->depth < 8) {

			struct BitmapTypeV1 *bmp1 = (struct BitmapTypeV1*)ds->b;
			aciRepack(ds->bits, bmp1->height * (bmp1->rowBytes * 8 / ds->depth), ds->depth);
			MemPtrResize(ds->b, sizeof(struct BitmapTypeV1) + bmp1->height * bmp1->rowBytes);
		}
		else if (ds->depth == 16) {

			aciClutApply(ds->bits, ds->rowBytes, ds->actualH, ds->clut);
			MemPtrFree(ds->clut);
			ds->clut = NULL;
		}
	}

	return ret;
}

bool imgDecode(struct DrawState **dsP, const void *data, uint32_t dataSz, uint32_t expectedW, uint32_t expectedH, uint8_t decodeAtThisDepth /* 0 for whatever screen is */)
{
	struct DrawState *ds;
	int ret;

	*dsP = NULL;

	ds = (struct DrawState *)MemPtrNew(sizeof(struct DrawState));
	if (!ds)
		return false;
	MemSet(ds, sizeof(*ds), 0);
	ds->expectedW = expectedW;
	ds->expectedH = expectedH;
	ds->blitterDensitySupportBits = getSupportedBitmapDensities();
	ds->depth = decodeAtThisDepth;
	ds->clut = NULL;

	ret = imgDecodeCall(ds, data, dataSz);
	if (ret < 0) {

		imgDrawStateFree(ds);
		return false;
	}

	*dsP = ds;
	return true;
}

const void* imgGetBits(struct DrawState *ds)
{
	return ds->bits;
}
