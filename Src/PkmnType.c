#include "BUILD_TYPE.h"

#include <PalmOS.h>

#include "Palmkedex.h"
#include "pokeInfo.h"
#include "UiResourceIDs.h"
#ifdef HANDERA_SUPPORT
#include "myTrg.h"
#endif

#define TYPES_START_X				1
#define TYPES_START_Y				17
#define TYPES_START_Y_HANDERA		29

#define TYPES_DX					89
#define TYPES_DX_HRES				84
#define TYPES_DX_HANDERA			133
#define TYPES_DY					16
#define TYPES_DY_HRES				11
#define TYPES_DY_HANDERA			24

#define TYPES_TXT_X_OFST			35
#define TYPES_TXT_X_OFST_HRES		52
#define TYPES_TXT_X_OFST_HANDERA	53

#define TYPES_TXT_Y_OFST			0
#define TYPES_TXT_Y_OFST_HANDERA	4


static Int16 GetTypesStartY(void)
{
	switch (getScreenDensity())
	{
	case kDensityOneAndAHalf:
		return isHanderaHiRes()? TYPES_START_Y_HANDERA : TYPES_START_Y;
		break;
	
	default:
		return TYPES_START_Y;
		break;
	}
}

static Int16 GetTypesDx(void)
{
	switch (getScreenDensity())
	{
	case kDensityDouble:
		return TYPES_DX_HRES;
		break;
	
	case kDensityOneAndAHalf:
		return isHanderaHiRes()? TYPES_DX_HANDERA : TYPES_DX;
		break;
		
	default:
		return TYPES_DX;
		break;
	}
}

static Int16 GetTypesDy(void)
{
	switch (getScreenDensity())
	{
	case kDensityDouble:
		return TYPES_DY_HRES;
		break;
	
	case kDensityOneAndAHalf:
		return isHanderaHiRes()? TYPES_DY_HANDERA : TYPES_DY;
		break;
	default:
		return TYPES_DY;
		break;
	}
}

static Int16 GetTextXOffset(void)
{
	switch (getScreenDensity())
	{
	case kDensityDouble:
		return TYPES_TXT_X_OFST_HRES;
		break;
	
	case kDensityOneAndAHalf:
		return isHanderaHiRes()? TYPES_TXT_X_OFST_HANDERA : TYPES_TXT_X_OFST;
		break;
	default:
		return TYPES_TXT_X_OFST;
		break;
	}
}

static Int16 GetTextYOffset(void)
{
	switch (getScreenDensity())
	{
	case kDensityOneAndAHalf:
		return isHanderaHiRes()? TYPES_TXT_Y_OFST_HANDERA : TYPES_TXT_Y_OFST;
		break;
	
	default:
		return TYPES_TXT_Y_OFST;
		break;
	}
}


static RGBColorType GetRGBForEff(UInt16 damage)
{
	RGBColorType rgb = {};
	
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

Boolean DrawEffectiveness(UInt16 selectedPkmnID, Int16 x, Int16 y, enum PokeType typeNum, Boolean skipEffOfOne)
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

	if (effectiveness == 100 && skipEffOfOne)
		return false;
	
	if (errNone != FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion))
		romVersion = 0;

	prevFont = FntSetFont(stdFont);

#ifdef MORE_THAN_1BPP_SUPPORT
	if (romVersion >= sysMakeROMVersion(3,5,0,sysROMStageRelease,0)) {

		prevColor = WinSetTextColor(0);	//get current color
		WinSetTextColor(prevColor);
	}
