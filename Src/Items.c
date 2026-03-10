#include <PalmOS.h>

#include "BUILD_TYPE.h"
#include "imgDraw.h"
#include "Palmkedex.h"
#include "pokeInfo.h"
#include "UiResourceIDs.h"
#ifdef HANDERA_SUPPORT
#include "myTrg.h"
#endif


#define POKE_ICON_SIZE						32
#define POKE_ICON_SIZE_HANDERA				60
#define POKE_ICON_X							0
#define POKE_ICON_Y							32
#define POKE_ICON_Y_HANDERA					49
#define ICON_RIGHT_MARGIN					23
#define ICON_RIGHT_MARGIN_HANDERA			18
#define ICON_BOTTOM_MARGIN					8
#define ICON_BOTTOM_MARGIN_HANDERA			24
#define ICON_TEXT_OFFSET					8
#define SCROLL_SHAFT_WIDTH					3
#define SCROLL_SHAFT_WIDTH_HANDERA			5
#define SCROLL_SHAFT_TOP					42
#define SCROLL_SHAFT_TOP_HANDERA			63
#define SCROLL_SHAFT_LEFT_MARGIN			2
#define SCROLL_SHAFT_LEFT_MARGIN_HANDERA	3
#define SCROLL_SHAFT_BOTTOM_MARGIN			10
#define SCROLL_SHAFT_BOTTOM_MARGIN_HANDERA	15

static UInt16 GetScrollShaftWidth(void)
{
	return isHanderaHiRes() ? SCROLL_SHAFT_WIDTH_HANDERA : SCROLL_SHAFT_WIDTH;
}

static UInt16 GetScrollShaftLeft(void)
{
	return isHanderaHiRes() ? SCROLL_SHAFT_LEFT_MARGIN_HANDERA : SCROLL_SHAFT_LEFT_MARGIN;
}

static UInt16 GetScrollShaftBottomMargin(void)
{
	return isHanderaHiRes() ? SCROLL_SHAFT_BOTTOM_MARGIN_HANDERA : SCROLL_SHAFT_BOTTOM_MARGIN;
}

static void DrawPokeIconPlaceholder(UInt16 x, UInt16 y)
{
	MemHandle h;
	BitmapPtr bitmapP;
	h = DmGetResource(bitmapRsc, BmpMissingIcon);

	bitmapP = (BitmapPtr)MemHandleLock(h);

	WinDrawBitmap(bitmapP, x, y);
	MemPtrUnlock(bitmapP);
	DmReleaseResource(h);
}

static void EraseRectangle(UInt16 x, UInt16 y, UInt16 extentX, UInt16 extentY)
{
	RectangleType rect;

	rect.topLeft.x = x;
	rect.topLeft.y = y;
	rect.extent.x = extentX;
	rect.extent.y = extentY;
	WinEraseRectangle(&rect, 0);
}

static void DrawPokeIcon(UInt16 pokeID, UInt16 x, UInt16 y)
{
	MemHandle imgMemHandle;
	struct DrawState *ds;
	UInt32 iconSize;

	iconSize = isHanderaHiRes() ? POKE_ICON_SIZE_HANDERA : POKE_ICON_SIZE;

	if (pokeID > TOTAL_POKE_COUNT_ZERO_BASED)
	{
		EraseRectangle(x, y, iconSize, iconSize);
		return;
	}

	imgMemHandle = pokeImageGet(pokeID, ITEM_ICON);
	if (imgMemHandle)
	{
		if (imgDecode(&ds, MemHandleLock(imgMemHandle), MemHandleSize(imgMemHandle), POKE_ICON_SIZE, POKE_ICON_SIZE, 0))
		{
			imgDrawRedraw(ds, x, y);
			imgDrawStateFree(ds);
		}
		MemHandleUnlock(imgMemHandle);
	} else {
		DrawPokeIconPlaceholder(x, y);
	}

	pokeImageRelease(imgMemHandle, ITEM_ICON);
	// imgDrawStateFree(ds);
	// *globalsSlotPtr(GLOBALS_SLOT_POKE_IMAGE) = NULL;
}

