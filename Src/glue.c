#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS
#include <PalmOS.h>
#include "glue.h"

void BmpGlueGetDimensions(const BitmapType *bitmapP, Coord *widthP, Coord *heightP, UInt16 *rowBytesP)
{
	UInt32 romVersion;

	if (errNone == FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion) && romVersion >= sysMakeROMVersion(4,0,0,sysROMStageRelease,0)) {

		BmpGetDimensions(bitmapP, widthP, heightP, rowBytesP);
	}
	else {
		if (widthP)
			*widthP = bitmapP->width;
		if (heightP)
			*heightP = bitmapP->height;
		if (rowBytesP)
			*rowBytesP = bitmapP->rowBytes;
	}
}

BitmapPtr BmpGlueGetNextBitmapAnyDensity(BitmapPtr bmp)
{
	UInt32 winMgrVersion;

	if (errNone == FtrGet(sysFtrCreator, sysFtrNumWinVersion, &winMgrVersion) && winMgrVersion >= 4)
		return BmpGetNextBitmapAnyDensity(bmp);
	else {

		do {
			UInt32 nextOffset = 0;

			switch (bmp->version) {
				case 0:
					nextOffset = 0;
					break;

				case 1:
					if (bmp->pixelSize == 0xff)
						nextOffset = sizeof(BitmapTypeV1);
					else
						nextOffset = 4UL * ((BitmapPtrV1)bmp)->nextDepthOffset;
					break;

				case 2:
					nextOffset = 4UL * ((BitmapPtrV2)bmp)->nextDepthOffset;
					break;

				case 3:
					nextOffset = ((BitmapPtrV3)bmp)->nextBitmapOffset;
					break;
			}

			if (!nextOffset)
				return NULL;

			bmp = (BitmapPtr)(((char*)bmp) + nextOffset);

		} while (bmp->version == 1 && bmp->pixelSize == 0xff);

		return bmp;
	}
}
