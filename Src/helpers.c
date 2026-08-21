#include <PalmOS.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "imgDraw.h"
#include "Palmkedex.h"
#include "UiResourceIDs.h"



#ifndef NATIVE_CODE

	#include "myTrg.h"

	static void debug_printf(const char* fmt, ...) {
	    UInt32 ftrValue;
	    char buffer[256];
	    va_list args;

	    if (FtrGet('cldp', 0, &ftrValue) || ftrValue != 0x20150103) return;

	    va_start(args, fmt);

	    if (StrVPrintF(buffer, fmt, (_Palm_va_list)args) > 255)
	        DbgMessage("DebugLog: buffer overflowed, memory corruption ahead");
	    else
	        DbgMessage(buffer);
	}
#endif

void* malloc(size_t sz) { return MemPtrNew(sz); }

void free(void *p) { if(p) MemPtrFree(p); }

void* realloc(void *p, size_t sz) {
    
    void *newPtr;
    
    if (errNone == MemPtrResize(p, sz))
    	return p;
    
    //fail means we ARE upsizing!
    
    newPtr = MemPtrNew(sz);
    if (!newPtr)
    	return NULL;
    
    MemMove(newPtr, p, MemPtrSize(p));
    MemPtrFree(p);
    return newPtr;
}

void* calloc(size_t qtty, size_t sz) {
    MemPtr p = MemPtrNew(sz*qtty);
    MemSet(p, sz*qtty, 0);
    return p;
}

void* memset (void *p, int c, size_t l) {
    MemSet(p, l, c);
    
    return p;
}

void* memcpy (void *d, const void *s, size_t l) {
    MemMove(d, s, l);
    
    return d;
}

void unregisterCurrentAci(void)
{
		struct DrawState *ds;

		ds = (struct DrawState*)globalsSlotVal(GLOBALS_SLOT_DETAIL_ACI_IMAGE);

		if (ds)
		{
			imgDrawStateFree(ds);
			*globalsSlotPtr(GLOBALS_SLOT_DETAIL_ACI_IMAGE) = NULL;
		}
}

void FreeDescriptionField(UInt16 objectID)
{
		FieldType *fld = GetObjectPtr(objectID);
		Char *ptr = FldGetTextPtr(fld);

		FldSetTextPtr(fld, (char*)noDexEntryString);

		if (ptr && ptr != (char*)noDexEntryString) {
			MemPtrFree(ptr);
		}
}

void SetItemMainFormTitle(SharedVariables *sharedVars)
{
		char titleStr[ITEM_NAME_LEN + 6]; // 6 = space + # + 4nums + null char

		itemNameGet(titleStr, sharedVars->selectedPkmnId);
		StrCat(titleStr, " #");
		StrIToA(titleStr + StrLen(titleStr), sharedVars->selectedPkmnId);

		FrmCopyTitle(FrmGetActiveForm(), titleStr);
}

void DrawItemPlaceholder(Coord x, Coord y)
{
		MemHandle h;
		BitmapPtr bitmapP;
		h = DmGetResource(bitmapRsc, BmpMissingIcon);

		bitmapP = (BitmapPtr)MemHandleLock(h);

		WinDrawBitmap(bitmapP, x, y);
		MemPtrUnlock(bitmapP);
		DmReleaseResource(h);
}

UInt8 getDanaMode(Coord width, Coord height)
{
	if (height == 160 && width > 320)
		return DANA_LANDSCAPE;

	if (width == 160 && height > 320)
		return DANA_POTRAIT;

	return 0;
}

void InnerIterate(WChar c)
	{
		SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

		UInt16 selected = sharedVars->selectedPkmnId;

		if (c == vchrPageUp)
		{
			selected--;
		}
		else if (c == vchrPageDown)
		{
			selected++;
		}

		if (selected == 0)
		{
			selected = TOTAL_ITEM_COUNT_ZERO_BASED;
		}
		else if (selected > TOTAL_ITEM_COUNT_ZERO_BASED)
		{
			selected = 1;
		}

		sharedVars->selectedPkmnId = selected;
	}