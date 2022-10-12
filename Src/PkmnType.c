#include <PalmOS.h>

#include "Palmkedex.h"
#include "Rsc/Palmkedex_Rsc.h"

static Boolean HasSecondType(UInt8 *pkmnBytes)
{
	return pkmnBytes[7] != UNKNOWN_TYPE;
}

static float ParseToFloat(UInt8 value)
{
	switch (value)
	{
		case 2:
			return 2.0f;
		case 1:
			return 1.0f;
		case 64:
			return 0.5f;
		case 0:
			return 0.0f;
		default:
			ErrFatalDisplay("Invalid pokemon damage!");
			return 0.0f;
	}
}

static float CalculateEffectivenessForType(UInt16 selectedPkmnID, UInt8 typeNum)
{
	float firstTypeDmg, secondTypeDmg;
	UInt8 *pkmnBytes, *effTable;
	MemHandle pInfHndl, pEffHndl;
	
	// Pokemon Data
	pInfHndl = DmGet1Resource('pINF', selectedPkmnID);
	ErrFatalDisplayIf(!pInfHndl, "Failed to load pINF");
	pkmnBytes = MemHandleLock(pInfHndl);

	// Effectiveness Data
	pEffHndl = DmGet1Resource('pEFF', typeNum);
	ErrFatalDisplayIf(!pInfHndl, "Failed to load pEFF");
	effTable = MemHandleLock(pEffHndl);

	firstTypeDmg = ParseToFloat(effTable[pkmnBytes[6]-1]);
	secondTypeDmg = ParseToFloat(effTable[pkmnBytes[7]-1]);

	MemHandleUnlock(pInfHndl);
	MemHandleUnlock(pEffHndl);

	return firstTypeDmg * secondTypeDmg;
}

static void DrawEffectiveness(UInt16 selectedPkmnID, UInt8 x, UInt8 y, UInt8 typeNum)
{
    UInt8 textColor;
	Char *str;
	float effectiveness = 0.0f;
	FontID curFont = 0;

	effectiveness = CalculateEffectivenessForType(selectedPkmnID, typeNum);

	if (effectiveness != 1){
		curFont = FntSetFont (boldFont);
		//if (effectiveness == QUARTER_DAMAGE)
		//{
			//textColor = WinSetTextColor(typeNum);
		//}
	}

    x += 35;
	WinDrawChars("x ", 2, x, y);
	
	x += 7;
	if (effectiveness == 0.5)
	{
		WinDrawChars("0.5", 3, x, y);
	}
	else if (effectiveness == 0.25)
	{
		WinDrawChars("0.25", 4, x, y);
	}
	else 
	{
		str = (Char *)MemPtrNew(sizeof(Char[4]));
		ErrFatalDisplayIf ((UInt32)str == 0, "Out of memory");
		MemSet(str, sizeof(Char[4]), 0);
		StrIToA(str, effectiveness);
    	WinDrawChars(str, StrLen(str), x, y);
    
		MemPtrFree(str);
	}
    FntSetFont (curFont);
    //WinSetTextColor(textColor);
}

static void DrawTypeIcons(UInt16 selectedPkmnID)
{
    MemHandle 	h;
	BitmapPtr 	bitmapP;
    UInt8       i, x, y;

    // Set start positions
    x = 1;
    y = 19;

    for (i = 1; i < 19; i++)
    {
        h = DmGetResource('pTYP', i);
        ErrFatalDisplayIf(!h, "Failed to load type bmp");

        bitmapP = (BitmapPtr)MemHandleLock(h);
        ErrFatalDisplayIf(!bitmapP, "Failed to lock type bmp");

        WinDrawBitmap (bitmapP, x, y);
        MemPtrUnlock (bitmapP);
        DmReleaseResource(h);

        DrawEffectiveness(selectedPkmnID, x, y, i);

        y += 16;

        if (i == 9)
        {
            x = 90;
            y = 19;
        }
    }
}

static void SetMenuSelection()
{
    ListType *list = GetObjectPtr(PkmnTypePopUpList);
	LstSetSelection(list, 1);
}

static void InitializeForm()
{
    UInt32 pstSharedInt;
	SharedVariables *sharedVars;
	Err err = errNone;

	err = FtrGet(appFileCreator, ftrShrdVarsNum, &pstSharedInt);
	ErrFatalDisplayIf (err != errNone, "Failed to load feature memory");
	sharedVars = (SharedVariables *)pstSharedInt;

    DrawTypeIcons(sharedVars->selectedPkmnId);

    SetMenuSelection();
    FrmSetTitle(FrmGetActiveForm(), sharedVars->pkmnFormTitle);
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

static Boolean PkmnTypeFormDoCommand(UInt16 command)
{
	Boolean handled = false;

	switch (command)
	{
		// case PkmnMainBackButton:
		// {
		// 	FrmGotoForm(MainForm);
		// 	handled = true;
		// 	break;
		// }

		default:
			break;
	}

	return handled;
}

Boolean PkmnTypeFormHandleEvent(EventType * eventP)
{
	Boolean handled = false;
	FormType * frmP;

	switch (eventP->eType) 
	{	
		case ctlSelectEvent:
			return PkmnTypeFormDoCommand(eventP->data.menu.itemID);

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm(frmP);
            InitializeForm();
			handled = true;
			break;

        case popSelectEvent:
			if (eventP->data.popSelect.selection == 0)
			{
				FrmGotoForm(PkmnMainForm);
			}
			break;

		default:
			break;
	}
    
	return handled;
}