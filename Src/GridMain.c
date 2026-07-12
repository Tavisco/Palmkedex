#include <PalmOS.h>
#include <stdarg.h>

#include "BUILD_TYPE.h"
#include "Palmkedex.h"
#include "pokeInfo.h"
#include "UiResourceIDs.h"
#include "imgDraw.h"
#ifdef HANDERA_SUPPORT
#include "myTrg.h"
#endif

#define POKE_ICON_SIZE						40
#define ITEM_ICON_SIZE						32
#define POKE_ICON_SIZE_HANDERA				60
#define ITEM_ICON_X							23
#define POKE_ICON_X							0
#define POKE_ICON_Y							32
#define POKE_ICON_Y_HANDERA					49
#define ICON_RIGHT_MARGIN					12
#define ICON_RIGHT_MARGIN_HANDERA			18
#define ICON_BOTTOM_MARGIN					2
#define ICON_BOTTOM_MARGIN_HANDERA			24
#define ICON_TEXT_OFFSET					10
#define SCROLL_SHAFT_WIDTH					3
#define SCROLL_SHAFT_WIDTH_HANDERA			5
#define SCROLL_SHAFT_TOP					42
#define SCROLL_SHAFT_TOP_HANDERA			63
#define SCROLL_SHAFT_LEFT_MARGIN			2
#define SCROLL_SHAFT_LEFT_MARGIN_HANDERA	3
#define SCROLL_SHAFT_BOTTOM_MARGIN			10
#define SCROLL_SHAFT_BOTTOM_MARGIN_HANDERA	15

static void debug_printf(const char* fmt, ...) {
	UInt32 ftrValue;
	char buffer[256];
	va_list args;

	if (FtrGet('cldp', 0, &ftrValue) || ftrValue != 0x20150103) return;

	va_start(args, fmt);

	if (StrVPrintF(buffer, fmt, (_Palm_va_list)args) > 255)
		DbgMessage("DebugLog: buffer overflowed, memory corruption ahead");
	else
		DbgMessage(buffer);
}

static Int16 GetScrollShaftWidth(void)
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

static 	UInt32 getIconSize(UInt8 gridType) {
	if (gridType == GRID_MODE_POKEMON) {
		return isHanderaHiRes() ? POKE_ICON_SIZE_HANDERA : POKE_ICON_SIZE;
	}
	return ITEM_ICON_SIZE;
}

static void DrawPokeIcon(UInt16 pokeID, UInt16 x, UInt16 y, UInt8 gridType)
{
	MemHandle imgMemHandle;
	struct DrawState *ds;
	const UInt32 iconSize = getIconSize(gridType);

	if (pokeID > TOTAL_POKE_COUNT_ZERO_BASED)
	{
		EraseRectangle(x, y, iconSize, iconSize);
		return;
	}

	UInt8 imgType = gridType == GRID_MODE_POKEMON? POKE_ICON : ITEM_ICON;
	uint32_t expectedSize =  gridType == GRID_MODE_POKEMON? POKE_ICON_SIZE : ITEM_ICON_SIZE;

	imgMemHandle = aciImageGet(pokeID, imgType);
	if (imgMemHandle)
	{
		if (imgDecode(&ds, MemHandleLock(imgMemHandle), MemHandleSize(imgMemHandle), expectedSize, expectedSize, 0))
		{
			imgDrawRedraw(ds, x, y);
			imgDrawStateFree(ds);
		}
		MemHandleUnlock(imgMemHandle);
	} else {
		if (gridType == GRID_MODE_ITEMS) {
			x -= 4;
		}
		DrawPokeIconPlaceholder(x, y);
	}

	pokeImageRelease(imgMemHandle, imgType);
}

