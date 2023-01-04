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