static void DrawPokeName(UInt16 pokeID, UInt16 x, UInt16 y)
{
	char pokeName[POKEMON_NAME_LEN + 1];
	Int16 nameWidth, pokeNameLen, iconSize;

	if (pokeID > TOTAL_POKE_COUNT_ZERO_BASED)
		return;

	iconSize = isHanderaHiRes() ? POKE_ICON_SIZE_HANDERA : POKE_ICON_SIZE;

	itemNameGet(pokeName, pokeID);

	pokeNameLen = StrLen(pokeName);
	nameWidth = FntCharsWidth(pokeName, pokeNameLen);

	if (nameWidth >= iconSize)
	{
		// If the name is too long, truncate it
		while (nameWidth >= iconSize + ICON_RIGHT_MARGIN)
		{
			pokeNameLen--;
			nameWidth = FntCharsWidth(pokeName, pokeNameLen);
		}
	} else {
		// If the name is too short, center it
		x += ((iconSize - nameWidth) / 2);
	}

    WinDrawChars(pokeName, pokeNameLen, x, y);
}

static void DrawIconsOnGrid(void)
{
	Int16 x, y, rows, drawnPokeCount = 0, xIncrement, yIncrement, bottomMargin, rightMargin, iconSize, pokeID;
	UInt32 topLeftPoke, scrollOffset;
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	Coord extentX, extentY;
	Boolean keepDrawing = true, colsCountSet = false;

	// Setup variables
	WinGetWindowExtent(&extentX, &extentY);
	x = POKE_ICON_X;
	y = isHanderaHiRes() ? POKE_ICON_Y_HANDERA : POKE_ICON_Y;
	topLeftPoke = sharedVars->gridView.currentTopLeftPokemon;
	scrollOffset = sharedVars->gridView.scrollOffset;
	rightMargin = isHanderaHiRes() ? ICON_RIGHT_MARGIN_HANDERA : ICON_RIGHT_MARGIN;
	bottomMargin = isHanderaHiRes() ? ICON_BOTTOM_MARGIN_HANDERA : ICON_BOTTOM_MARGIN;
	xIncrement = POKE_ICON_SIZE + rightMargin - GetScrollShaftWidth();
	yIncrement = POKE_ICON_SIZE + bottomMargin;
	rows = 0;
	iconSize = isHanderaHiRes() ? POKE_ICON_SIZE_HANDERA : POKE_ICON_SIZE;

	// Erase names from first row
	RectangleType rect;
	rect.topLeft.x = 0;
	rect.topLeft.y = y + iconSize - ICON_TEXT_OFFSET;
	rect.extent.x = extentX - GetScrollShaftWidth() - GetScrollShaftLeft() - 2;
	rect.extent.y = ICON_TEXT_OFFSET + 2;
	WinEraseRectangle(&rect, 0);

	while (keepDrawing) {
		// The 5 is to allow for some overlapping...
		if (x + xIncrement - 5 >= extentX)
		{
			// We've reached the end of the row
			x = 0;
			y += yIncrement;
			// Erase names for the next row
			RectangleType rect;
			rect.topLeft.x = 0;
			rect.topLeft.y = y + iconSize - ICON_TEXT_OFFSET;
			rect.extent.x = extentX - GetScrollShaftWidth() - GetScrollShaftLeft() - 2;
			rect.extent.y = ICON_TEXT_OFFSET + 2;
			WinEraseRectangle(&rect, 0);
			// if (!colsCountSet)
			// {
			// 	sharedVars->gridView.cols = drawnPokeCount;
			// 	colsCountSet = true;
			// 	// Redraw the up button on the scroll bar to ensure it's on top
			// 	CtlDrawControl(GetObjectPtr(GridMainScrollBtnUp));
			// }
			rows++;
		}

		if (y >= extentY)
		{
			// We've reached the bottom of the screen
			keepDrawing = false;
			sharedVars->gridView.rows = rows;
			continue;
		}

		if (drawnPokeCount + scrollOffset >= sharedVars->sizeAfterFiltering)
		{
			// We've reached the end of the filtered pokemon list
			EraseRectangle(x, y, iconSize, iconSize + ICON_TEXT_OFFSET);
			x += xIncrement;
			continue;
		}

		if (sharedVars->sizeAfterFiltering == TOTAL_POKE_COUNT_ZERO_BASED)
		{
			pokeID = drawnPokeCount + topLeftPoke + scrollOffset;
		} else {
			pokeID = sharedVars->filteredPkmnNumbers[drawnPokeCount + scrollOffset];
		}


		DrawPokeIcon(pokeID, x, y);


		// DrawPokeName(pokeID, x, y + iconSize - ICON_TEXT_OFFSET);

		x += xIncrement;
		drawnPokeCount++;
	}

	// Redraw the down button on the scroll bar to ensure it's on top
	CtlDrawControl(GetObjectPtr(GridMainScrollBtnDown));
}

