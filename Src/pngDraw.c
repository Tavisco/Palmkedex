#include <PalmOS.h>
#include <PceNativeCall.h>
#include "pngDrawInt.h"
#include "pngDraw.h"


void pngDrawStateFree(struct DrawState *ds)
{
	BmpDelete(ds->b);
	MemPtrFree(ds);
}

void pngDrawRedraw(struct DrawState *ds, int16_t x, int16_t y)
{
	WinDrawBitmap(ds->b, x, y);
}


static unsigned char pngDrawHdrCbk(struct DrawState *ds, uint32_t w, uint32_t h)
{
	UInt16 rowBytes;
	BitmapPtr b;
	UInt32 val;
	Err err;

	//check for exact integer or 1/2 multiple of size, same for W & H
	if (w * 2 % ds->expectedW || h * 2 % ds->expectedW || w * 2 / ds->expectedW != h * 2 / ds->expectedW)
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
			if (!(ds->densitySupportFlags & (PNG_VARIOUS_DENSITIES_SUPPORTED | PNG_DOUBLE_DENSITY_SUPPORTED)))
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

	
	b = BmpCreate(w, h, 16, NULL, &err);
	if (b == NULL) {
		if (err == sysErrParamErr)
		{
			ErrFatalDisplay("Sprites not supported on this device as of now! Please uninstall them to use Palmkedex.");
		}
		if (err != sysErrNoFreeResource)
		{
			ErrFatalDisplay("Not enough memory!");
		}
		ErrFatalDisplay("Error creating bitmap!");
		
		return false;
	}
	ds->bits = BmpGetBits(b);
	if (ds->bits == NULL) {
		BmpDelete(b);
		ErrFatalDisplay("Error getting bitmap bits!");
		
		return false;
	}
	
	BmpGetDimensions(b, NULL, NULL, &rowBytes);
	ds->rowHalfwords = rowBytes / sizeof(UInt16);
	ds->b = b;
	
	return true;
}

int pngDrawDecodeCall(struct DrawState *ds, const void *data, uint32_t dataSz)
{
	UInt32 processorType, result;
	int ret;
	
	if (errNone == FtrGet(sysFileCSystem, sysFtrNumProcessorID, &processorType)	&& sysFtrNumProcessorIsARM(processorType)) {
		
		MemHandle armH;
	
		struct ArmParams p = {
			.ds = ds,
			.data = data,
			.dataSz = dataSz,
			.hdrDecodedF = pngDrawHdrCbk,
		};
				
		ret = PceNativeCall((NativeFuncType*)MemHandleLock(armH = DmGetResource('armc', 1)), &p);
		MemHandleUnlock(armH);
		DmReleaseResource(armH);
	}
	else {
		
		ret = pngDrawDecode(ds, data, dataSz, pngDrawHdrCbk);
	}
	
	return ret;
}

void pngDrawAt(struct DrawState **dsP, const void *data, uint32_t dataSz, int16_t x, int16_t y, uint32_t expectedW, uint32_t expectedH, uint8_t densitySupportFlags)
{
	struct DrawState *ds;
	int ret;

	ds = (struct DrawState *)MemPtrNew(sizeof(struct DrawState));
	if (!ds)
		return;
	MemSet(ds, sizeof(*ds), 0);
	ds->expectedW = expectedW;
	ds->expectedH = expectedH;
	ds->densitySupportFlags = densitySupportFlags;

	ret = pngDrawDecodeCall(ds, data, dataSz);
	ErrFatalDisplayIf(ret < 0, "Error feeding PNG data!");

	pngDrawRedraw(ds, x, y);

	*dsP = ds;
}
