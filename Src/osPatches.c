#include "BUILD_TYPE.h"

#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS
#include "osPatches.h"
#include "Palmkedex.h"
#include <SonyCLIE.h>
#include <PalmOS.h>
#include "myTrg.h"
#include "glue.h"


struct DecompressState {		//we can use this as we please
	UInt8 available[8];
};

struct OsPatchState {
	void (*oldTrapWinDrawBitmap)(BitmapPtr bitmapP, Coord x, Coord y);
	Err (*oldTrapBltCopyRectangle)(const void* dstState, const BitmapType* srcBmp, RectangleType* clippedDstRect, Int16 srcX, Int16 srcY);
	Int32 (*oldTrapScrDecompress)(BitmapCompressionType comprTyp, const UInt8 *src, UInt32 srcLen, UInt8 *dst, UInt32 dstLen, struct DecompressState *dcs);
	UInt8 curPackbitsDecompressDepth;
	Boolean inWinDrawBitmapPatch;		//to avoid circular calls
	UInt8 winDrawBitmapPatchesDisabled;
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

#ifdef MORE_THAN_1BPP_SUPPORT	//packbits not used on 1bpp v1 bitmaps
	
	static Err osPatchesBltCopyRectangle(const void* dstState, const BitmapType* srcBmp, RectangleType* clippedDstRect, Int16 srcX, Int16 srcY)
	{
		struct OsPatchState *osps = osPatchesGetState(false);
		BitmapPtrV2 srcBmpV2 = (BitmapPtrV2)srcBmp;
		
		//it would see that we have no reason to patch this function - our beef, after all, if only with ScrDecompress()
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
#endif

static BitmapPtr osPatchesPrvV3bmpToV2bmp(const BitmapPtr src)
{
	const BitmapPtrV3 srcV3 = (BitmapPtrV3)src;
	const ColorTableType *srcClut = NULL;
	Boolean isCompressed = false;
	UInt8 compressionType = 0;
	UInt16 clutSize = 0;
	UInt32 bitsSize = 0;
	char *bits = NULL;
	char *pastHeader;
	
	//sanity
	if (src->pixelSize > 8 && src->pixelSize != 16)
		return NULL;
	
	pastHeader = ((char*)srcV3) + srcV3->size;
	isCompressed = srcV3->flags.compressed;
	compressionType = srcV3->compressionType;
	
	if (srcV3->flags.hasColorTable) {
		
		if (srcV3->flags.indirectColorTable) {
			
			srcClut = *(const ColorTableType**)pastHeader;
			pastHeader += sizeof(ColorTableType*);
		}
		else {
			
			srcClut = (const ColorTableType*)pastHeader;
			pastHeader += sizeof(ColorTableType) + srcClut->numEntries * sizeof(RGBColorType);
}
		clutSize = sizeof(ColorTableType) + srcClut->numEntries * sizeof(RGBColorType);
	}
	if (srcV3->flags.indirect)
		bits = *(char**)pastHeader;
	else
		bits = pastHeader;
	
	if (isCompressed) {
		
		bitsSize = *(UInt32*)bits - sizeof(UInt32);
		bits += sizeof(UInt32);
	}
	else {
		
		bitsSize = srcV3->rowBytes * srcV3->height;
	}
	
	//verify the compressed size can be represented in a V2 bitmap
	if (!isCompressed || bitsSize <= 0xFFFD) {
		
		UInt32 size = sizeof(BitmapTypeV2) + bitsSize;
		BitmapDirectInfoType *dci = NULL;
		ColorTableType *clut = NULL;
		BitmapPtrV2 hdr;
		
		
		if (isCompressed)
			size += sizeof(UInt16);	//compressed size
		
		if (srcClut)
			size += clutSize;
		
		if (src->pixelSize > 8)
			size += sizeof(BitmapDirectInfoType);
		
		hdr = MemPtrNew(size);
		
		if (hdr) {
			
			char *pastHeader = (char*)(hdr + 1);
			
			//sort out where everything will be
			if (srcClut) {
				
				clut = (ColorTableType*)pastHeader;
				pastHeader += clutSize;
			}
			
			if (src->pixelSize == 16) {
				
				dci = (BitmapDirectInfoType*)pastHeader;
				pastHeader += sizeof(BitmapDirectInfoType);
			}
			
			if (isCompressed) {
				
				*(UInt16*)pastHeader = bitsSize + sizeof(UInt16);
				pastHeader += sizeof(UInt16);
			}

			MemSet(hdr, sizeof(BitmapTypeV2), 0);
			hdr->width = src->width;
			hdr->height = src->height;
			hdr->rowBytes = src->rowBytes;
			hdr->flags.compressed = isCompressed;
			hdr->flags.hasColorTable = !!srcClut;
			hdr->flags.hasTransparency = src->flags.hasTransparency;
			hdr->flags.directColor = src->pixelSize == 16;
			hdr->pixelSize = src->pixelSize;
			hdr->version = 2;
			hdr->compressionType = compressionType;
			
			if (clut)
				MemMove(clut, srcClut, clutSize);
			
			if (dci) {
				
				MemSet(dci, sizeof(dci), 0);
				
				dci->redBits = 5;
				dci->greenBits = 6;
dci->blueBits = 5;
				
				if (hdr->flags.hasTransparency) {
					
					dci->transparentColor.r = (UInt8)(((UInt8)srcV3->transparentValue) << 3);
					dci->transparentColor.g = (UInt8)(((UInt16)srcV3->transparentValue) >> 3) & 0xfc;
					dci->transparentColor.b = (UInt8)(((UInt16)srcV3->transparentValue) >> 8) & 0xf8;
				}
			}
			else if (hdr->flags.hasTransparency) {
				
				hdr->transparentIndex = srcV3->transparentValue;
			}
			
			MemMove(pastHeader, bits, bitsSize);
			
			return (BitmapPtr)hdr;
		}
	}
	
	return NULL;
}

#ifdef HANDERA_SUPPORT
	static void osPatchesWinDrawBitmapHandera(BitmapPtr bitmapP, Coord x, Coord y)
	{
		struct OsPatchState *osps = osPatchesGetState(false);
		Boolean osSupports16bppImages;
		UInt32 curDepth, romVersion;
		VgaRotateModeType curRot;
		VgaScreenModeType curMod;
		
		VgaGetScreenMode(&curMod, &curRot);
		if (!osps->winDrawBitmapPatchesDisabled && curMod == screenMode1To1) {
			
			//this is wrong on the Visor Prism,but this patch is not enabled there...
			osSupports16bppImages = errNone == FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion) && romVersion >= sysMakeROMVersion(4,0,0,sysROMStageRelease,0);
			
			if (!osps->inWinDrawBitmapPatch && errNone == WinScreenMode(winScreenModeGet, NULL, NULL, &curDepth, NULL)) {
				
				BitmapPtr bestHr = NULL;
				UInt8 bestHrDepth = 0;
				BitmapPtr cur;
				
				osps->inWinDrawBitmapPatch = true;
				
				for (cur = bitmapP; cur; cur = BmpGlueGetNextBitmapAnyDensity(cur)) {
					
					if (cur->version <= 3) {
						
						UInt16 density = cur->version == 3 ? ((BitmapPtrV3)cur)->density : kDensityLow;
						UInt8 depth = cur->pixelSize;
						
						if (density == kDensityOneAndAHalf && ((depth <= curDepth && depth > bestHrDepth) || (curDepth == 8 && depth == 16 && bestHrDepth < 8 && osSupports16bppImages))) {
							
							bestHrDepth = depth;
							bestHr = cur;
						}
					}
				}
				if (bestHr) {
					
					BitmapPtr v2 = osPatchesPrvV3bmpToV2bmp(bestHr);
					
					if (v2) {
						
						WinDrawBitmap(v2, x, y);
						MemPtrFree(v2);
						
						osps->inWinDrawBitmapPatch = false;
						
						return;
					}
				}
				
				//do not call with a bitmap ver not supported by the system
				if (bitmapP->version < 3) {
					
					VgaWinDrawBitmapExpanded(bitmapP, x, y);
					osps->inWinDrawBitmapPatch = false;
					
					return;
				}
			}
		}
		
		osps->oldTrapWinDrawBitmap(bitmapP, x, y);
	}
#endif

#ifdef SONY_HIRES_SUPPORT
	static void osPatchesWinDrawBitmapSony(BitmapPtr bitmapP, Coord x, Coord y)
	{
		struct OsPatchState *osps = osPatchesGetState(false);
		Boolean osSupports16bppImages;
		UInt32 curDepth, romVersion;
		UInt16 hrLibRef;
		
		if (!osps->winDrawBitmapPatchesDisabled) {
			//this is wrong on the Visor Prism,but this patch is not enabled there...
			osSupports16bppImages = errNone == FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion) && romVersion >= sysMakeROMVersion(4,0,0,sysROMStageRelease,0);
			
			if (!osps->inWinDrawBitmapPatch && errNone == SysLibFind(sonySysLibNameHR, &hrLibRef) && hrLibRef != 0xffff && errNone == WinScreenMode(winScreenModeGet, NULL, NULL, &curDepth, NULL)) {
				
				BitmapPtr bestHr = NULL;
				UInt8 bestHrDepth = 0;
				BitmapPtr cur;
				
				osps->inWinDrawBitmapPatch = true;
				
				for (cur = bitmapP; cur; cur = BmpGlueGetNextBitmapAnyDensity(cur)) {
					
					if (cur->version <= 3) {
						
						UInt16 density = cur->version == 3 ? ((BitmapPtrV3)cur)->density : kDensityLow;
						UInt8 depth = cur->pixelSize;
						
						if (density == kDensityDouble && ((depth <= curDepth && depth > bestHrDepth) || (curDepth == 8 && depth == 16 && bestHrDepth < 8 && osSupports16bppImages))) {
							
							bestHrDepth = depth;
							bestHr = cur;
						}
					}
				}
				if (bestHr) {
					
					BitmapPtr v2 = osPatchesPrvV3bmpToV2bmp(bestHr);
					
					if (v2) {
						
						HRWinDrawBitmap(hrLibRef, v2, x * 2, y * 2);
						MemPtrFree(v2);
						
						osps->inWinDrawBitmapPatch = false;
						
						return;
					}
				}
				osps->inWinDrawBitmapPatch = false;
			}
		}
		osps->oldTrapWinDrawBitmap(bitmapP, x, y);
	}
