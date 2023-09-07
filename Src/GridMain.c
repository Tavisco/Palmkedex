#include <PalmOS.h>

#include "BUILD_TYPE.h"
#include "Palmkedex.h"
#include "pokeInfo.h"
#include "UiResourceIDs.h"
#include "imgDraw.h"
#ifdef HANDERA_SUPPORT
#include "myTrg.h"
#endif

#define POKE_ICON_SIZE				40
#define POKE_ICON_X					0
#define POKE_ICON_Y					32
#define POKE_ROWS					3
#define POKE_COLUMNS				3
#define ICON_RIGHT_MARGIN			14
#define ICON_BOTTOM_MARGIN			2
#define ICON_TEXT_OFFSET			9
#define SCROLL_SHAFT_WIDTH			3
#define SCROLL_SHAFT_TOP			42
#define SCROLL_SHAFT_LEFT_MARGIN	2
#define SCROLL_SHAFT_TOP_MARGIN		10

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

static void ErasePokeIcon(UInt16 x, UInt16 y)
{
	RectangleType rect;

	rect.topLeft.x = x;
	rect.topLeft.y = y;
	rect.extent.x = POKE_ICON_SIZE;
	rect.extent.y = POKE_ICON_SIZE;
	WinEraseRectangle(&rect, 0);
}

static void DrawPokeIcon(UInt16 pokeID, UInt16 x, UInt16 y)
{
	MemHandle imgMemHandle;
	struct DrawState *ds;
	BitmapType *bmpP;
	MemPtr pngData;
	WinHandle win;
	UInt32 size;
	Err error;
	int ret;

	if (pokeID > TOTAL_POKE_COUNT_ZERO_BASED)
	{
		ErasePokeIcon(x, y);
		return;
	}

	imgMemHandle = pokeImageGet(pokeID, POKE_ICON);
	if (imgMemHandle)
	{
		if (imgDecode(&ds, MemHandleLock(imgMemHandle), MemHandleSize(imgMemHandle), POKE_ICON_SIZE, POKE_ICON_SIZE, 0))
		{
			imgDrawRedraw(ds, x, y);
		} else {
			ds = NULL;
		}

		imgDrawStateFree(ds);
		MemHandleUnlock(imgMemHandle);
	} else {
		DrawPokeIconPlaceholder(x, y);
	}

	pokeImageRelease(imgMemHandle);
}

static void DrawPokeName(UInt16 pokeID, UInt16 x, UInt16 y)
{
	char pokeName[POKEMON_NAME_LEN + 1];
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	Int16 nameWidth, pokeNameLen;

	if (pokeID > TOTAL_POKE_COUNT_ZERO_BASED)
		return;

	pokeNameGet(pokeName, pokeID);

	pokeNameLen = StrLen(pokeName);
	nameWidth = FntCharsWidth(pokeName, pokeNameLen);

	if (nameWidth >= POKE_ICON_SIZE)
	{
		// If the name is too long, truncate it
		while (nameWidth >= POKE_ICON_SIZE + ICON_RIGHT_MARGIN)
		{
			pokeNameLen--;
			nameWidth = FntCharsWidth(pokeName, pokeNameLen);
		}
	} else {
		// If the name is too short, center it
		x += ((POKE_ICON_SIZE - nameWidth) / 2);
	}

	WinDrawChars(pokeName, pokeNameLen, x, y);
}

static void DrawNamesOnGrid(void)
{
	UInt16 x, y;
	UInt32 topLeftPoke, scrollOffset;
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

	x = POKE_ICON_X;
	y = POKE_ICON_Y;
	topLeftPoke = sharedVars->gridView.currentTopLeftPokemon;
	scrollOffset = sharedVars->gridView.scrollOffset;

	RectangleType rect;

	// Erase names from first row
	rect.topLeft.x = 0;
	rect.topLeft.y = y + POKE_ICON_SIZE - ICON_TEXT_OFFSET;
	rect.extent.x = 154;
	rect.extent.y = ICON_TEXT_OFFSET + 2; 
	WinEraseRectangle(&rect, 0);

	for (UInt16 i = 0; i < POKE_ROWS * POKE_COLUMNS; i++) {
		if (i + scrollOffset >= sharedVars->sizeAfterFiltering)
		{
			rect.topLeft.y = rect.topLeft.y + POKE_ICON_SIZE + ICON_BOTTOM_MARGIN;
			WinEraseRectangle(&rect, 0);
			continue;
		}

		if (x >= 160)
		{
			x = 0;
			y += POKE_ICON_SIZE + ICON_BOTTOM_MARGIN;
			rect.topLeft.y = rect.topLeft.y + POKE_ICON_SIZE + ICON_BOTTOM_MARGIN;
			WinEraseRectangle(&rect, 0);
		}

		if (sharedVars->sizeAfterFiltering == TOTAL_POKE_COUNT_ZERO_BASED)
			DrawPokeName(i + topLeftPoke + scrollOffset, x, y + POKE_ICON_SIZE - ICON_TEXT_OFFSET);
		else
			DrawPokeName(sharedVars->filteredPkmnNumbers[i + scrollOffset], x, y + POKE_ICON_SIZE - ICON_TEXT_OFFSET);

		x += POKE_ICON_SIZE + ICON_RIGHT_MARGIN;
	}
}

