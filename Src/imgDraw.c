#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS
#include <PalmOS.h>
#include <PceNativeCall.h>
#include <SonyCLIE.h>
#include "imgDrawInt.h"
#include "imgDraw.h"
#include "osExtra.h"
#include "glue.h"



#define PNG_HI_RES_SUPPORTED				1		//sonyHR only supports double
#define PNG_VARIOUS_DENSITIES_SUPPORTED		2		//palmHR supports various



static Boolean isHighDensitySupported(void)
{
	UInt32 version;

	return errNone == FtrGet(sysFtrCreator, sysFtrNumWinVersion, &version) && version >= 4;
}

static Boolean isSonyHiResSupported(void)
{
	UInt16 hrLibRef;

	return errNone == SysLibFind(sonySysLibNameHR, &hrLibRef) && hrLibRef != 0xffff;
}

void imgDrawStateFree(struct DrawState *ds)
{
	if (ds->b) {
		if (ds->depth < 8)
			MemPtrFree(ds->b);	//we allocated it manually - free it so too
		else
			BmpDelete(ds->b);
	}
	MemPtrFree(ds);
}

void imgDrawRedraw(struct DrawState *ds, int16_t x, int16_t y)
{
	if (ds->density == kDensityLow) {

		WinDrawBitmap(ds->b, x, y);
	}
	else if (ds->densitySupportFlags & PNG_VARIOUS_DENSITIES_SUPPORTED) {	//high density feature set is easier to deal with  - use that

		BitmapPtr b3 = (BitmapPtr)BmpCreateBitmapV3(ds->b, ds->density, ds->bits, NULL);
		if (b3) {

			WinDrawBitmap(b3, x, y);
			BmpDelete(b3);
		}
	}
	else if (ds->density == kDensityDouble && (ds->densitySupportFlags & PNG_HI_RES_SUPPORTED)) {

		UInt16 hrLibRef;

		if (errNone == SysLibFind(sonySysLibNameHR, &hrLibRef) && hrLibRef != 0xffff) {

			/*
				Some Sony CLIE devices with PalmOS 4 have a bug when drawing a high-res image with
				its own palette unto the screen. They will swap every two columns with each other,
				creating a horizontal blinds-like effect. This is indeed a bug in the OS. To verify,
				simply try drawing the image in non-high-res-mode or not having a colortable
				attached to the image. The workaround is to not use such an image. TBD
			*/

			HRWinDrawBitmap(hrLibRef, ds->b, x * 2, y * 2);
		}
	}
}

