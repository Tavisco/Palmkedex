#include <PalmOS.h>

#include "Palmkedex.h"
#include "Rsc/Palmkedex_Rsc.h"
#include "Src/pngle.h"

static void on_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
{
	RGBColorType rgb;
	MemSet(&rgb, sizeof(rgb), 0);

	rgb.r=rgba[0];
	rgb.g=rgba[1];
	rgb.b=rgba[2];

	if (rgba[3] >= 1)
	{
		WinSetForeColorRGB(&rgb, NULL);
		WinDrawPixel(x, y);
	}
}

static void DrawPkmnSprite(UInt16 selectedPkmnId)
{
	MemHandle 	h;
	DmOpenRef 	dbRef;
	MemPtr pngData;
	pngle_t *pngle = pngle_new();
	UInt32 size;
	int ret;
	
	BitmapType *bmpP; 
	WinHandle win; 
	Err error; 
	RectangleType onScreenRect; 
	
	bmpP = BmpCreate(64, 64, 8, NULL, &error); 
	ErrFatalDisplayIf(!bmpP, "Failed to allocate BMP");

	win = WinCreateBitmapWindow(bmpP, &error);
	ErrFatalDisplayIf(!win, "Failed to allocate off-screen window");
	
	WinSetDrawWindow(win); 

	pngle_set_draw_callback(pngle, on_draw);
	
	dbRef = DmOpenDatabaseByTypeCreator('pSPR', 'PKSP', dmModeReadOnly);
	h = DmGet1Resource('pSPT', selectedPkmnId);

	pngData = MemHandleLock(h);
	size = MemPtrSize(pngData);

	ret = pngle_feed(pngle, pngData, size);
	ErrFatalDisplayIf(ret < 0, "Error feeding PNG data!");
	
	pngle_destroy(pngle);
	DmReleaseResource(h);
	if (dbRef)
	{
		DmCloseDatabase(dbRef);
	}

	WinSetDrawWindow(WinGetDisplayWindow());
	WinPaintBitmap(bmpP, 1, 16);

}

void LoadPkmnStats()
{
	UInt32 pstSharedInt;
	SharedVariables *sharedVars;
	UInt8 *pkmnBytes;
	MemHandle hndl;
	FormType *frm;
	ListType *list;
	Err err = errNone;

	err = FtrGet(appFileCreator, ftrShrdVarsNum, &pstSharedInt);
	ErrFatalDisplayIf (err != errNone, "Failed to load feature memory");
	sharedVars = (SharedVariables *)pstSharedInt;

	hndl = DmGetResource('pINF', sharedVars->selectedPkmnId);
	pkmnBytes = MemHandleLock(hndl);
	frm = FrmGetActiveForm();

	SetLabelInfo(PkmnMainHPValueLabel, pkmnBytes[0], frm);
	SetLabelInfo(PkmnMainAtkValueLabel, pkmnBytes[1], frm);
	SetLabelInfo(PkmnMainDefValueLabel, pkmnBytes[2], frm);
	SetLabelInfo(PkmnMainSPAtkValueLabel, pkmnBytes[3], frm);
	SetLabelInfo(PkmnMainSPDefValueLabel, pkmnBytes[4], frm);
	SetLabelInfo(PkmnMainSpeedValueLabel, pkmnBytes[5], frm);
	DrawTypes(pkmnBytes);

	MemHandleUnlock(hndl);

	SetDescriptionField(sharedVars->selectedPkmnId);
	DrawPkmnSprite(sharedVars->selectedPkmnId);

	list = GetObjectPtr(PkmnMainPopUpList);
	LstSetSelection(list, 0);

	SetFormTitle(sharedVars);
}

void SetDescriptionField(UInt16 selectedPkmnId)
{
	
	UInt16				scrollPos;
	UInt16				textHeight;
	UInt16				fieldHeight;
	Int16				maxValue;
	ScrollBarPtr	bar;
	MemHandle hndl = DmGet1Resource('pDSC', selectedPkmnId);
	Char* pkmnDesc = MemHandleLock(hndl);
	FieldType *fld = GetObjectPtr(PkmnMainDescField);

	FldSetTextPtr(fld, pkmnDesc);
	FldRecalculateField(fld, true);

	MemHandleUnlock(hndl);

	bar = GetObjectPtr (PkmnMainDescScroll);
	
	FldGetScrollValues (fld, &scrollPos, &textHeight,  &fieldHeight);
	
	if (textHeight > fieldHeight)
		maxValue = textHeight - fieldHeight;
	else if (scrollPos)
		maxValue = scrollPos;
	else
		maxValue = 0;

	SclSetScrollBar (bar, scrollPos, 0, maxValue, fieldHeight-1);
}

void DrawTypes(UInt8* pkmnBytes)
{
	MemHandle 				h;
	BitmapPtr 			bitmapP;

	h = DmGetResource('pTYP', pkmnBytes[6]);
	ErrFatalDisplayIf(!h, "Failed to load type bmp");

	bitmapP = (BitmapPtr)MemHandleLock(h);
	ErrFatalDisplayIf(!bitmapP, "Failed to lock type bmp");

	WinDrawBitmap (bitmapP , 1, 82);
	MemPtrUnlock (bitmapP);
	DmReleaseResource(h);

	if (pkmnBytes[7] != 21)
	{
		h = DmGetResource('pTYP', pkmnBytes[7]);
		ErrFatalDisplayIf(!h, "Failed to load type bmp");

		bitmapP = (BitmapPtr)MemHandleLock(h);
		ErrFatalDisplayIf(!bitmapP, "Failed to lock type bmp");

		WinDrawBitmap (bitmapP , 34, 82);
		MemPtrUnlock (bitmapP);
		DmReleaseResource(h);
	}
}