static void DrawIconsOnGrid(void)
{
	Int16 x, y, pokeColumns, pokeRows, drawnPokeCount = 0, xIncrement, yIncrement;
	UInt32 topLeftPoke, scrollOffset;
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	RectangleType form;
	Boolean keepDrawing = true;

	WinGetBounds(WinGetDisplayWindow(), &form);

	x = POKE_ICON_X;
	y = POKE_ICON_Y;
	topLeftPoke = sharedVars->gridView.currentTopLeftPokemon;
	scrollOffset = sharedVars->gridView.scrollOffset;
	xIncrement = POKE_ICON_SIZE + ICON_RIGHT_MARGIN;
	yIncrement = POKE_ICON_SIZE + ICON_BOTTOM_MARGIN;

	while (keepDrawing) {
		// The 5 is to allow for some overlapping...
		if (x + xIncrement - 5 >= form.extent.x) 
		{
			x = 0;
			y += yIncrement;
		}

		if (y + yIncrement >= form.extent.y)
		{
			keepDrawing = false;
			continue;
		}

		if (drawnPokeCount + scrollOffset >= sharedVars->sizeAfterFiltering)
		{
			ErasePokeIcon(x, y);
			x += xIncrement;
			continue;
		}

		if (sharedVars->sizeAfterFiltering == TOTAL_POKE_COUNT_ZERO_BASED)
			DrawPokeIcon(drawnPokeCount + topLeftPoke + scrollOffset, x, y);
		else
			DrawPokeIcon(sharedVars->filteredPkmnNumbers[drawnPokeCount + scrollOffset], x, y);

		x += xIncrement;
		drawnPokeCount++;
	}
}

static void GridOpenAboutDialog(void)
{
	FormType * frmP;

	/* Clear the menu status from the display */
	MenuEraseStatus(0);

	/* Display the About Box. */
	frmP = FrmInitForm (AboutForm);
#ifdef HANDERA_SUPPORT
	if (isHanderaHiRes())
		VgaFormModify(frmP, vgaFormModify160To240);
#endif
	FrmDoDialog (frmP);
	FrmDeleteForm (frmP);
}

static void OpenSelectedPokemon(UInt16 button)
{
	UInt32 selectedPoke;
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	const char *searchStr = FldGetTextPtr(GetObjectPtr(GridMainSearchField));

	if (button + sharedVars->gridView.scrollOffset >= sharedVars->sizeAfterFiltering)
		return;

	if (sharedVars->sizeAfterFiltering == TOTAL_POKE_COUNT_ZERO_BASED)
	{
		selectedPoke = sharedVars->gridView.currentTopLeftPokemon + button + sharedVars->gridView.scrollOffset;
	} else {
		selectedPoke = sharedVars->filteredPkmnNumbers[button + sharedVars->gridView.scrollOffset];
	}

	if (selectedPoke > TOTAL_POKE_COUNT_ZERO_BASED)
		return;

	sharedVars->selectedPkmnId = selectedPoke;
	StrCopy(sharedVars->nameFilter, searchStr);
	FrmGotoForm(PkmnMainForm);
}

static void SetupVars(void)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

	sharedVars->gridView.currentTopLeftPokemon = (sharedVars->sizeAfterFiltering == TOTAL_POKE_COUNT_ZERO_BASED)
						? 1
						: sharedVars->filteredPkmnNumbers[0];
	
	if (!sharedVars->nameFilter)
	{
		sharedVars->gridView.scrollCarPosition = 0;
		sharedVars->gridView.scrollOffset = 0;
	}
}

