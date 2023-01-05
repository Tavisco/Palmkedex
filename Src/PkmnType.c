#include <PalmOS.h>

#include "Palmkedex.h"
#include "pokeInfo.h"
#include "UiResourceIDs.h"
#include "myTrg.h"

#define TYPES_START_X				1
#define TYPES_START_Y				19
#define TYPES_DX					89
#define TYPES_DY					16

#define TYPES_START_X_HANDERA		1
#define TYPES_START_Y_HANDERA		29
#define TYPES_DX_HANDERA			133
#define TYPES_DY_HANDERA			24




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
    UInt16 x = isHanderaHiRes() ? TYPES_START_X_HANDERA : TYPES_START_X;
	UInt16 y = isHanderaHiRes() ? TYPES_START_Y_HANDERA : TYPES_START_Y;
	const UInt16 dx = isHanderaHiRes() ? TYPES_DX_HANDERA : TYPES_DX;
	const UInt16 dy = isHanderaHiRes() ? TYPES_DY_HANDERA : TYPES_DY;
	MemHandle 	h;
	BitmapPtr 	bitmapP;
    UInt8       i;


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

        y += dy;

        if (i == PokeTypeFlying)
        {
            x += dx;
            y -= 9 * dy;
        }
    }
}

static void SetMenuSelection(void)
{
    ListType *list = GetObjectPtr(PkmnTypePopUpList);
	LstSetSelection(list, 1);
}

static void drawFormCustomThings(void)
{
	SharedVariables *sharedVars;

	FtrGet(appFileCreator, ftrShrdVarsNum, (UInt32*)&sharedVars);

	DrawTypeIcons(sharedVars->selectedPkmnId);
}

static void InitializeForm(void)
{
	SharedVariables *sharedVars;

	FtrGet(appFileCreator, ftrShrdVarsNum, (UInt32*)&sharedVars);

    SetMenuSelection();
    SetFormTitle(sharedVars);
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

static Boolean resizePkmnTypeForm(FormPtr fp)
{
	WinHandle wh = FrmGetWindowHandle(fp);
	Coord newW, newH, oldW, oldH;
	RectangleType rect;
	UInt32 romVersion;
	UInt16 idx, num;

	WinGetDisplayExtent(&newW, &newH);
	wh = WinSetDrawWindow(wh);
	WinGetDrawWindowBounds(&rect);
	wh = WinSetDrawWindow(wh);

	if (rect.extent.x == newW && rect.extent.y == newH)
		return false;

	oldW = rect.extent.x;
	oldH = rect.extent.y;
	rect.extent.x = newW;
	rect.extent.y = newH;
	WinSetBounds(wh, &rect);
	(void)oldH;
	(void)oldW;

	for (idx = 0, num = FrmGetNumberOfObjects(fp); idx < num; idx++) {

		FrmGetObjectBounds(fp, idx, &rect);

		switch (FrmGetObjectId(fp, idx)) {
			case PkmnTypePopUpList:
			case PkmnTypePopUpTrigger:
				rect.topLeft.x += newW - oldW;
				break;

			default:
				continue;
		}

		FrmSetObjectBounds(fp, idx, &rect);
	}

	return true;
}

Boolean PkmnTypeFormHandleEvent(EventType * eventP)
{
	FormType * frmP = FrmGetActiveForm();
	Boolean handled = false;
	UInt32 pinsVersion;

	switch (eventP->eType) 
	{	
		case ctlSelectEvent:
			return PkmnTypeFormDoCommand(eventP->data.menu.itemID);

		case frmOpenEvent:
			if (errNone == FtrGet(pinCreator, pinFtrAPIVersion, &pinsVersion) && pinsVersion) {
				FrmSetDIAPolicyAttr(frmP, frmDIAPolicyCustom);
				WinSetConstraintsSize(FrmGetWindowHandle(frmP), 160, 240, 640, 160, 240, 640);
				PINSetInputTriggerState(pinInputTriggerEnabled);
			}
			if (isHanderaHiRes())
				VgaFormModify(frmP, vgaFormModify160To240);
			resizePkmnTypeForm(frmP);
			FrmDrawForm(frmP);
            InitializeForm();
            drawFormCustomThings();
			handled = true;
			break;

        case popSelectEvent:
			if (eventP->data.popSelect.selection == 0)
			{
				FrmGotoForm(PkmnMainForm);
			}
			break;

		case winEnterEvent:
			if (isHanderaHiRes())
				break;
			//fallthrough except for handera
			//fallthrough

		case displayExtentChangedEvent:
		case winDisplayChangedEvent:
		case frmUpdateEvent:
			if (resizePkmnTypeForm(frmP)) {
				WinEraseWindow();
				FrmDrawForm(frmP);
				drawFormCustomThings();
			}
			return true;

		default:
			break;
	}
    
	return handled;
}