void SetLabelInfo(UInt16 labelId, UInt8 stat, FormType *frm)
{
	Char *str;

	str = (Char *)MemPtrNew(sizeof(Char[4]));
	if ((UInt32)str == 0)
		return;
	MemSet(str, sizeof(Char[4]), 0);

	StrIToA(str, stat);
	FrmCopyLabel(frm, labelId, str);

	MemPtrFree(str);
}

void SetFormTitle(SharedVariables *sharedVars)
{
	UInt32 pstSpeciesInt;
	Species *species;
	Char *numStr;
	Err err = errNone;

	err = FtrGet(appFileCreator, ftrPkmnNamesNum, &pstSpeciesInt);
	ErrFatalDisplayIf (err != errNone, "Failed to load feature memory");
	species = (Species*)pstSpeciesInt;

	
	if ((UInt32)sharedVars->pkmnFormTitle != 0)
	{
		MemPtrFree(sharedVars->pkmnFormTitle);
	}

	sharedVars->pkmnFormTitle = (Char *)MemPtrNew(sizeof(Char[18]));
	if ((UInt32)sharedVars->pkmnFormTitle == 0)
		return;
	MemSet(sharedVars->pkmnFormTitle, sizeof(Char[18]), 0);

	 numStr = (Char *)MemPtrNew(sizeof(Char[5]));
	 if ((UInt32)numStr == 0)
	 	return;
	 MemSet(numStr, sizeof(Char[5]), 0);

	StrCopy(sharedVars->pkmnFormTitle, species->nameList[sharedVars->selectedPkmnId-1].name);
	StrCat(sharedVars->pkmnFormTitle, " #");
	StrIToA(numStr, sharedVars->selectedPkmnId);
	StrCat(sharedVars->pkmnFormTitle, numStr);
	
	FrmSetTitle(FrmGetActiveForm(), sharedVars->pkmnFormTitle);

	MemPtrFree(numStr);
}

static void PkmnDescriptionScroll (WinDirectionType direction)
{
	Int16 value;
	Int16 min;
	Int16 max;
	Int16 pageSize;
	UInt16 linesToScroll;
	FieldPtr fld;
	ScrollBarPtr bar;

	fld = GetObjectPtr (PkmnMainDescField);
	
	if (FldScrollable (fld, direction))
	{
		linesToScroll = FldGetVisibleLines (fld) - 1;
		FldScrollField (fld, linesToScroll, direction);


		// Update the scroll bar.
		bar = GetObjectPtr (PkmnMainDescScroll);
		SclGetScrollBar (bar, &value, &min, &max, &pageSize);

		if (direction == winUp)
			value -= linesToScroll;
		else
			value += linesToScroll;
		
		SclSetScrollBar (bar, value, min, max, pageSize);
	}
}

static void PkmnDescriptionSimpleScroll (Int16 linesToScroll)
{
	FieldPtr fld;

	fld = GetObjectPtr (PkmnMainDescField);

	if (linesToScroll < 0)
		FldScrollField (fld, -linesToScroll, winUp);

	else if (linesToScroll > 0)
		FldScrollField (fld, linesToScroll, winDown);
}

/*
 * FUNCTION: PkmnMainFormDoCommand
 *
 * DESCRIPTION: This routine performs the menu command specified.
 *
 * PARAMETERS:
 *
 * command
 *     menu item id
 */

static Boolean PkmnMainFormDoCommand(UInt16 command)
{
	Boolean handled = false;

	switch (command)
	{
		case PkmnMainBackButton:
		{
			FrmGotoForm(MainForm);
			handled = true;
			break;
		}

		default:
			break;
	}

	return handled;
}

/*
 * FUNCTION: PkmnMainFormHandleEvent
 *
 * DESCRIPTION:
 *
 * This routine is the event handler for the "PkmnMainForm" of this 
 * application.
 *
 * PARAMETERS:
 *
 * eventP
 *     a pointer to an EventType structure
 *
 * RETURNED:
 *     true if the event was handled and should not be passed to
 *     FrmHandleEvent
 */

Boolean PkmnMainFormHandleEvent(EventType *eventP)
{
	Boolean handled = false;
	FormType * frmP;

	switch (eventP->eType) 
	{	
		case ctlSelectEvent:
			return PkmnMainFormDoCommand(eventP->data.menu.itemID);

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm(frmP);
			LoadPkmnStats();
			handled = true;
			break;

		case keyDownEvent:
			if (eventP->data.keyDown.chr == vchrPageUp)
			{
				PkmnDescriptionScroll (winUp);
				handled = true;
			}
			else if (eventP->data.keyDown.chr == vchrPageDown)
			{
				PkmnDescriptionScroll (winDown);
				handled = true;
			}
			break;

		case sclRepeatEvent:
			PkmnDescriptionSimpleScroll (eventP->data.sclRepeat.newValue - 
			eventP->data.sclRepeat.value);
			break;

		case popSelectEvent:
			if (eventP->data.popSelect.selection == 1)
			{
				FrmGotoForm(PkmnTypeForm);
			}
			break;

		default:
			break;
	}
    
	return handled;
}