#endif

	if (effectiveness != 100){
		FntSetFont(boldFont);

		rgb = GetRGBForEff(effectiveness);

#ifdef MORE_THAN_1BPP_SUPPORT
		if (romVersion >= sysMakeROMVersion(3,5,0,sysROMStageRelease,0))
			WinSetTextColor(WinRGBToIndex(&rgb));
#endif
	}

	WinDrawChars("x ", 2, x, y);
	
	x += 7;
	if (effectiveness == HALF_DAMAGE)
	{
		WinDrawChars(".5", 2, x, y);
	}
	else if (effectiveness == QUARTER_DAMAGE)
	{
		WinDrawChars(".25", 3, x, y);
	}
	else
	{
		char str[4];
		StrIToA(str, effectiveness/100);
	WinDrawChars(str, StrLen(str), x, y);
	}

	FntSetFont(prevFont);
#ifdef MORE_THAN_1BPP_SUPPORT
	if (romVersion >= sysMakeROMVersion(3,5,0,sysROMStageRelease,0))
		WinSetTextColor(prevColor);
#endif

	(void)rgb;	//quiet down GCC's warnings

	return true;
}

static void DrawTypeIcons(UInt16 selectedPkmnID)
{
	Int16 x;
	Int16 y = GetTypesStartY();
	Int16 dx = GetTypesDx();
	Int16 dy = GetTypesDy();
	Int16 max_x, max_y; // maximum coordinate for the current screen resolution
	Int16 total_y_padding; // total padding between types to fill the screen
	Int16 x_padding, y_padding; // padding between each type

	const Int16 num_types = PokeTypeFairy - PokeTypeFirst + 1; // number of types
	MemHandle h;
	BitmapPtr bitmapP;
	UInt8 i;
	Coord extentX, extentY;
	
	WinGetWindowExtent(&extentX, &extentY);

	max_y = extentY - y;
	total_y_padding = max_y - (num_types / 2 * dy);
	y_padding = total_y_padding / (num_types / 2 - 1);

	max_x = extentX - TYPES_START_X;
	x = (max_x - dx * 2) / 2;

	if (x < TYPES_START_X)
		x = TYPES_START_X;

	for (i = PokeTypeFirst; i <= PokeTypeFairy; i++)
	{
		drawBmpForType(i, x, y, false);
		DrawEffectiveness(selectedPkmnID, x + GetTextXOffset(), y + GetTextYOffset(), (enum PokeType)i, false);

		y += dy;

		if (i == PokeTypeFlying)
		{
			x += dx;
			y = GetTypesStartY();
		}
		else
		{
			y += y_padding;
		}
	}
}

static void drawFormCustomThings(void)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

	DrawTypeIcons(sharedVars->selectedPkmnId);
	drawBackButton(PkmnTypeBackButton);
}

static void InitializeForm(void)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

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
		case PkmnTypeBackButton:
		{
			GoToPreferredMainForm();
			handled = true;
			break;
		}
		case PkmnTypeInfoButton:
		{
			FrmGotoForm(PkmnMainForm);
			handled = true;
			break;
		}

		default:
			break;
	}

	return handled;
}

static Boolean resizePkmnTypeForm(FormPtr fp)
{
#ifdef SCREEN_RESIZE_SUPPORT
	WinHandle wh = FrmGetWindowHandle(fp);
	Coord newW, newH, oldW, oldH;
	FieldPtr field = NULL;
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
			case PkmnTypeInfoButton:
			case PkmnTypeBackButton:
				rect.topLeft.x += newW - oldW;
				break;

			default:
				continue;
		}

		FrmSetObjectBounds(fp, idx, &rect);
	}
	if (field)
		FldRecalculateField(field, true /* we do not need the redraw but before PalmOs 4.0, without it, no recalculation takes place */);
	return true;
#else
	return false;
#endif
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
#ifdef HANDERA_SUPPORT
			if (isHanderaHiRes())
				VgaFormModify(frmP, vgaFormModify160To240);
#endif
			resizePkmnTypeForm(frmP);
			FrmDrawForm(frmP);
			InitializeForm();
			drawFormCustomThings();
			handled = true;
			break;

		case winEnterEvent:
			if (isHanderaHiRes())	//fallthrough except for handera
				break;
			//fallthrough

#ifdef HANDERA_SUPPORT
		case displayExtentChangedEvent:
#endif

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