static void uiPrvDrawScrollCar(UInt32 curPosY, UInt32 totalY, UInt16 viewableY)
{
	UInt16 shaftLeft, shaftTop, shaftHeight;
	UInt32 imgAvail;
	CustomPatternType greyPat = {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55};
	UInt32 carHeight, screenAvail;
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	RectangleType r, form;

	WinGetBounds(WinGetDisplayWindow(), &form);

	shaftLeft = form.extent.x - SCROLL_SHAFT_WIDTH - SCROLL_SHAFT_LEFT_MARGIN;
	shaftTop = form.topLeft.y + SCROLL_SHAFT_TOP;
	shaftHeight = form.extent.y - SCROLL_SHAFT_TOP - SCROLL_SHAFT_TOP_MARGIN;

	// Save the shaft left and height for using in HandleScrollBarEvent
	sharedVars->gridView.scrollShaftLeft = shaftLeft;
	sharedVars->gridView.shaftHeight = shaftHeight;

	// Calculate imgAvail based on the number of items in the list
	imgAvail = totalY - viewableY;
	if (imgAvail < 1)
		imgAvail = 1;

	// Calculate carHeight based on the number of items in the list
	carHeight = (viewableY * shaftHeight) / totalY;
	if (carHeight < 10)
		carHeight = 10;
	else if (carHeight > shaftHeight)
		carHeight = shaftHeight;

	screenAvail = shaftHeight - carHeight;
	
	WinSetPattern(&greyPat);
	r.topLeft.x = shaftLeft;
	r.topLeft.y = shaftTop;
	r.extent.x = SCROLL_SHAFT_WIDTH;
	r.extent.y = shaftHeight;
	WinFillRectangle(&r, 0);

	r.topLeft.y += curPosY * screenAvail / imgAvail;
	r.extent.y = carHeight;
	WinDrawRectangle(&r, 0);
}

static void SetupMyScrollBar(void)
{
	const UInt16 numItemsPerPage = POKE_COLUMNS * POKE_COLUMNS;
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

	UInt32 scrollBarMax = (sharedVars->sizeAfterFiltering == 0)
		? 1
		: sharedVars->sizeAfterFiltering;

	uiPrvDrawScrollCar(sharedVars->gridView.scrollOffset, scrollBarMax, numItemsPerPage);
}

static void DrawGrid(void)
{
	SetupMyScrollBar();
	DrawIconsOnGrid();
	//DrawNamesOnGrid();
}

static void SetNewOffsetAndDraw(Int32 newScrollOffset)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

	if (newScrollOffset > TOTAL_POKE_COUNT_ZERO_BASED)
		return;


	if (newScrollOffset + POKE_COLUMNS * POKE_COLUMNS > sharedVars->sizeAfterFiltering + POKE_COLUMNS)
		return;
		
	
	if (newScrollOffset < 0)
		return;

	sharedVars->gridView.scrollOffset = newScrollOffset;
	DrawGrid();
}

static void ScrollGridByButton(Int8 scrollQtty)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	SetNewOffsetAndDraw(sharedVars->gridView.scrollOffset + scrollQtty);
}

static void FilterAndDrawGrid(void)
{
	FilterDataSet(FldGetTextPtr(GetObjectPtr(GridMainSearchField)));
	SetupVars();
	DrawGrid();
}

static void RecoverPreviousFilter(void)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

	if (!sharedVars->nameFilter)
		return;

	SetFieldText(GridMainSearchField, sharedVars->nameFilter);
}

static void ResetScrollBar(void)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

	sharedVars->gridView.scrollCarPosition = 0;
	sharedVars->gridView.scrollOffset = 0;
}

static int abs(int x) {
	return (x < 0) ? -x : x;
}

// Recieve penDownEvent and check if it is on the scroll bar
// If it is, then call uiPrvDrawScrollCar with the new position
// Return true if the event is handled, false otherwise
static Boolean HandleScrollBarEvent(EventType *event)
{
	const UInt16 itemsPerPage = POKE_COLUMNS * POKE_COLUMNS;
	const UInt16 itemsPerScroll = POKE_COLUMNS; // Scroll by 3 items
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	Boolean isPenDown, handled = false;
	Int32 newScrollOffset, scrollOffsetDifference;
	Int16 lastY = 0;
	RectangleType form;
	Int16 shaftLeft, shaftHeight;

	shaftLeft = sharedVars->gridView.scrollShaftLeft;
	shaftHeight = sharedVars->gridView.shaftHeight;

	// If there are fewer than 9 Pokémon, there's nothing to scroll
	if (sharedVars->sizeAfterFiltering < itemsPerPage)
		return false;

	UInt32 maxScrollBarValue = (sharedVars->sizeAfterFiltering == 0)
		? 1
		: sharedVars->sizeAfterFiltering;

	if (event->screenX >= shaftLeft && event->screenX <= shaftLeft + SCROLL_SHAFT_WIDTH)
	{
		do {
			EvtGetPen(&event->screenX, &event->screenY, &isPenDown);
			// If the pen is still down and the Y coordinate hasn't changed by more than 5 pixels, don't do anything
			if (abs(event->screenY - lastY) <= 5)
				continue;

			if (event->screenY >= SCROLL_SHAFT_TOP && event->screenY <= SCROLL_SHAFT_TOP + shaftHeight)
			{
				// Calculate the new scroll offset while ensuring it's always a multiple of 3
				newScrollOffset = (event->screenY - SCROLL_SHAFT_TOP) * maxScrollBarValue / shaftHeight;
				newScrollOffset = (newScrollOffset / itemsPerScroll) * itemsPerScroll;

				// Only redraw the grid if the scroll offset has changed by at least 3 items
				scrollOffsetDifference = abs(newScrollOffset - sharedVars->gridView.scrollOffset);

				if (scrollOffsetDifference >= itemsPerScroll)
				{
					SetNewOffsetAndDraw(newScrollOffset);
					handled = true;
				}
			}

			lastY = event->screenY;
		} while (isPenDown);

		return handled;
	}

	return false;
}