static unsigned char imgDrawHdrCbk(struct DrawState *ds, uint32_t w, uint32_t h, struct ColortableEntry *colors, uint16_t numColors, unsigned char isGreyscale)
{
	UInt32 curDepth, romVersion;
	Boolean colorSupport;
	Err err;

	if (errNone != FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion) || romVersion < sysMakeROMVersion(3,0,0,sysROMStageDevelopment,0)) {

		colorSupport = false;
		curDepth = 1;
	}
	else if (errNone != WinScreenMode(winScreenModeGet, NULL, NULL, &curDepth, &colorSupport)) {

		colorSupport = false;
	}

	//honour requested depth
	if (ds->depth)
		curDepth = ds->depth;

	//check for nonzero exact integer or 1/2 multiple of size, same for W & H
	if (!w || !h || w * 2 % ds->expectedW || h * 2 % ds->expectedW || w * 2 / ds->expectedW != h * 2 / ds->expectedW)
		return false;

	//see WHICH multiple it is, along the way, verify we support & expect that density
	switch (w * 2 / ds->expectedW) {
		case 2:	//expected size
			ds->density = kDensityLow;
			break;

		case 3:	//1.5 the size
			ds->density = kDensityOneAndAHalf;
			if (!(ds->densitySupportFlags & PNG_VARIOUS_DENSITIES_SUPPORTED))
				return false;
			break;

		case 4:	//2x the size
			ds->density = kDensityDouble;
			if (!(ds->densitySupportFlags & (PNG_VARIOUS_DENSITIES_SUPPORTED | PNG_HI_RES_SUPPORTED)))
				return false;
			break;

		case 6:	//3x the density
			ds->density = kDensityTriple;
			if (!(ds->densitySupportFlags & PNG_VARIOUS_DENSITIES_SUPPORTED))
				return false;
			break;

		case 8:	//4x the density
			ds->density = kDensityQuadruple;
			if (!(ds->densitySupportFlags & PNG_VARIOUS_DENSITIES_SUPPORTED))
				return false;
			break;

		default:
			return false;
	}

	/*
		We need to handle the incoming color table info we do so by setting the system palette as needed
		and adjusting the indices as well. They come in already sorted from most to least common.

		Palm OS Programmer�s Companion, Volume I, "Color and Grayscale Support" quoth:
			216 color �Web-safe� palette, which includes all
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

		UInt16 i, j, palSize = 256, nextFreeColor = 0xE7, lastFreeColor = 0xFE;
		struct RGBColorType *clut;

		if (curDepth != 8) {

			ErrAlertCustom(0, "Current screen depth is not a supported one", NULL, NULL);
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
		ds->b = BmpCreate(w, h, 8, NULL, &err);
		if (!ds->b) {
			ErrAlertCustom(err, "Cannot create bitmap", NULL, NULL);
			return false;
		}
		BmpGlueGetDimensions(ds->b, NULL, NULL, &ds->rowBytes);
		ds->bits = BmpGetBits(ds->b);
		ds->depth = 8;
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

static int imgDecodeCall(struct DrawState *ds, const void *data, uint32_t dataSz)
{
	UInt32 processorType, result, romVersion;
	int ret;

	if (errNone == FtrGet(sysFileCSystem, sysFtrNumProcessorID, &processorType)	&& sysFtrNumProcessorIsARM(processorType)) {

		MemHandle armH;

		struct ArmParams p = {
			.ds = ds,
			.data = data,
			.dataSz = dataSz,
			.hdrDecodedF = imgDrawHdrCbk,
		};

		ret = PceNativeCall((NativeFuncType*)MemHandleLock(armH = DmGetResource('armc', 1)), &p);
		MemHandleUnlock(armH);
		DmReleaseResource(armH);
	}
	else {

		ret = aciDecode(ds, data, dataSz, imgDrawHdrCbk);
	}

	//repack
	if (ret >= 0 && ds->depth < 8) {

		struct BitmapTypeV1 *bmp1 = (struct BitmapTypeV1*)ds->b;
		aciRepack(ds->bits, bmp1->height * (bmp1->rowBytes * 8 / ds->depth), ds->depth);
		MemPtrResize(ds->b, sizeof(struct BitmapTypeV1) + bmp1->height * bmp1->rowBytes);
	}

	return ret;
}

bool imgDecode(struct DrawState **dsP, const void *data, uint32_t dataSz, uint32_t expectedW, uint32_t expectedH, uint8_t decodeAtThisDepth /* 0 for whatever screen is */)
{
	uint8_t densitySupportFlags = 0;
	struct DrawState *ds;
	int ret;

	*dsP = NULL;

	if (isHighDensitySupported())
		densitySupportFlags |= PNG_VARIOUS_DENSITIES_SUPPORTED;
	if (isSonyHiResSupported())
		densitySupportFlags |= PNG_HI_RES_SUPPORTED;

	ds = (struct DrawState *)MemPtrNew(sizeof(struct DrawState));
	if (!ds)
		return false;
	MemSet(ds, sizeof(*ds), 0);
	ds->expectedW = expectedW;
	ds->expectedH = expectedH;
	ds->densitySupportFlags = densitySupportFlags;
	ds->depth = decodeAtThisDepth;

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
