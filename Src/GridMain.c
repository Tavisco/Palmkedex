#include <PalmOS.h>

#include "BUILD_TYPE.h"
#include "Palmkedex.h"
#include "pokeInfo.h"
#include "UiResourceIDs.h"
#include "imgDraw.h"
#ifdef HANDERA_SUPPORT
#include "myTrg.h"
#endif

#define POKE_ICON_SIZE						40
#define POKE_ICON_SIZE_HANDERA				60
#define POKE_ICON_X							0
#define POKE_ICON_Y							32
#define POKE_ICON_Y_HANDERA					49
#define ICON_RIGHT_MARGIN					14
#define ICON_RIGHT_MARGIN_HANDERA			18
#define ICON_BOTTOM_MARGIN					2
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
	BitmapType *bmpP;
	MemPtr pngData;
	WinHandle win;
	UInt32 size, iconSize;
	Err error;
	int ret;

	iconSize = isHanderaHiRes() ? POKE_ICON_SIZE_HANDERA : POKE_ICON_SIZE;

	if (pokeID > TOTAL_POKE_COUNT_ZERO_BASED)
	{
		EraseRectangle(x, y, iconSize, iconSize);
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
	Int16 nameWidth, pokeNameLen, iconSize;

	if (pokeID > TOTAL_POKE_COUNT_ZERO_BASED)
		return;

	iconSize = isHanderaHiRes() ? POKE_ICON_SIZE_HANDERA : POKE_ICON_SIZE;

	pokeNameGet(pokeName, pokeID);

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
	xIncrement = POKE_ICON_SIZE + rightMargin;
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
			if (!colsCountSet)
			{
				sharedVars->gridView.cols = drawnPokeCount;
				colsCountSet = true;
				// Redraw the up button on the scroll bar to ensure it's on top
				CtlDrawControl(GetObjectPtr(GridMainScrollBtnUp));
			}
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
		DrawPokeName(pokeID, x, y + iconSize - ICON_TEXT_OFFSET);

		x += xIncrement;
		drawnPokeCount++;
	}

	// Redraw the down button on the scroll bar to ensure it's on top
	CtlDrawControl(GetObjectPtr(GridMainScrollBtnDown));
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
	RectangleType r;
	Coord extentX, extentY;

	WinGetWindowExtent(&extentX, &extentY);

	shaftLeft = extentX - GetScrollShaftWidth() - GetScrollShaftLeft();
	shaftTop = isHanderaHiRes() ? SCROLL_SHAFT_TOP_HANDERA : SCROLL_SHAFT_TOP;
	shaftHeight = extentY - shaftTop - GetScrollShaftBottomMargin();

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
	r.extent.x = GetScrollShaftWidth();
	r.extent.y = shaftHeight;
	WinFillRectangle(&r, 0);

	r.topLeft.y += curPosY * screenAvail / imgAvail;
	r.extent.y = carHeight;
	WinDrawRectangle(&r, 0);
}

static void SetupMyScrollBar(void)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	const UInt16 numItemsPerPage = sharedVars->gridView.cols * sharedVars->gridView.rows;

	UInt32 scrollBarMax = (sharedVars->sizeAfterFiltering == 0)
		? 1
		: sharedVars->sizeAfterFiltering;

	uiPrvDrawScrollCar(sharedVars->gridView.scrollOffset, scrollBarMax, numItemsPerPage);
}

static void DrawGrid(void)
{
	DrawIconsOnGrid();
	SetupMyScrollBar();
}

static void SetNewOffsetAndDraw(Int32 newScrollOffset)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	const UInt16 numItemsPerPage = sharedVars->gridView.cols * sharedVars->gridView.rows;

	// If the new scroll offset is greater than the number of pokemon, don't scroll
	if (newScrollOffset > TOTAL_POKE_COUNT_ZERO_BASED)
		return;


	// If the new scroll offset is greater than the number of filtered pokemon, don't scroll
	if (newScrollOffset + numItemsPerPage > sharedVars->sizeAfterFiltering + sharedVars->gridView.rows)
		return;

	if (newScrollOffset < 0)
		newScrollOffset = 0;

	sharedVars->gridView.scrollOffset = newScrollOffset;
	DrawGrid();
}