static void DrawPokeName(UInt16 pokeID, Int16 x, UInt16 y, UInt8 gridMode)
{
	char pokeName[21];
	Int16 nameWidth, pokeNameLen;

	if (pokeID > TOTAL_POKE_COUNT_ZERO_BASED)
		return;

	if (gridMode == GRID_MODE_POKEMON) {
		pokeNameGet(pokeName, pokeID);
	} else if (gridMode == GRID_MODE_ITEMS) {
		itemNameGet(pokeName, pokeID);
	}

	pokeNameLen = StrLen(pokeName);
	nameWidth = FntCharsWidth(pokeName, pokeNameLen);

	Coord extentX, extentY;
	WinGetWindowExtent(&extentX, &extentY);

	SharedVariables *sharedVars = globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

	Int16 colWidht = extentX / sharedVars->gridView.cols;
	Int16 index = x / colWidht;
	Int16 colStart = index * colWidht;

	Int16 iconSize = getIconSize(gridMode);
	Int16 idealX = x + ((iconSize - nameWidth) / 2);

	if (nameWidth > colWidht) {
		// The name is wider than the column!
		x = colStart;

		// Truncate one character at a time until it fits inside colWidht
		while (nameWidth > colWidht && pokeNameLen > 0) {
			pokeNameLen--;
			nameWidth = FntCharsWidth(pokeName, pokeNameLen);
		}
	} else {
		// The name fits! Start at the ideal centered position under the icon
		x = idealX;
	}

	// Clamp it down
	if (x <= 0)
		x = 0;

    WinDrawChars(pokeName, pokeNameLen, x, y);
}

static void DrawIconsOnGrid(void)
{
Int16 x, y, drawnPokeCount, iconSize, pokeID;
    UInt16 yBaseline;
    Int16 numCols, numRows, maxItems, colWidth, rowHeight, gridWidth, availY, minColWidth, minRowHeight;
    UInt32 topLeftPoke, scrollOffset;
    SharedVariables *sharedVars = globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
    Coord extentX, extentY;
    Boolean adventureModeEnabled;
    UInt8 adventureStatus;

    // Setup variables
    WinGetWindowExtent(&extentX, &extentY);
    yBaseline = isHanderaHiRes() ? POKE_ICON_Y_HANDERA : POKE_ICON_Y;
    iconSize = getIconSize(sharedVars->gridView.mode);

    topLeftPoke = sharedVars->gridView.currentTopLeftPokemon;
    scrollOffset = sharedVars->gridView.scrollOffset;
    adventureModeEnabled = isAdventureModeEnabled();

    // Erase whole grid area
    RectangleType rect;
    rect.topLeft.x = 0;
    rect.topLeft.y = yBaseline;
    rect.extent.x = extentX - GetScrollShaftWidth() - GetScrollShaftLeft() - 2;
    rect.extent.y = extentY;
    WinEraseRectangle(&rect, 0);

    // Calculate grid parameters
    gridWidth = extentX - GetScrollShaftWidth();
    availY = extentY - yBaseline;

    // Calculate minimum cell sizes
	if (sharedVars->gridView.mode == GRID_MODE_ITEMS) {
		minColWidth = iconSize * 2;
	} else {
		minColWidth = iconSize + ICON_RIGHT_MARGIN;
	}

    minRowHeight = iconSize + ICON_BOTTOM_MARGIN;

    // Determine how many columns and rows fit on screen
    numCols = gridWidth / minColWidth;
    if (numCols <= 0) numCols = 1;

    numRows = availY / minRowHeight;
    if (numRows <= 0) numRows = 1;

    // Determine spacing
    colWidth = gridWidth / numCols;
    rowHeight = availY / numRows;

    // Save grid dimensions
    sharedVars->gridView.cols = numCols;
    sharedVars->gridView.rows = numRows;

    maxItems = numCols * numRows;

    for (drawnPokeCount = 0; drawnPokeCount < maxItems; drawnPokeCount++) {
        // We've reached the end of the filtered pokemon/item list
        if (drawnPokeCount + scrollOffset >= sharedVars->sizeAfterFiltering)
        {
            break;
        }

        Int16 col = drawnPokeCount % numCols;
        Int16 row = drawnPokeCount / numCols;

        // Center the icon inside its calculated column cell
        x = (col * colWidth) + ((colWidth - iconSize) / 2);

        // Pin icon to the top of its row cell to leave room for text underneath
        y = yBaseline + (row * rowHeight);

        // Resolve ID
        if (sharedVars->sizeAfterFiltering == TOTAL_POKE_COUNT_ZERO_BASED || sharedVars->sizeAfterFiltering == TOTAL_ITEM_COUNT_ZERO_BASED)
        {
            pokeID = drawnPokeCount + topLeftPoke + scrollOffset;
        } else {
            pokeID = sharedVars->filteredPkmnNumbers[drawnPokeCount + scrollOffset];
        }

        adventureStatus = getPokeAdventureStatus(pokeID);

        // Draw Icon
        if (!adventureModeEnabled || (adventureModeEnabled && adventureStatus != POKE_ADVENTURE_NOT_SEEN))
        {
            DrawPokeIcon(pokeID, x, y, sharedVars->gridView.mode);
        } else {
            DrawPokeIconPlaceholder(x, y);
        }

        // Draw Name
    	const UInt16 textYOffset = sharedVars->gridView.mode == GRID_MODE_POKEMON ? ICON_TEXT_OFFSET : 0;
        DrawPokeName(pokeID, x, y + iconSize - textYOffset, sharedVars->gridView.mode);
    }

    debug_printf("rows: %i, cols %i", sharedVars->gridView.rows, sharedVars->gridView.cols);

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

	if (sharedVars->sizeAfterFiltering == TOTAL_POKE_COUNT_ZERO_BASED || sharedVars->sizeAfterFiltering == TOTAL_ITEM_COUNT_ZERO_BASED)
	{
		selectedPoke = sharedVars->gridView.currentTopLeftPokemon + button + sharedVars->gridView.scrollOffset;
	} else {
		selectedPoke = sharedVars->filteredPkmnNumbers[button + sharedVars->gridView.scrollOffset];
	}

	if (sharedVars->gridView.mode == GRID_MODE_POKEMON && selectedPoke > TOTAL_POKE_COUNT_ZERO_BASED) {
		return;
	}

	if (sharedVars->gridView.mode == GRID_MODE_ITEMS && selectedPoke > TOTAL_ITEM_COUNT_ZERO_BASED) {
		return;
	}

	sharedVars->selectedPkmnId = selectedPoke;
	if (searchStr != NULL)
	{
		StrCopy(sharedVars->nameFilter, searchStr);
	}

	if (sharedVars->gridView.mode == GRID_MODE_ITEMS) {
		FrmGotoForm(ItemsForm);
		return;
	}
	FrmGotoForm(PkmnMainForm);
}

