#include <PalmOS.h>

#include "BUILD_TYPE.h"
#include "Palmkedex.h"
#include "pokeInfo.h"
#include "UiResourceIDs.h"
#include "imgDraw.h"
#ifdef HANDERA_SUPPORT
#include "myTrg.h"
#endif

#define POKE_ICON_SIZE		40
#define POKE_ICON_X			0
#define POKE_ICON_Y			32
#define POKE_ROWS			3
#define POKE_COLUMNS		3
#define ICON_RIGHT_MARGIN	15
#define ICON_BOTTOM_MARGNIN	2
#define ICON_TEXT_OFFSET	9
#define SCROLL_SHAFT_TOP	42
#define SCROLL_SHAFT_HEIGHT	108

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
		pokeImageRelease(imgMemHandle);
	} else {
		DrawPokeIconPlaceholder(x, y);
	}
}

static void DrawPokeName(UInt16 pokeID, UInt16 x, UInt16 y)
{
	char pokeName[POKEMON_NAME_LEN + 1];

	if (pokeID > TOTAL_POKE_COUNT_ZERO_BASED)
		return;

	pokeNameGet(pokeName, pokeID);
	WinDrawChars(pokeName, StrLen(pokeName), x, y);
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
		if (i >= sharedVars->sizeAfterFiltering + scrollOffset)
		{
			rect.topLeft.y = rect.topLeft.y + POKE_ICON_SIZE + ICON_BOTTOM_MARGNIN;
			WinEraseRectangle(&rect, 0);
			continue;
		}

		if (x >= 160)
		{
			x = 0;
			y += POKE_ICON_SIZE + ICON_BOTTOM_MARGNIN;
			rect.topLeft.y = rect.topLeft.y + POKE_ICON_SIZE + ICON_BOTTOM_MARGNIN;
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
	UInt16 x, y;
	UInt32 topLeftPoke, scrollOffset;
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

	x = POKE_ICON_X;
	y = POKE_ICON_Y;
	topLeftPoke = sharedVars->gridView.currentTopLeftPokemon;
	scrollOffset = sharedVars->gridView.scrollOffset;

	for (UInt16 i = 0; i < POKE_ROWS * POKE_COLUMNS; i++) {
		if (x >= 160) 
		{
			x = 0;
			y += POKE_ICON_SIZE + ICON_BOTTOM_MARGNIN;
		}

		if (i >= sharedVars->sizeAfterFiltering)
		{
			ErasePokeIcon(x, y);
			x += POKE_ICON_SIZE + ICON_RIGHT_MARGIN;
			continue;
		}

		if (sharedVars->sizeAfterFiltering == TOTAL_POKE_COUNT_ZERO_BASED)
			DrawPokeIcon(i + topLeftPoke + scrollOffset, x, y);
		else
			DrawPokeIcon(sharedVars->filteredPkmnNumbers[i + scrollOffset], x, y);

		x += POKE_ICON_SIZE + ICON_RIGHT_MARGIN;
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

	if (button >= sharedVars->sizeAfterFiltering)
		return;

	if (sharedVars->sizeAfterFiltering == TOTAL_POKE_COUNT_ZERO_BASED)
	{
		selectedPoke = sharedVars->gridView.currentTopLeftPokemon + button;
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
	const UInt16 shaftLeft = 155, shaftWidth = 3, shaftTop = SCROLL_SHAFT_TOP, shaftHeight = SCROLL_SHAFT_HEIGHT;
	UInt32 imgAvail;
	CustomPatternType greyPat = {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55};
	UInt32 carHeight, screenAvail;
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	
	RectangleType r;

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
		// TODO: Erase the whole scroll bar from screen in this condition

	screenAvail = shaftHeight - carHeight;
	
	WinSetPattern(&greyPat);
	r.topLeft.x = shaftLeft;
	r.topLeft.y = shaftTop;
	r.extent.x = shaftWidth;
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
	DrawNamesOnGrid();
}

static void ScrollGrid(Int8 scrollQtty)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	Int32 newScrollOffset = sharedVars->gridView.scrollOffset + scrollQtty;

	if (newScrollOffset > TOTAL_POKE_COUNT_ZERO_BASED)
		return;


	if (newScrollOffset + POKE_COLUMNS * POKE_COLUMNS > sharedVars->sizeAfterFiltering)
		return;
		
	
	if (newScrollOffset < 0)
		return;

	sharedVars->gridView.scrollOffset = newScrollOffset;
	DrawGrid();
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

// Recieve penDownEvent and check if it is on the scroll bar
// If it is, then call uiPrvDrawScrollCar with the new position
// Return true if the event is handled, false otherwise
static Boolean HandleScrollBarEvent(EventType *eventP)
{
	Int16 x, y;
	const UInt16 numItemsPerPage = POKE_COLUMNS * POKE_COLUMNS;
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	Boolean down, handled = false;
	const Int16 scrollShaftBottom = SCROLL_SHAFT_TOP + SCROLL_SHAFT_HEIGHT;
	Int32 scrollOffset, positiveOffset;

	// If there are less than 9 pokemon, there is nothing to scroll
	if (sharedVars->sizeAfterFiltering < POKE_COLUMNS * POKE_COLUMNS)
		return false;

	x = eventP->screenX;
	y = eventP->screenY;

	UInt32 scrollBarMax = (sharedVars->sizeAfterFiltering == 0)
		? 1
		: sharedVars->sizeAfterFiltering;

	if (x >= 152 && x <= 160)
	{
		do {
			EvtGetPen(&x, &y, &down);
			if (y >= SCROLL_SHAFT_TOP && y <= scrollShaftBottom)
			{
				scrollOffset = (y - SCROLL_SHAFT_TOP) * scrollBarMax / SCROLL_SHAFT_HEIGHT;
				if (scrollOffset < POKE_COLUMNS)
					scrollOffset = 0;
				else if (scrollOffset + POKE_COLUMNS > sharedVars->sizeAfterFiltering)
					scrollOffset = sharedVars->sizeAfterFiltering - POKE_COLUMNS;

				// Only draw grid, if the scroll offset has changed at least POKE_COLUMNS
				if (sharedVars->gridView.scrollOffset - scrollOffset < 0)
				{
					positiveOffset = scrollOffset - sharedVars->gridView.scrollOffset;
				} else {
					positiveOffset = sharedVars->gridView.scrollOffset - scrollOffset;
				}

				if (positiveOffset >= POKE_COLUMNS)
				{
					sharedVars->gridView.scrollOffset = scrollOffset;
					uiPrvDrawScrollCar(sharedVars->gridView.scrollOffset, scrollBarMax, numItemsPerPage);
					DrawGrid();
					handled = true;
				}
			}
		} while (down);

		return handled;
	}

	return false;
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
			ScrollGrid(-1 * POKE_ROWS);
			handled = true;
			break;
		}
		case GridMainScrollBtnDown:
		{
			ScrollGrid(POKE_ROWS);
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
			// if (eventP->data.keyDown.chr == vchrPageUp || eventP->data.keyDown.chr == vchrPageDown)
			// {
				// TODO: Handle page up/down
			// 	return true;
			// }
			
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

		default:
			break;
	}

	return false;
}