#endif

void osPatchesInstall(void)
{
	struct OsPatchState *osps;
	UInt32 romVersion;
	UInt16 hrLibRef;

	if (errNone != FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion))
		return;

	osps = osPatchesGetState(true);
	(void)osps;	//quiet down GCC

#ifdef MORE_THAN_1BPP_SUPPORT
	//PackBits compression only needs to be added for OS < 4.0 and OS >= 3.5 (it is only used for Bitmaps v2+ images which is new in PalmOS 3.5)
	if (romVersion >= sysMakeROMVersion(3,5,0,sysROMStageDevelopment,0) && romVersion < sysMakeROMVersion(4,0,0,sysROMStageDevelopment,0)) {

		osps->oldTrapScrDecompress = SysGetTrapAddress(sysTrapScrDecompress);
		osps->oldTrapBltCopyRectangle = SysGetTrapAddress(sysTrapBltCopyRectangle);
		SysSetTrapAddress(sysTrapScrDecompress, osPatchesScrDecompress);
		SysSetTrapAddress(sysTrapBltCopyRectangle, osPatchesBltCopyRectangle);
	}
#endif

#ifdef SONY_HIRES_SUPPORT
	//"drawing hi-res images on sony devices patch" is only needed for when HRLib is found and OS is < 5.0
	if (romVersion < sysMakeROMVersion(5,0,0,sysROMStageDevelopment,0) && errNone == SysLibFind(sonySysLibNameHR, &hrLibRef) && hrLibRef != 0xffff) {

		osps->oldTrapWinDrawBitmap = SysGetTrapAddress(sysTrapWinDrawBitmap);
		SysSetTrapAddress(sysTrapWinDrawBitmap, osPatchesWinDrawBitmapSony);
	}
#endif

#ifdef HANDERA_SUPPORT
	//we also enable have a similar one for Handera, if we find a 1.5 density image (else 1.0 density is used and it will be upscaled)
	if (isHanderaHiRes()) {
	
		osps->oldTrapWinDrawBitmap = SysGetTrapAddress(sysTrapWinDrawBitmap);
		SysSetTrapAddress(sysTrapWinDrawBitmap, osPatchesWinDrawBitmapHandera);
	}
#endif
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

void osPatchesDrawingInterceptionStateSet(Boolean enabled)
{
	struct OsPatchState *osps = osPatchesGetState(true);

	if (enabled)
		osps->winDrawBitmapPatchesDisabled--;
	else
		osps->winDrawBitmapPatchesDisabled++;
}