static void SetupVars(void)
{
	SharedVariables *sharedVars = globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

	sharedVars->gridView.currentTopLeftPokemon = 1;

	if (sharedVars->gridView.mode == GRID_MODE_ITEMS) {
		sharedVars->gridView.currentTopLeftPokemon = (sharedVars->sizeAfterFiltering == TOTAL_ITEM_COUNT_ZERO_BASED)
							? 1
							: sharedVars->filteredPkmnNumbers[0];
	} else if (sharedVars->gridView.mode == GRID_MODE_POKEMON) {
		sharedVars->gridView.currentTopLeftPokemon = (sharedVars->sizeAfterFiltering == TOTAL_POKE_COUNT_ZERO_BASED)
							? 1
							: sharedVars->filteredPkmnNumbers[0];
	}

	if (!sharedVars->nameFilter[0])
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

	// If we don't need a scrollBar, remove it!
	if (totalY <= viewableY) {
		EraseRectangle(shaftLeft - 2, shaftTop - 9, 160, 160);
		return;
	}

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

	CtlDrawControl(GetObjectPtr(GridMainScrollBtnUp));
	CtlDrawControl(GetObjectPtr(GridMainScrollBtnDown));
}

static void SetupMyScrollBar(void)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	// cols and rows are 0 based
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
	Int32 scrollQtty = sharedVars->gridView.rows * rowQtty;

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
	Boolean foundPrefs;
	struct PalmkedexPrefs *prefs;
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	UInt16 latestPrefSize;

	latestPrefSize = sizeof(struct PalmkedexPrefs);

	prefs = MemPtrNew(latestPrefSize);
	if (!prefs)
	{
		SysFatalAlert("Failed to allocate memory to store preferences!");
		MemPtrFree(prefs);
		return;
	}
	MemSet(prefs, latestPrefSize, 0);

	foundPrefs = PrefGetAppPreferencesV10(appFileCreator, appPrefVersionNum, prefs, latestPrefSize);
	if (!foundPrefs)
	{
		ErrAlertCustom(0, "Failed to load preferences! Cannot recover search.", NULL, NULL);
		MemPtrFree(prefs);
		return;
	}

	if (prefs->shouldRememberSearch){
		SetFieldText(GridMainSearchField, sharedVars->nameFilter);
	} else {
		SetFieldText(GridMainSearchField, "");
	}

	MemPtrFree(prefs);
	return;
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
	const UInt16 itemsPerScroll = sharedVars->gridView.cols; // Scroll by 1 row
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
	SharedVariables *sharedVars = globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	const Int16 cols = sharedVars->gridView.cols;
	UInt16 selectedPoke;
	Coord height, width;
	Int16 yBaseline;
	Int16 gridWidth, availY, minColWidth, minRowHeight;
	Int16 numCols, numRows, colWidth, rowHeight, iconSize;

	WinGetWindowExtent(&width, &height);

	yBaseline = isHanderaHiRes() ? POKE_ICON_Y_HANDERA : POKE_ICON_Y;
	iconSize = isHanderaHiRes() ? POKE_ICON_SIZE_HANDERA : POKE_ICON_SIZE;

	// Check if the tap is within the overall grid boundaries (excluding scrollbar and header)
	if (event->screenX >= 0 && event->screenX < (width - GetScrollShaftWidth()) && event->screenY >= yBaseline)
	{
		gridWidth = width - GetScrollShaftWidth();
		availY = height - yBaseline;

		minColWidth = iconSize + (isHanderaHiRes() ? ICON_RIGHT_MARGIN_HANDERA : ICON_RIGHT_MARGIN);
		if (sharedVars->gridView.mode == GRID_MODE_ITEMS) {
			minColWidth += 14;
		}
		minRowHeight = iconSize + (isHanderaHiRes() ? ICON_BOTTOM_MARGIN_HANDERA : ICON_BOTTOM_MARGIN);

		numCols = gridWidth / minColWidth;
		if (numCols <= 0) numCols = 1;

		numRows = availY / minRowHeight;
		if (numRows <= 0) numRows = 1;

		// Calculate exact cell dimensions
		colWidth = gridWidth / numCols;
		rowHeight = availY / numRows;

		// Calculate exact clicked row and column based on uniform cell sizes
		Int16 clickedCol = event->screenX / colWidth;
		Int16 clickedRow = (event->screenY - yBaseline) / rowHeight;

		// Check if we select an actual pokemon
		if (clickedCol >= numCols || clickedRow >= numRows) {
			return false;
		}

		// Calculate absolute index
		selectedPoke = (clickedRow * cols) + clickedCol;

		OpenSelectedPokemon(selectedPoke);
		return true;
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
		case popSelectEvent:
			if (eventP->data.popSelect.selection == 1)
			{
				SharedVariables *sharedVars = globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
				sharedVars->gridView.mode = GRID_MODE_ITEMS;
				SetFieldText(GridMainSearchField, "");
				ResetScrollBar();
				FilterAndDrawGrid();
			} else if (eventP->data.popSelect.selection == 0) {
				SharedVariables *sharedVars = globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
				sharedVars->gridView.mode = GRID_MODE_POKEMON;
				SetFieldText(GridMainSearchField, "");
				ResetScrollBar();
				FilterAndDrawGrid();
			}
			break;
		default:
			break;
	}

	return false;
}
