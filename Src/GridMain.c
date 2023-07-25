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

	// Check if there is any image for current pkmn
	imgMemHandle = pokeImageGet(pokeID, POKE_ICON);
	if (imgMemHandle) {
		if (imgDecode(&ds, MemHandleLock(imgMemHandle), MemHandleSize(imgMemHandle), POKE_ICON_SIZE, POKE_ICON_SIZE, 0))
			imgDrawRedraw(ds, x, y);
		else
			ds = NULL;
		MemHandleUnlock(imgMemHandle);
		pokeImageRelease(imgMemHandle);
	}
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

	x = 0;
	y = 32;

	for (int i = 1; i <= 9; i++) {

		if (x >= 160) {
			x = 0;
			y += POKE_ICON_SIZE;
		}

		DrawPokeIcon(i, x, y);

		x += POKE_ICON_SIZE + 17;
	}

	x = 0;
	y = 32;

	for (int i = 1; i <= 9; i++) {

		if (x >= 160) {
			x = 0;
			y += POKE_ICON_SIZE;
		}

		DrawPokeName(i, x, y + POKE_ICON_SIZE-5);

		x += POKE_ICON_SIZE + 17;
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