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

static UInt32 DrawPokeIcon(UInt16 pokeID, UInt16 x, UInt16 y)
{
	MemHandle imgMemHandle;
	struct DrawState *ds;
	BitmapType *bmpP;
	MemPtr pngData;
	WinHandle win;
	UInt32 size;
	Err error;
	int ret;
	UInt32 timeStart = 0, timeEnd = 0;

	// Check if there is any image for current pkmn
	imgMemHandle = pokeImageGet(pokeID, POKE_ICON);
	if (imgMemHandle) {
		timeStart = TimGetTicks();
		if (imgDecode(&ds, MemHandleLock(imgMemHandle), MemHandleSize(imgMemHandle), POKE_ICON_SIZE, POKE_ICON_SIZE, 0))
		{
			timeEnd = TimGetTicks();
			imgDrawRedraw(ds, x, y);
		} else {
			ds = NULL;
			timeEnd = TimGetTicks();
		}

		imgDrawStateFree(ds);
		MemHandleUnlock(imgMemHandle);
		pokeImageRelease(imgMemHandle);
	}

	return timeEnd - timeStart;
}

static void DrawPokeName(UInt16 pokeID, UInt16 x, UInt16 y)
{
	char pokeName[POKEMON_NAME_LEN + 1];

	pokeNameGet(pokeName, pokeID);
	WinDrawChars(pokeName, StrLen(pokeName), x, y);
}


static void DrawIconGrid(void)
{
	UInt16 x, y;
	UInt32 topLeftPoke;
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

	x = POKE_ICON_X;
	y = POKE_ICON_Y;
	topLeftPoke = sharedVars->gridView.currentTopLeftPokemon;

	UInt32 decodeTime = 0;
	UInt32 drawTime = 0;
	UInt32 timeStart = TimGetTicks();

	for (int i = 0; i < POKE_ROWS * POKE_COLUMNS; i++) {
		if (x >= 160) {
			x = 0;
			y += POKE_ICON_SIZE + ICON_BOTTOM_MARGNIN;
		}

		decodeTime += DrawPokeIcon(i+topLeftPoke, x, y);

		x += POKE_ICON_SIZE + ICON_RIGHT_MARGIN;
	}
	drawTime = TimGetTicks() - timeStart - decodeTime;

	// Draw the names above the icons, so we have to iterate again
	x = POKE_ICON_X;
	y = POKE_ICON_Y;

	RectangleType rect;

	rect.topLeft.x = 0;
	rect.topLeft.y = y + POKE_ICON_SIZE - ICON_TEXT_OFFSET;
	rect.extent.x = 154;
	rect.extent.y = ICON_TEXT_OFFSET; 
	WinEraseRectangle(&rect, 0);
	rect.topLeft.y = rect.topLeft.y + POKE_ICON_SIZE + ICON_BOTTOM_MARGNIN;
	WinEraseRectangle(&rect, 0);
	rect.topLeft.y = rect.topLeft.y + POKE_ICON_SIZE + ICON_BOTTOM_MARGNIN;
	WinEraseRectangle(&rect, 0);

	for (int i = 0; i < POKE_ROWS * POKE_COLUMNS; i++) {
		if (x >= 160) {
			x = 0;
			y += POKE_ICON_SIZE + ICON_BOTTOM_MARGNIN;
		}

		DrawPokeName(i+topLeftPoke, x, y + POKE_ICON_SIZE - ICON_TEXT_OFFSET);

		x += POKE_ICON_SIZE + ICON_RIGHT_MARGIN;
	}

	// All this code below is just meant for testing,
	// as it will break compatibility with earlier versions
	// of Palm OS.
	Char *totalTimeStr;

	totalTimeStr = (Char *)MemPtrNew(sizeof(Char[21]));
	if ((UInt32)totalTimeStr == 0)
		return;
	MemSet(totalTimeStr, sizeof(Char[24]), 0);

	StrPrintF(totalTimeStr, "Time spent: %lu/%lu", decodeTime , drawTime);
	WinDrawChars(totalTimeStr, StrLen(totalTimeStr), 62, 2);

	MemPtrFree(totalTimeStr);

	// WinDrawLine(80, 0, 80, 160);
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
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	sharedVars->selectedPkmnId = button + sharedVars->gridView.currentTopLeftPokemon;
	FrmGotoForm(PkmnMainForm);
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

static void SetupGrid(void)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

	sharedVars->gridView.numItems = TOTAL_POKE_COUNT_ZERO_BASED;
	sharedVars->gridView.currentTopLeftPokemon = 1;
}

static void SetupScrollBar(void)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	FormPtr frm = FrmGetActiveForm();
	UInt16 numItems = sharedVars->gridView.numItems;
	UInt16 numItemsPerPage = POKE_COLUMNS;
	UInt16 scrollBarValue = sharedVars->gridView.currentTopLeftPokemon;

	SclSetScrollBar(GetObjectPtr(GridMainScrollBar), scrollBarValue, 1, numItems, numItemsPerPage);
}

// Handle the grid scrolling
static Boolean ScrollGrid(EventPtr event)
{
	SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
	sharedVars->gridView.currentTopLeftPokemon = event->data.sclRepeat.newValue;
	DrawIconGrid();
	return SclHandleEvent(event->data.sclRepeat.pScrollBar, event);
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
			SetupGrid();
			SetupScrollBar();
			DrawIconGrid();
			return true;

		// Handle scroll bar events
		case sclRepeatEvent:
			return ScrollGrid(eventP);

		case winEnterEvent:
			if (isHanderaHiRes()) //fallthrough except for handera
				break;

		default:
			break;
	}

	return false;
}