static void ScrollGridByButton(WChar direction, Int32 rowQtty)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	Int32 scrollQtty = sharedVars->gridView.cols * rowQtty;

	if (direction == vchrPageDown)
	{
		SetNewOffsetAndDraw(sharedVars->gridView.scrollOffset + scrollQtty);
	} else if (direction == vchrPageUp) {
		SetNewOffsetAndDraw(sharedVars->gridView.scrollOffset - scrollQtty);
	}
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
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	const UInt16 numItemsPerPage = sharedVars->gridView.cols * sharedVars->gridView.rows;
	const UInt16 itemsPerScroll = sharedVars->gridView.rows; // Scroll by 1 row
	Boolean isPenDown, handled = false;
	Int32 newScrollOffset, scrollOffsetDifference;
	Int16 lastY = 0;
	RectangleType form;
	Int16 shaftLeft, shaftHeight, shaftTop;

	shaftLeft = sharedVars->gridView.scrollShaftLeft;
	shaftHeight = sharedVars->gridView.shaftHeight;
	shaftTop = isHanderaHiRes() ? SCROLL_SHAFT_TOP_HANDERA : SCROLL_SHAFT_TOP;

	// If there are fewer mons than what our grid can show, there's nothing to scroll
	if (sharedVars->sizeAfterFiltering < numItemsPerPage)
		return false;

	UInt32 maxScrollBarValue = (sharedVars->sizeAfterFiltering == 0)
		? 1
		: sharedVars->sizeAfterFiltering;

	if (event->screenX >= shaftLeft - 6)
	{
		do {
			EvtGetPen(&event->screenX, &event->screenY, &isPenDown);
			// If the pen is still down and the Y coordinate hasn't changed by more than 5 pixels, don't do anything
			if (abs(event->screenY - lastY) <= 5)
				continue;

			if (event->screenY >= shaftTop && event->screenY <= shaftTop + shaftHeight)
			{
				// Calculate the new scroll offset while ensuring it's always a multiple of 3
				newScrollOffset = (event->screenY - shaftTop) * maxScrollBarValue / shaftHeight;
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

static Boolean SelectPokeUnderPen(EventType *event)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	const Int16 cols = sharedVars->gridView.cols;
	const Int16 rows = sharedVars->gridView.rows;
	UInt16 selectedPoke;
	Int16 rightMargin, bottomMargin, pokeIconY;

	rightMargin = isHanderaHiRes() ? ICON_RIGHT_MARGIN_HANDERA : ICON_RIGHT_MARGIN;
	bottomMargin = isHanderaHiRes() ? ICON_BOTTOM_MARGIN_HANDERA : ICON_BOTTOM_MARGIN;
	pokeIconY = isHanderaHiRes() ? POKE_ICON_Y_HANDERA : POKE_ICON_Y;

	if (event->screenX >= POKE_ICON_X && event->screenX <= sharedVars->gridView.scrollShaftLeft - 6)
	{
		if (event->screenY >= pokeIconY && event->screenY <= pokeIconY + (POKE_ICON_SIZE + bottomMargin) * rows)
		{
			Int16 clickedRow = (event->screenY - pokeIconY) / (POKE_ICON_SIZE + bottomMargin);
			Int16 clickedCol = (event->screenX - POKE_ICON_X) / (POKE_ICON_SIZE + rightMargin);

			selectedPoke = clickedRow * cols + clickedCol;
			OpenSelectedPokemon(selectedPoke);
			return true;
		}
	}

	return false;
}

static Boolean HandlePenDownEvent(EventType *event)
{
	// Nasty hack to check if the form is completely loaded
	if (FldGetTextPtr(GetObjectPtr(GridMainSearchField)) == NULL)
		return false;

	if (HandleScrollBarEvent(event))
		return true;

	if (SelectPokeUnderPen(event))
		return true;

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
			ScrollGridByButton(vchrPageUp, 1);
			handled = true;
			break;
		}
		case GridMainScrollBtnDown:
		{
			ScrollGridByButton(vchrPageDown, 1);
			handled = true;
			break;
		}
		default:
		{
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
			return HandlePenDownEvent(eventP);

		case keyDownEvent:
			if (eventP->data.keyDown.chr == vchrPageUp || eventP->data.keyDown.chr == vchrPageDown)
			{
				ScrollGridByButton(eventP->data.keyDown.chr, 2);
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