// static void DrawItemSprite(UInt16 selectedPkmnId, Coord y)
// {
//     struct DrawState *ds;
//
//     MemHandle imgMemHandle = pokeImageGet(selectedPkmnId, ITEM_ICON);
//     if (imgMemHandle) {
//         if (imgDecode(&ds, MemHandleLock(imgMemHandle), MemHandleSize(imgMemHandle), 32, 32, 0))
//             imgDrawRedraw(ds, 32, y + 32);
//         else
//             ds = NULL;
//         MemHandleUnlock(imgMemHandle);
//         pokeImageRelease(imgMemHandle, ITEM_ICON);
//
//             imgDrawStateFree(ds);
//             *globalsSlotPtr(GLOBALS_SLOT_POKE_IMAGE) = NULL;
//     }
// }

// static void PokemonItemsDraw(Int16 listIndex, RectangleType *bounds, Char **sharedVarsPtr)
// {
//     SharedVariables *sharedVars = (SharedVariables*)sharedVarsPtr;
//     char pokeName[33];
//     UInt16 pokeNum;
//
//     if (sharedVars->sizeAfterFiltering == TOTAL_POKE_COUNT_ZERO_BASED)
//         pokeNum = listIndex + 1;
//     else
//         pokeNum = sharedVars->filteredItemNumbers[listIndex];
//
//     if (pokeNum == MAX_SEARCH_PKMN_NUM)
//         StrCopy(pokeName, MAX_SEARCH_STR);
//     else
//         itemNameGet(pokeName, pokeNum);
//
//     WinDrawChars(pokeName, StrLen(pokeName), bounds->topLeft.x, bounds->topLeft.y);
//     DrawItemSprite(pokeNum, bounds->topLeft.y);
// }

static Boolean myCaselessStringNcmp(const char *as, const char *bs, UInt16 len)
{
    while (len--) {

        char ac = *as++;
        char bc = *bs++;

        if (ac >= 'a' && ac <= 'z')
            ac += 'A' - 'a';
        if (bc >= 'a' && bc <= 'z')
            bc += 'A' - 'a';

        if (ac != bc)
            return false;
    }

    return true;
}

void FilterItemDataSet(const char *searchStr)
{
    SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
    UInt16 i;


    if (!searchStr || !searchStr[0]) {	//no search

        sharedVars->sizeAfterFiltering = TOTAL_POKE_COUNT_ZERO_BASED;
    }
    else {								//we have a search

        UInt16 potentialItemID;
        char firstLetter;

        //find the first letter of the search, uppercase it, verify it IS a letter
        firstLetter = searchStr[0];
        if (firstLetter >= 'a' && firstLetter <= 'z')
            firstLetter += 'A' - 'a';

        if (firstLetter < 'A' || firstLetter > 'Z') {	//not a letter - no pokemon names match!
            sharedVars->sizeAfterFiltering = 0;
        } else {
            const UInt16 *potentialMatches = sharedVars->ItemIdsPerEachStartingLetter[firstLetter - 'A'];
            UInt16 L = StrLen(searchStr), matchCount = 0;

            //check each
            for (i = 0; (potentialItemID = potentialMatches[i]) != 0; i++) {

                char potentialItemName[33];

                itemNameGet(potentialItemName, potentialItemID);

                if (myCaselessStringNcmp(potentialItemName, searchStr, L)) {

                    sharedVars->filteredItemNumbers[matchCount] = potentialItemID;
                    matchCount++;

                    if (matchCount == MAX_SEARCH_RESULT_LEN)
                    {
                        sharedVars->filteredItemNumbers[MAX_SEARCH_RESULT_LEN - 1] = MAX_SEARCH_PKMN_NUM;
                        break;
                    }
                }
            }
            sharedVars->sizeAfterFiltering = matchCount;
        }
    }
}

