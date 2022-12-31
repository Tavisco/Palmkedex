#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS
#include "osPatches.h"
#include "Palmkedex.h"
#include <SonyCLIE.h>
#include <PalmOS.h>


struct DecompressState {		//we can use this as we please
	UInt8 available[8];
};

struct OsPatchState {
	void (*oldTrapWinDrawBitmap)(BitmapPtr bitmapP, Coord x, Coord y);
	Err (*oldTrapBltCopyRectangle)(const void* dstState, const BitmapType* srcBmp, RectangleType* clippedDstRect, Int16 srcX, Int16 srcY);
	Int32 (*oldTrapScrDecompress)(BitmapCompressionType comprTyp, const UInt8 *src, UInt32 srcLen, UInt8 *dst, UInt32 dstLen, struct DecompressState *dcs);
	UInt8 curPackbitsDecompressDepth;
};

static struct OsPatchState* osPatchesGetState(Boolean allocIfNone)
{
	struct OsPatchState *ret;

	if (errNone != FtrGet(appFileCreator, ftrOsPatchState, (UInt32*)&ret)) {

		if (allocIfNone) {

			ret = MemPtrNew(sizeof(struct OsPatchState));
			if (ret) {

				MemSet(ret, sizeof(struct OsPatchState), 0);
				FtrSet(appFileCreator, ftrOsPatchState, (UInt32)ret);
			}
		}
		else
			ret = NULL;
	}

	return ret;
}

static Err osPatchesBltCopyRectangle(const void* dstState, const BitmapType* srcBmp, RectangleType* clippedDstRect, Int16 srcX, Int16 srcY)
{
	struct OsPatchState *osps = osPatchesGetState(false);
	BitmapPtrV2 srcBmpV2 = (BitmapPtrV2)srcBmp;

	//it would see that we have no rason to patch this function - our beef, after all, if only with ScrDecompress()
	//but in OS4.0, ScrDecompress dispatches to ScrDecompress8 or ScrDecompress16 based on passed-in state, where the
	//first byte has the depth. In PalmOS 3.5 nobody will do this for us, so we are forced to track down all callers
	//of ScrDecompress() and patch them and record the depth of the source material. Luckily there is only one caller:
	//BltCopyRectangle(). Thus this patch :)

	if (srcBmp && srcBmp->flags.compressed && srcBmp->version == 2 && srcBmpV2->compressionType == BitmapCompressionTypePackBits) {

		UInt8 prevDecompressType = osps->curPackbitsDecompressDepth;
		Err ret;

		osps->curPackbitsDecompressDepth = srcBmp->pixelSize;
		ret = osps->oldTrapBltCopyRectangle(dstState, srcBmp, clippedDstRect, srcX, srcY);
		osps->curPackbitsDecompressDepth = prevDecompressType;

		return ret;
	}
	else
		return osps->oldTrapBltCopyRectangle(dstState, srcBmp, clippedDstRect, srcX, srcY);
}

static Int32 osPatchesScrDecompressPackBits8(const UInt8 *src, UInt8 *dst, UInt32 dstLen)
{
	const UInt8 *srcInitial = src;
	UInt8 *dstEnd = dst + dstLen;

	while (dst < dstEnd) {

		UInt8 action = *src++;

		if (action < 128) {

			action++;
			if (dstEnd - dst < action)
				return -1;
			while (action--)
				*dst++ = *src++;
		}
		else {

			UInt8 val = *src++;

			action = 1 - action;

			if (dstEnd - dst < action)
				return -1;

			while (action--)
				*dst++ = val;
		}
	}
	return src - srcInitial;
}


