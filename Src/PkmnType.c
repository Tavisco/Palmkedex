#include <PalmOS.h>

#include "Palmkedex.h"
#include "Src/pokeInfo.h"
#include "Rsc/Palmkedex_Rsc.h"


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

static UInt16 CalculateEffectivenessForType(const struct PokeInfo *info, UInt16 typeNum)
{
	UInt16 firstTypeDmg = pokeGetTypeEffectiveness(typeNum, info->type[0]);
	UInt16 secondTypeDmg = pokeGetTypeEffectiveness(typeNum, info->type[1]);

	return (firstTypeDmg * secondTypeDmg) / 100;
}

static void DrawEffectiveness(UInt16 selectedPkmnID, UInt8 x, UInt8 y, enum PokeType typeNum)
{
	UInt32 romVersion;
	IndexedColorType prevColor = 0;
	FontID prevFont;
	Char *str;
	UInt16 effectiveness;
	RGBColorType rgb;
	struct PokeInfo info;
	
	pokeInfoGet(&info, selectedPkmnID);

	effectiveness = CalculateEffectivenessForType(&info, typeNum);
	
	if (errNone != FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion))
		romVersion = 0;

	prevFont = FntSetFont(stdFont);
	if (romVersion >= sysMakeROMVersion(3,5,0,sysROMStageRelease,0)) {

		prevColor = WinSetTextColor(0);	//get current color
		WinSetTextColor(prevColor);
	}

	if (effectiveness != 100){
		FntSetFont(boldFont);
		
		rgb = GetRGBForEff(effectiveness);
		
		if (romVersion >= sysMakeROMVersion(3,5,0,sysROMStageRelease,0))
			WinSetTextColor(WinRGBToIndex(&rgb));
	}

    x += 35;
	WinDrawChars("x ", 2, x, y);
	
	x += 7;
	if (effectiveness == HALF_DAMAGE)
	{
		WinDrawChars("0.5", 3, x, y);
	}
	else if (effectiveness == QUARTER_DAMAGE)
	{
		WinDrawChars("0.25", 4, x, y);
	}
	else 
	{
		str = (Char *)MemPtrNew(sizeof(Char[4]));
		ErrFatalDisplayIf ((UInt32)str == 0, "Out of memory");
		MemSet(str, sizeof(Char[4]), 0);
		StrIToA(str, effectiveness/100);
	WinDrawChars(str, StrLen(str), x, y);
    
		MemPtrFree(str);
	}

	FntSetFont(prevFont);
	if (romVersion >= sysMakeROMVersion(3,5,0,sysROMStageRelease,0))
		WinSetTextColor(prevColor);
}

static void DrawTypeIcons(UInt16 selectedPkmnID)
{
    MemHandle 	h;
	BitmapPtr 	bitmapP;
    UInt8       i, x, y;

    // Set start positions
    x = 1;
    y = 19;

    for (i = PokeTypeFirst; i <= PokeTypeFairy; i++)
    {
        h = DmGetResource(bitmapRsc, POKEMON_TYPE_IMAGES_BASE + i);
        ErrFatalDisplayIf(!h, "Failed to load type bmp");

        bitmapP = (BitmapPtr)MemHandleLock(h);
        ErrFatalDisplayIf(!bitmapP, "Failed to lock type bmp");

        WinDrawBitmap (bitmapP, x, y);
        MemPtrUnlock (bitmapP);
        DmReleaseResource(h);

        DrawEffectiveness(selectedPkmnID, x, y, (enum PokeType)i);

        y += 16;

        if (i == PokeTypeFlying)
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