// static void UpdateItemList(void)
// {
//     SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
//     FormPtr fp = FrmGetActiveForm();
//     ListType *list;
//
//     FilterItemDataSet(FldGetTextPtr(GetObjectPtr(ItemsSearchField)));
//     list = GetObjectPtr(ItemsSearchList);
//     // Set custom list drawing callback function.
//     LstSetDrawFunction(list, PokemonItemsDraw);
//     // Set list item number. pass "shared variables" as text - it can be quickly retrieved the the draw code (faster than FtrGet)
//     LstSetListChoices(list, (char**)sharedVars, sharedVars->sizeAfterFiltering);
//     if (sharedVars->sizeAfterFiltering > 0)
//         LstSetTopItem(list, 0);
//     LstSetSelection(list, -1);
//     LstDrawList(list);
//     FrmSetFocus(fp, FrmGetObjectIndex(fp, ItemsSearchField));
// }

static Boolean IsSelectionValid(UInt16 selection)
{
    return selection != MAX_SEARCH_PKMN_NUM;
}

static void calcPokemonNumberWidth(void)
{
    SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
    UInt8 i, maxWidth = 0;
    FontID oldFont;
    char ch[5];

    //calculate list font width (palmos has no kerning)
    oldFont = FntSetFont(boldFont);
    ch[4] = '#';
    for (i = 0; i < 10; i++) {
        UInt8 nowWidth;

        ch[0] = ch[1] = ch[2] = ch[3] = '0' + i;
        nowWidth = FntCharsWidth(ch, 5);

        if (nowWidth > maxWidth)
            maxWidth = nowWidth;
    }
    sharedVars->listNumsWidth = maxWidth + 2;
    FntSetFont(oldFont);
}

void ShowItemDetails(Int16 selection)
{
    SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
    const char *searchStr = FldGetTextPtr(GetObjectPtr(ItemsSearchField));
    UInt16 selectedPkmn;
    MemHandle hndl;
    char *dexEntry = NULL;
    ListType *list;

    selectedPkmn = GetItemId(selection);

    if (IsSelectionValid((UInt16) selectedPkmn))
    {
        DmOpenRef dbRef = DmOpenDatabaseByTypeCreator('ITEM', appFileCreator, dmModeReadOnly);
        if (!dbRef)
        {
            ErrFatalDisplay("Failed to find item database!");
            return;
        }

        hndl = DmGet1Resource('DESC', 0);
        dexEntry = pokeDescrGet(hndl, selectedPkmn);

        if (dexEntry == NULL)
        {
            FrmCustomAlert(DexEntryAlert, "oops", "", "");
            return;
        }

        FrmCustomAlert(DexEntryAlert, dexEntry, " ", "");
        MemPtrFree(dexEntry);
        DmCloseDatabase(dbRef);
    } else {
        FrmAlert (InvalidPokemonAlert);
    }
}

UInt16 GetItemId(Int16 selection)
{
    SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

    if (sharedVars->sizeAfterFiltering == TOTAL_POKE_COUNT_ZERO_BASED)
        return selection + 1;
    else
        return sharedVars->filteredItemNumbers[selection];
}