static Int32 osPatchesScrDecompressPackBits16(const UInt8 *src, UInt16 *dst, UInt32 dstLen)
{
	const UInt8 *srcInitial = src;
	UInt16 *dstEnd = dst + dstLen;

	while (dst < dstEnd) {

		UInt8 action = *src++;

		if (action < 128) {

			action++;
			if (dstEnd - dst < action)
				return -1;
			while (action--) {
				UInt16 val = *src++;
				val = (val << 8) + *src++;

				*dst++ = val;
			}
		}
		else {

			UInt16 val = val = *src++;
			val = (val << 8) + *src++;

			action = 1 - action;

			if (dstEnd - dst < action)
				return -1;

			while (action--)
				*dst++ = val;
		}
	}
	return src - srcInitial;
}


static Int32 osPatchesScrDecompress(BitmapCompressionType comprTyp, const UInt8 *src, UInt32 srcLen, UInt8 *dst, UInt32 dstLen, struct DecompressState *dcs)
{
	struct OsPatchState *osps = osPatchesGetState(false);

	if (comprTyp != BitmapCompressionTypePackBits)
		return osps->oldTrapScrDecompress(comprTyp, src, srcLen, dst, dstLen, dcs);
	else if (osps->curPackbitsDecompressDepth == 16)
		return osPatchesScrDecompressPackBits16(src, (UInt16*)dst, dstLen / sizeof(UInt16));
	else
		return osPatchesScrDecompressPackBits8(src, dst, dstLen);
}

static void osPatchesWinDrawBitmap(BitmapPtr bitmapP, Coord x, Coord y)
{
	UInt32 curDepth;
	UInt16 hrLibRef;

	if (errNone == SysLibFind(sonySysLibNameHR, &hrLibRef) && hrLibRef != 0xffff && errNone == WinScreenMode(winScreenModeGet, NULL, NULL, &curDepth, NULL)) {

		BitmapPtr bestHr = NULL;
		UInt8 bestLrDepth = 0, bestHrDepth = 0;
		BitmapPtr cur = bitmapP;
		UInt32 romVersion;

		while (cur) {

			UInt32 nextOffset = 0;

			if ((cur->version != 1 || cur->pixelSize <= 8) && cur->version <= 3) {		//we do not consider 16bpp images - this is a simple hack for a specific purpose

				UInt16 density = cur->version == 3 ? ((BitmapPtrV3)cur)->density : kDensityLow;
				UInt8 depth = cur->pixelSize;

				if (density == kDensityDouble && depth <= curDepth && depth > bestHrDepth) {
					bestHrDepth = depth;
					bestHr = cur;
				}
				else if (density == kDensityLow && depth <= curDepth && depth > bestLrDepth) {

					bestLrDepth = depth;
				}
			}

			switch (cur->version) {
				case 0:
					nextOffset = 0;
					break;

				case 1:
					//special: hi-res separator
					if (cur->pixelSize == 0xff)
						nextOffset = sizeof(BitmapTypeV1);
					else
						nextOffset = 4UL * ((BitmapPtrV1)cur)->nextDepthOffset;
					break;

				case 2:
					nextOffset = 4UL * ((BitmapPtrV2)cur)->nextDepthOffset;
					break;

				case 3:
					nextOffset = ((BitmapPtrV3)cur)->nextBitmapOffset;
					break;
			}
			cur = nextOffset ? (BitmapPtr)(((char*)cur) + nextOffset) : NULL;
		}
		//we prefer greater depth over higher resolution
		//we must also verify it has no features we cannot easily represent in an V2 bitmap
		if (bestHr && bestHrDepth >= bestLrDepth && !bestHr->flags.hasColorTable) {

			BitmapPtrV3 srcV3 = (BitmapPtrV3)bestHr;
			Boolean isCompressed = false;
			UInt8 compressionType = 0;
			BitmapPtr src = bestHr;
			UInt32 bitsSize = 0;
			char *bits = NULL;
			char *pastHeader;

			srcV3 = (BitmapPtrV3)bestHr;
			pastHeader = ((char*)srcV3) + srcV3->size;
			isCompressed = srcV3->flags.compressed;
			compressionType = srcV3->compressionType;
			bits = srcV3->flags.indirect ? *(char**)pastHeader : pastHeader;
			if (isCompressed) {
				bitsSize = *(UInt32*)bits - sizeof(UInt32);
				bits += sizeof(UInt32);
			}
			else {

				bitsSize = srcV3->rowBytes * srcV3->height;
			}

			//verify the compressed size can be represented in a V2 bitmap
			if (!isCompressed || bitsSize <= 0xFFFD) {

				//we must assemble a new bitmap in RAM since compressed bitmaps include their own size and we are not sure we can write over it
				BitmapPtrV2 hdr = MemPtrNew(sizeof(BitmapTypeV2) + (isCompressed ? sizeof(UInt16) : 0) + bitsSize);

				if (hdr) {

					char *pastHeader = (char*)(hdr + 1);

					MemSet(hdr, sizeof(BitmapTypeV2), 0);
					hdr->width = src->width;
					hdr->height = src->height;
					hdr->rowBytes = src->rowBytes;
					hdr->flags.compressed = isCompressed;
					hdr->pixelSize = src->pixelSize;
					hdr->version = 2;
					hdr->compressionType = compressionType;

					if (isCompressed) {

						*(UInt16*)pastHeader = bitsSize + sizeof(UInt16);
						pastHeader += sizeof(UInt16);
					}
					MemMove(pastHeader, bits, bitsSize);

					HRWinDrawBitmap(hrLibRef, (BitmapPtr)hdr, x * 2, y * 2);
					MemPtrFree(hdr);

					return;
				}
			}
		}
	}
	osPatchesGetState(false)->oldTrapWinDrawBitmap(bitmapP, x, y);
}

