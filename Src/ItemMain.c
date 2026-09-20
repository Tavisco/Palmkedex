//
// Created by tavisco on 07/07/2026.
//

#include "BUILD_TYPE.h"

#include <PalmOS.h>
#include "Palmkedex.h"
#include "UiResourceIDs.h"
#include "pokeInfo.h"
#include "imgDraw.h"
#ifdef HANDERA_SUPPORT
#include "myTrg.h"
#endif

#define ITEM_ICON_SIZE						32
#define ITEM_ICON_SIZE_HANDERA				60
#define ITEM_IMAGE_AT_X					1
#define ITEM_IMAGE_AT_X_HANDERA			1
#define ITEM_IMAGE_AT_Y					16
#define ITEM_IMAGE_AT_Y_HANDERA			24

#define DANA_POTRAIT					1
#define DANA_LANDSCAPE					2

static const char noDexEntryString[31] = "This pokemon has no Dex Entry.";

static void clearItemImage(void)
{
	RectangleType rect;
	int itemImageSize = isHanderaHiRes() ? ITEM_ICON_SIZE_HANDERA : ITEM_ICON_SIZE;

	rect.topLeft.x = isHanderaHiRes() ? ITEM_IMAGE_AT_X_HANDERA : ITEM_IMAGE_AT_X;
	rect.topLeft.y = isHanderaHiRes() ? ITEM_IMAGE_AT_Y_HANDERA : ITEM_IMAGE_AT_Y;
	rect.extent.x = itemImageSize;

	WinEraseRectangle(&rect, 0);
}

static void FreeUsedVariables(void)
{
	unregisterCurrentAci();
	FreeDescriptionField(ItemsFormDescriptionField, noDexEntryString);
}

static void redrawDecodedSprite(struct DrawState *ds)
{
	if (isHanderaHiRes())
		imgDrawRedraw(ds, ITEM_IMAGE_AT_X_HANDERA, ITEM_IMAGE_AT_Y_HANDERA);
	else
		imgDrawRedraw(ds, ITEM_IMAGE_AT_X, ITEM_IMAGE_AT_Y);
}

static void DrawItemSprite(UInt16 selectedItemId) // Can we have QR Code?
{
	MemHandle imgMemHandle;
	struct DrawState *ds;

	ds = (struct DrawState*)globalsSlotVal(GLOBALS_SLOT_DETAIL_ACI_IMAGE);

	// Check if there is any image for current item
	imgMemHandle = aciImageGet(selectedItemId, ITEM_ICON);
	if (imgMemHandle) {
		if (imgDecode(&ds, MemHandleLock(imgMemHandle), MemHandleSize(imgMemHandle), ITEM_ICON_SIZE, ITEM_ICON_SIZE, 0)) {
			redrawDecodedSprite(ds);
		} else {
			ds = NULL;
		}
		MemHandleUnlock(imgMemHandle);
		imageRelease(imgMemHandle, ITEM_ICON);
		*globalsSlotPtr(GLOBALS_SLOT_DETAIL_ACI_IMAGE) = ds;
	} else {
		if (isHanderaHiRes())
			DrawItemPlaceholder(ITEM_IMAGE_AT_X_HANDERA, ITEM_IMAGE_AT_Y_HANDERA);
		else
			DrawItemPlaceholder(ITEM_IMAGE_AT_X, ITEM_IMAGE_AT_Y);
	}
}

static void SetDescriptionField(UInt16 selectedItemID)
{
	Coord width, height;
	FieldType *fld;
	RectangleType rect;
	UInt8 danaMode;
	MemHandle hndl;
	char *dexEntry = NULL;

	WinGetWindowExtent(&width, &height);

	fld = GetObjectPtr(ItemsFormDescriptionField);
	danaMode = getDanaMode(width, height);

	if (danaMode == DANA_LANDSCAPE)
	{
		rect.topLeft.x = ITEM_IMAGE_AT_X + ITEM_ICON_SIZE + 10;
		rect.extent.x = width - rect.topLeft.x - 59;
		rect.topLeft.y = 22;
		rect.extent.y = 58;
		FldSetBounds(fld, &rect);
	}

	DmOpenRef dbRef = DmOpenDatabaseByTypeCreator('ITEM', appFileCreator, dmModeReadOnly);
	if (!dbRef)
	{
		ErrFatalDisplay("Failed to find item database!");
		return;
	}

	hndl = DmGet1Resource('DESC', 0);
	dexEntry = dexEntryGet(hndl, selectedItemID);

	if (hndl) {
		DmReleaseResource(hndl);
	}

	if (!dexEntry)
		dexEntry = (char*)noDexEntryString;

	FreeDescriptionField(ItemsFormDescriptionField, noDexEntryString);
	FldSetTextPtr(fld, dexEntry);
	FldRecalculateField(fld, true);
	DmCloseDatabase(dbRef);
}