static Boolean ItemsFormDoCommand(UInt16 command)
{
    Boolean handled = false;

    switch (command)
    {
        case OptionsAboutPalmkedex:
        {
            OpenAboutDialog();
            handled = true;
            break;
        }
        case OptionsPreferences:
        {
            FrmPopupForm(PrefsForm);
            handled = true;
            break;
        }
        case ItemsSearchClearButton:
        {
            SetFieldText(ItemsSearchField,"");
            handled = true;
            break;
        }
    }

    return handled;
}

static Boolean resizeItemsForm(FormPtr fp)
{
    #ifdef SCREEN_RESIZE_SUPPORT
    WinHandle wh = FrmGetWindowHandle(fp);
    Coord newW, newH, oldW, oldH;
    RectangleType rect;
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

        //moving a GSI is hard
        if (FrmGetObjectType(fp, idx) == frmGraffitiStateObj)
            rect.topLeft.x += newW - oldW;
        else switch (FrmGetObjectId(fp, idx)) {
        case ItemsSearchField:
            rect.extent.x += newW - oldW;
            break;

        case ItemsSearchClearButton:
            rect.topLeft.x += newW - oldW;
            break;

        default:
            continue;
        }

        FrmSetObjectBounds(fp, idx, &rect);
    }

    return true;
    #else
    return false;
    #endif
}

Boolean ItemsFormHandleEvent(EventType * eventP)
{
    FormPtr fp = FrmGetActiveForm();
    UInt32 pinsVersion;

    switch (eventP->eType)
    {
        case menuEvent:
            return ItemsFormDoCommand(eventP->data.menu.itemID);

        case ctlSelectEvent:
            return ItemsFormDoCommand(eventP->data.ctlSelect.controlID);

        case frmOpenEvent:
            if (errNone == FtrGet(pinCreator, pinFtrAPIVersion, &pinsVersion) && pinsVersion) {
                FrmSetDIAPolicyAttr(fp, frmDIAPolicyCustom);
                WinSetConstraintsSize(FrmGetWindowHandle(fp), 160, 240, 640, 160, 240, 640);
                PINSetInputTriggerState(pinInputTriggerEnabled);
            }
            #ifdef HANDERA_SUPPORT
            if (isHanderaHiRes())
                VgaFormModify(fp, vgaFormModify160To240);
        #endif
        resizeItemsForm(fp);
        calcPokemonNumberWidth();
        FrmDrawForm(fp);
    	DrawIconsOnGrid();
        // UpdateItemList();
        return true;

        case lstSelectEvent:
            ShowItemDetails(eventP->data.lstSelect.selection);
            break;

        case keyDownEvent:
            if (eventP->data.keyDown.chr == vchrPageUp || eventP->data.keyDown.chr == vchrPageDown)
            {
                // ScrollSearchList(eventP->data.keyDown.chr); // TODO: ADD HANDERA JOG SUPPORT AS WELL!
                return true;
            }

            //the key will change the field, but it has not yet done so
            //the way it works is that the field will be told to handle
            //the event if it is in focus, and it'l self update. It is
            //a pain to try to wait for that, so we give the Field code
            //the event now, and then update ourselves. It is important
            //to mark the event as handled, to avoid the field getting
            //it again.

            if (FrmGetFocus(fp) == FrmGetObjectIndex(fp, ItemsSearchField)) {

                FldHandleEvent(GetObjectPtr(ItemsSearchField), eventP);
                // UpdateItemList();
                return true;
            }
            break;

        case winEnterEvent:
            if (isHanderaHiRes()) //fallthrough except for handera
                break;

        #ifdef HANDERA_SUPPORT
        case displayExtentChangedEvent:
            #endif
        case winDisplayChangedEvent:
        case frmUpdateEvent:
            if (resizeItemsForm(fp)) {
                WinEraseWindow();
                FrmDrawForm(fp);
            }
            return true;
        case popSelectEvent:
			if (eventP->data.popSelect.selection == 0)
			{
				FrmGotoForm(GridMainForm);
			}
			break;
        default:
            break;
    }

    return false;
}