void osPatchesInstall(void)
{
	UInt32 romVersion;
	UInt16 hrLibRef;

	if (errNone != FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion))
		return;

	//PackBits compression only needs to be added for OS < 4.0 and OS >= 3.5 (it is only usd for Bitmaps v2+ images which is new in PalmOS 3.5)
	if (romVersion >= sysMakeROMVersion(3,5,0,sysROMStageDevelopment,0) && romVersion < sysMakeROMVersion(4,0,0,sysROMStageDevelopment,0)) {

		osPatchesGetState(true)->oldTrapScrDecompress = SysGetTrapAddress(sysTrapScrDecompress);
		osPatchesGetState(true)->oldTrapBltCopyRectangle = SysGetTrapAddress(sysTrapBltCopyRectangle);
		SysSetTrapAddress(sysTrapScrDecompress, osPatchesScrDecompress);
		SysSetTrapAddress(sysTrapBltCopyRectangle, osPatchesBltCopyRectangle);
	}

	//"drawing hi-res images on sony devices patch" is only needed for when HRLib is found ands OS is < 5.0
	if (romVersion < sysMakeROMVersion(5,0,0,sysROMStageDevelopment,0) && errNone == SysLibFind(sonySysLibNameHR, &hrLibRef) && hrLibRef != 0xffff) {

		osPatchesGetState(true)->oldTrapWinDrawBitmap = SysGetTrapAddress(sysTrapWinDrawBitmap);
		SysSetTrapAddress(sysTrapWinDrawBitmap, osPatchesWinDrawBitmap);
	}
}

void osPatchesRemove(void)
{
	struct OsPatchState *osps = osPatchesGetState(false);

	if (!osps)
		return;

	if (osps->oldTrapScrDecompress)
		SysSetTrapAddress(sysTrapScrDecompress, osps->oldTrapScrDecompress);

	if (osps->oldTrapBltCopyRectangle)
		SysSetTrapAddress(sysTrapBltCopyRectangle, osps->oldTrapBltCopyRectangle);

	if (osps->oldTrapWinDrawBitmap)
		SysSetTrapAddress(sysTrapWinDrawBitmap, osps->oldTrapWinDrawBitmap);

	MemPtrFree(osps);
	FtrUnregister(appFileCreator, ftrOsPatchState);
}