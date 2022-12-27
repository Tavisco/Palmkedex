#include <PalmOS.h>

#include "Palmkedex.h"
#include "Rsc/Palmkedex_Rsc.h"

static Boolean HasSecondType(UInt8 *pkmnBytes)
{
	return pkmnBytes[7] != UNKNOWN_TYPE;
}

static RGBColorType GetRGBForEff(UInt16 damage)
{
	RGBColorType rgb;
	MemSet(&rgb, sizeof(rgb), 0);
	
	switch (damage)
	{
	case QUADRUPLE_DAMAGE:
		rgb.r=255;
		rgb.g=0;
		rgb.b=0;
		break;
	case DOUBLE_DAMAGE:
		rgb.r=255;
		rgb.g=128;
		rgb.b=0;
		break;
	case HALF_DAMAGE:
		rgb.r=153;
		rgb.g=255;
		rgb.b=51;
		break;
	case QUARTER_DAMAGE:
		rgb.r=102;
		rgb.g=204;
		rgb.b=0;
		break;
	case NO_DAMAGE:
		rgb.r=0;
		rgb.g=153;
		rgb.b=0;
		break;
	
	default: // 1x
		rgb.r=0;
		rgb.g=0;
		rgb.b=0;
		break;
	}
	
	return rgb;
}

static UInt16 CalculateEffectivenessForType(UInt8 *pkmnBytes, UInt16 typeNum)
{
	UInt16 firstTypeDmg, secondTypeDmg;
	UInt8 *effTable;
	MemHandle pEffHndl;
	
	// Effectiveness Data
	pEffHndl = DmGet1Resource('pEFF', typeNum);
	ErrFatalDisplayIf(!pEffHndl, "Failed to load pEFF");
	effTable = MemHandleLock(pEffHndl);

	firstTypeDmg = effTable[pkmnBytes[6]-1];
	secondTypeDmg = effTable[pkmnBytes[7]-1];

	MemHandleUnlock(pEffHndl);

	if (firstTypeDmg == HALF_DAMAGE)
	{
		return secondTypeDmg / 2;
	}

	if (secondTypeDmg == HALF_DAMAGE)
	{
		return firstTypeDmg / 2;
	}

	return (firstTypeDmg * secondTypeDmg) / 100;
}

static void DrawEffectiveness(UInt16 selectedPkmnID, UInt8 x, UInt8 y, UInt8 typeNum)
{
	Char *str;
	UInt16 effectiveness;
	MemHandle pInfHndl;
	UInt8 *pkmnBytes;
	RGBColorType rgb;
	
	pInfHndl = DmGet1Resource('pINF', selectedPkmnID);
	ErrFatalDisplayIf(!pInfHndl, "Failed to load pINF");
	pkmnBytes = MemHandleLock(pInfHndl);

	effectiveness = CalculateEffectivenessForType(pkmnBytes, typeNum);
	
	MemHandleUnlock(pInfHndl);

	if (effectiveness != 100){
		WinPushDrawState();
		FntSetFont(boldFont);
		
		rgb = GetRGBForEff(effectiveness);
		
		WinSetTextColor(WinRGBToIndex(&rgb));
	}

    x += 35;
	WinPaintChars("x ", 2, x, y);
	
	x += 7;
	if (effectiveness == HALF_DAMAGE)
	{
		WinPaintChars("0.5", 3, x, y);
	}
	else if (effectiveness == QUARTER_DAMAGE)
	{
		WinPaintChars("0.25", 4, x, y);
	}
	else 
	{
		str = (Char *)MemPtrNew(sizeof(Char[4]));
		ErrFatalDisplayIf ((UInt32)str == 0, "Out of memory");
		MemSet(str, sizeof(Char[4]), 0);
		StrIToA(str, effectiveness/100);
    	WinPaintChars(str, StrLen(str), x, y);
    
		MemPtrFree(str);
	}
    WinPopDrawState();
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
        h = DmGetResource(bitmapRsc, POKEMON_TYPE_IMAGES_BASE + i);
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