static Boolean resizeGridMainForm(FormPtr fp)
{
#ifdef SCREEN_RESIZE_SUPPORT
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

		//moving a GSI is hard
		if (FrmGetObjectType(fp, idx) == frmGraffitiStateObj)
			rect.topLeft.x += newW - oldW;
		else switch (FrmGetObjectId(fp, idx)) {
			case GridMainSearchField:
				rect.extent.x += newW - oldW;
				break;

			case GridMainSearchClearButton:
			case GridMainScrollBtnUp:
				rect.topLeft.x += newW - oldW;
				break;

			case GridMainScrollBtnDown:
				rect.topLeft.x += newW - oldW;
				rect.topLeft.y += newH - oldH;
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

static Boolean GridMainFormDoCommand(UInt16 command)
{
	Boolean handled = false;

	switch (command)
	{
		case OptionsAboutPalmkedex:
		{
			GridOpenAboutDialog();
			handled = true;
			break;
		}
		case OptionsPreferences:
		{
			FrmPopupForm(PrefsForm);
			handled = true;
			break;
		}
		case GridMainSearchClearButton:
		{
			SetFieldText(GridMainSearchField, "");
			ResetScrollBar();
			FilterAndDrawGrid();
			handled = true;
			break;
		}
		case GridMainScrollBtnUp:
		{
			ScrollGridByButton(-1 * POKE_ROWS);
			handled = true;
			break;
		}
		case GridMainScrollBtnDown:
		{
			ScrollGridByButton(POKE_ROWS);
			handled = true;
			break;
		}
		default:
		{
			if (command >= 1300 && command <= 1308)
			{
				OpenSelectedPokemon(command - 1300);
				handled = true;
			}
			break;
		}
	}

	return handled;
}

Boolean GridMainFormHandleEvent(EventType * eventP)
{
	FormPtr fp = FrmGetActiveForm();
	UInt32 pinsVersion;

	switch (eventP->eType)
	{
		case menuEvent:
			return GridMainFormDoCommand(eventP->data.menu.itemID);

		case ctlSelectEvent:
			return GridMainFormDoCommand(eventP->data.ctlSelect.controlID);

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
			resizeGridMainForm(fp);
			FrmDrawForm(fp);
			RecoverPreviousFilter();
			FilterAndDrawGrid();
			return true;

		case winEnterEvent:
			if (isHanderaHiRes()) //fallthrough except for handera
				break;

		case penDownEvent:
			return HandleScrollBarEvent(eventP);

		case keyDownEvent:
			if (eventP->data.keyDown.chr == vchrPageUp || eventP->data.keyDown.chr == vchrPageDown)
			{
				// Handle page down
				if (eventP->data.keyDown.chr == vchrPageDown)
					ScrollGridByButton(POKE_ROWS * 2);
				else
					ScrollGridByButton(-1 * POKE_ROWS * 2);
			}

			//the key will change the field, but it has not yet done so
			//the way it works is that the field will be told to handle
			//the event if it is in focus, and it'l self update. It is
			//a pain to try to wait for that, so we give the Field code
			//the event now, and then update ourselves. It is important
			//to mark the event as handled, to avoid the field getting
			//it again.

			if (FrmGetFocus(fp) == FrmGetObjectIndex(fp, GridMainSearchField)) {
				FldHandleEvent(GetObjectPtr(GridMainSearchField), eventP);
				ResetScrollBar();
				FilterAndDrawGrid();
				return true;
			}
			break;

		#ifdef HANDERA_SUPPORT
			case displayExtentChangedEvent:
		#endif
		case winDisplayChangedEvent:
		case frmUpdateEvent:
			if (resizeGridMainForm(fp)) {
				WinEraseWindow();
				FrmDrawForm(fp);
				FilterAndDrawGrid();
			}
			return true;

		default:
			break;
	}

	return false;
}