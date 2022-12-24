#include <PalmOS.h>
#include <PceNativeCall.h>
#include "pngDrawInt.h"
#include "pngDraw.h"


static struct DrawState* setupDrawState(uint32_t w, uint32_t h) {
	Err err;
	BitmapPtr b = BmpCreate(w, h, 16, NULL, &err);

	// Check if BmpCreate succeeded
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
		return NULL;
	}

	struct DrawState *ds = (struct DrawState *)MemPtrNew(sizeof(struct DrawState));

	// Check if MemPtrNew succeeded
	if (!ds) {
		BmpDelete(b);
		ErrFatalDisplay("Error allocating memory for draw state!");
		return NULL;
	}

	MemSet(ds, sizeof(struct DrawState), 0);
	UInt16 rowBytes;

	BmpGetDimensions(b, NULL, NULL, &rowBytes);
	ds->rowHalfwords = rowBytes / sizeof(UInt16);
	ds->b = b;
	ds->bits = BmpGetBits(b);

	if (ds->bits == NULL) {
		BmpDelete(b);
		ErrFatalDisplay("Error getting bitmap bits!");
		return NULL;
	}

	return ds;
}

static void finish(struct DrawState *ds, uint32_t x, uint32_t y)
{
	WinDrawBitmap(ds->b, x, y);
	BmpDelete(ds->b);
	MemPtrFree(ds);
}

void pngDrawStateFree(struct DrawState *ds)
{
	BmpDelete(ds->b);
	MemPtrFree(ds);
}

void pngDrawRedraw(struct DrawState *ds, int16_t x, int16_t y)
{
	WinDrawBitmap(ds->b, x, y);
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
		};
		
		ret = PceNativeCall((NativeFuncType*)MemHandleLock(armH = DmGetResource('armc', 1)), &p);
		MemHandleUnlock(armH);
		DmReleaseResource(armH);
	}
	else {
		
		ret = pngDrawDecode(ds, data, dataSz);
	}
	
	return ret;
}

void pngDrawAt(struct DrawState **dsP, const void *data, uint32_t dataSz, int16_t x, int16_t y, uint32_t w, uint32_t h)
{
	struct DrawState *ds;
	int ret;

	// Start the PNG decoding and drawing to memory
	ds = setupDrawState(64, 64);
	ErrFatalDisplayIf(!ds, "Failed to setup DrawState!");

	ret = pngDrawDecodeCall(ds, data, dataSz);
	ErrFatalDisplayIf(ret < 0, "Error feeding PNG data!");

	pngDrawRedraw(ds, x, y);

	*dsP = ds;
}