static void DrawItemType(UInt16 selectedItemId) {
	struct ItemInfo itemInfo;
	itemInfoGet(&itemInfo, selectedItemId);
	Char *typeStr;
	switch (itemInfo.type) {
		case 0:
			typeStr = "Battle items";
			break;
		case 1:
			typeStr = "Berries";
			break;
		case 2:
			typeStr = "General items";
			break;
		case 3:
			typeStr = "Hold items";
			break;
		case 4:
			typeStr = "Machines";
			break;
		case 5:
			typeStr = "Medicine";
			break;
		case 6:
			typeStr = "Pokeballs";
			break;
		default:
			typeStr = "Unknown";
	}

	FormType *frm;
	frm = FrmGetActiveForm();
	FrmCopyLabel(frm, ItemsFormItemTypeField, typeStr);
}

static void drawFormCustomThings(void)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

	drawBackButton(ItemsFormBackButton);
	SetCustomFormTitle(sharedVars);
	DrawItemSprite(sharedVars->selectedPkmnId);
	DrawItemType(sharedVars->selectedPkmnId);
	SetDescriptionField(sharedVars->selectedPkmnId);
	drawQr(sharedVars->selectedPkmnId, 102, 0, 102, 0, 2, GRID_MODE_ITEMS);
}

static void IterateItem(WChar c)
{
	InnerIterate(c);
	clearItemImage();
	FreeUsedVariables();
	drawFormCustomThings();
}

static Boolean ItemMainFormDoCommand(UInt16 command, EventType *eventP)
{
	Boolean handled = false;

	switch (command)
	{
		case ItemsFormBackButton:
		{
			GoToPreferredMainForm();
			handled = true;
			break;
		}

		default:
			break;
	}

	return handled;
}

static Boolean isSelectedItemInvalid(void)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	return sharedVars->selectedPkmnId == 0 || sharedVars->selectedPkmnId > TOTAL_ITEM_COUNT_ZERO_BASED;
}

Boolean ItemMainFormHandleEvent(EventType *eventP)
{
	FormType *frmP = FrmGetActiveForm();
	Boolean handled = false;
	UInt32 pinsVersion;

	switch (eventP->eType)
	{
	case ctlSelectEvent:
		return ItemMainFormDoCommand(eventP->data.ctlSelect.controlID, eventP);

	case frmOpenEvent:
		if (isSelectedItemInvalid()) {
			ErrAlertCustom(0, "You tried opening an invalid item!", NULL, NULL);
			GoToPreferredMainForm();
			return true;
		}

		if (errNone == FtrGet(pinCreator, pinFtrAPIVersion, &pinsVersion) && pinsVersion) {
			FrmSetDIAPolicyAttr(frmP, frmDIAPolicyCustom);
			WinSetConstraintsSize(FrmGetWindowHandle(frmP), 160, 240, 640, 160, 240, 640);
			PINSetInputTriggerState(pinInputTriggerEnabled);
		}
#ifdef HANDERA_SUPPORT
		if (isHanderaHiRes())
			VgaFormModify(frmP, vgaFormModify160To240);
#endif
		FrmDrawForm(FrmGetActiveForm());
		drawFormCustomThings();
		handled = true;
		break;

	case keyDownEvent:
	 	if (eventP->data.keyDown.chr == vchrPageUp || eventP->data.keyDown.chr == vchrPageDown)
		{
			IterateItem(eventP->data.keyDown.chr); // TODO: ADD HANDERA JOG SUPPORT AS WELL!
			handled = true;
		}

		break;

	case frmCloseEvent:
		//no matter why we're closing, free things we allocated
		FreeUsedVariables();
		break;

	case winEnterEvent:
		if (isHanderaHiRes())	//fallthrough except for handera
			break;
		//fallthrough

	default:
		break;
	}

	return handled;
}