#include <PalmOS.h>

#include "BUILD_TYPE.h"
#include "Palmkedex.h"
#include "pokeInfo.h"
#include "UiResourceIDs.h"
#include "imgDraw.h"
#ifdef HANDERA_SUPPORT
#include "myTrg.h"
#endif

#define POKE_ICON_SIZE	40
#define POKE_ICON_X		0
#define POKE_ICON_Y		32
#define POKE_ROWS		3
#define POKE_COLUMNS	3
#define ICON_MARGIN		17

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
	UInt16 x, y, tempPokeCount;

	x = POKE_ICON_X;
	y = POKE_ICON_Y;

	UInt32 decodeTime = 0;
	UInt32 drawTime = 0;
	UInt32 timeStart = TimGetTicks();

	for (int i = 1; i <= POKE_ROWS * POKE_COLUMNS; i++) {
		if (x >= 160) {
			x = 0;
			y += POKE_ICON_SIZE;
		}

		decodeTime += DrawPokeIcon(i, x, y);

		x += POKE_ICON_SIZE + ICON_MARGIN;
	}
	drawTime = TimGetTicks() - timeStart - decodeTime;

	// Draw the names above the icons, so we have to iterate again
	x = POKE_ICON_X;
	y = POKE_ICON_Y;

	for (int i = 1; i <= POKE_ROWS * POKE_COLUMNS; i++) {
		if (x >= 160) {
			x = 0;
			y += POKE_ICON_SIZE;
		}

		DrawPokeName(i, x, y + POKE_ICON_SIZE-5);

		x += POKE_ICON_SIZE + ICON_MARGIN;
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
			DrawIconGrid();
			return true;

		case winEnterEvent:
			if (isHanderaHiRes()) //fallthrough except for handera
				break;

		default:
			break;
	}

	return false;
}