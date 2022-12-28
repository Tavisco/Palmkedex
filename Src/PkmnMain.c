#include <PalmOS.h>

#include "Palmkedex.h"
#include "Rsc/Palmkedex_Rsc.h"
#include "Src/imgDraw.h"


void DrawPkmnPlaceholder()
{
	MemHandle h;
	BitmapPtr bitmapP;
	h = DmGetResource(bitmapRsc, BmpMissingPokemon);

	bitmapP = (BitmapPtr)MemHandleLock(h);
	ErrFatalDisplayIf(!bitmapP, "Failed to lock placeholder bmp");

	WinDrawBitmap(bitmapP, 1, 16);
	MemPtrUnlock(bitmapP);
	DmReleaseResource(h);
}

void DrawPkmnSprite(UInt16 selectedPkmnId)
{
	MemHandle imgMemHandle;
	DmOpenRef dbRef;
	MemPtr pngData;
	UInt32 size;
	int ret;
	BitmapType *bmpP;
	WinHandle win;
	Err error;
	struct DrawState *ds;

	// Check if the PNG for the current pkmn
	// is already decoded in memory
	error = FtrGet(appFileCreator, 0, (UInt32*)&ds);
	if (error == errNone)
	{
		// If it is, draw it and return
		imgDrawRedraw(ds, 1, 16);
		return;
	}

	// Check if there is any PNG for current pkmn
	dbRef = DmOpenDatabaseByTypeCreator('pSPR', 'PKSP', dmModeReadOnly);
	imgMemHandle = DmGet1Resource('pSPT', selectedPkmnId);
	if (!imgMemHandle)
	{
		// If there isnt, draw the placeholder and return
		DrawPkmnPlaceholder();
		if (dbRef)
		{
			DmCloseDatabase(dbRef);
		}
		return;
	}

	imgDrawAt(&ds, MemHandleLock(imgMemHandle), MemHandleSize(imgMemHandle), 1, 16, 64, 64);
	MemHandleUnlock(imgMemHandle);
	DmReleaseResource(imgMemHandle);
	if (dbRef)
	{
		DmCloseDatabase(dbRef);
	}
	// And store its pointer to quickly redraw it
	FtrSet(appFileCreator, 0, (UInt32)ds);
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
	ErrFatalDisplayIf(err != errNone, "Failed to load feature memory");
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

	list = GetObjectPtr(PkmnMainPopUpList);
	LstSetSelection(list, 0);

	SetDescriptionField(sharedVars->selectedPkmnId);
	DrawPkmnSprite(sharedVars->selectedPkmnId);

	SetFormTitle(sharedVars);
}

void SetDescriptionField(UInt16 selectedPkmnId)
{
	UInt16 textHeight;
	UInt16 fieldHeight;
	Int16 maxValue;
	MemHandle hndl = DmGet1Resource('pDSC', selectedPkmnId);
	Char *pkmnDesc = MemHandleLock(hndl);
	FieldType *fld = GetObjectPtr(PkmnMainDescField);

	FldSetTextPtr(fld, pkmnDesc);
	FldRecalculateField(fld, true);

	MemHandleUnlock(hndl);
}

void DrawTypes(UInt8 *pkmnBytes)
{
	MemHandle h;
	BitmapPtr bitmapP;

	h = DmGetResource(bitmapRsc, POKEMON_TYPE_IMAGES_BASE + pkmnBytes[6]);
	ErrFatalDisplayIf(!h, "Failed to load type bmp");

	bitmapP = (BitmapPtr)MemHandleLock(h);
	ErrFatalDisplayIf(!bitmapP, "Failed to lock type bmp");

	WinDrawBitmap(bitmapP, 1, 82);
	MemPtrUnlock(bitmapP);
	DmReleaseResource(h);

	if (pkmnBytes[7] != 21)
	{
		h = DmGetResource(bitmapRsc, POKEMON_TYPE_IMAGES_BASE + pkmnBytes[7]);
		ErrFatalDisplayIf(!h, "Failed to load type bmp");

		bitmapP = (BitmapPtr)MemHandleLock(h);
		ErrFatalDisplayIf(!bitmapP, "Failed to lock type bmp");

		WinDrawBitmap(bitmapP, 34, 82);
		MemPtrUnlock(bitmapP);
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
	ErrFatalDisplayIf(err != errNone, "Failed to load feature memory");
	species = (Species *)pstSpeciesInt;

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

	StrCopy(sharedVars->pkmnFormTitle, species->nameList[sharedVars->selectedPkmnId - 1].name);
	StrCat(sharedVars->pkmnFormTitle, " #");
	StrIToA(numStr, sharedVars->selectedPkmnId);
	StrCat(sharedVars->pkmnFormTitle, numStr);

	FrmSetTitle(FrmGetActiveForm(), sharedVars->pkmnFormTitle);

	MemPtrFree(numStr);
}

static void PkmnDescriptionScroll(WinDirectionType direction)
{
	Int16 value;
	Int16 min;
	Int16 max;
	Int16 pageSize;
	UInt16 linesToScroll;
	FieldPtr fld;
	ScrollBarPtr bar;

	fld = GetObjectPtr(PkmnMainDescField);

	if (FldScrollable(fld, direction))
	{
		linesToScroll = FldGetVisibleLines(fld) - 1;
		FldScrollField(fld, linesToScroll, direction);
	}
}

static void unregisterCurrentPng()
{
	struct DrawState *ds;

	if (FtrGet(appFileCreator, 0, (UInt32*)&ds) == errNone && ds)
	{
		imgDrawStateFree(ds);
		FtrUnregister(appFileCreator, 0);
	}
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
	FormType *frmP;

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
			PkmnDescriptionScroll(winUp);
			handled = true;
		}
		else if (eventP->data.keyDown.chr == vchrPageDown)
		{
			PkmnDescriptionScroll(winDown);
			handled = true;
		}
		break;

	case popSelectEvent:
		if (eventP->data.popSelect.selection == 1)
		{
			FrmGotoForm(PkmnTypeForm);
		}
		break;

	case frmCloseEvent:
		//no matter why we're closing, free the bitmap
		unregisterCurrentPng();
		break;

	default:
		break;
	}

	return handled;
}
