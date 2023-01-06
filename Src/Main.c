#include <PalmOS.h>

#include "Palmkedex.h"
#include "pokeInfo.h"
#include "UiResourceIDs.h"
#include "myTrg.h"


static void PokemonListDraw(Int16 itemNum, RectangleType *bounds, Char **sharedVarsPtr)
{
	SharedVariables *sharedVars = (SharedVariables*)sharedVarsPtr;
	const char *pokeName;
	UInt16 pokeNum, t, i;
	FontID prevFont;
	char numStr[4];

	if (sharedVars->sizeAfterFiltering == pokeGetNumber())
		pokeNum = itemNum + 1;
	else
		pokeNum = sharedVars->filteredPkmnNumbers[itemNum];

	if (pokeNum == MAX_SEARCH_PKMN_NUM)
		pokeName = MAX_SEARCH_STR;
	else
		pokeName = pokeNameGet(pokeNum);

	//to string with a hash up front
	for (t = pokeNum, i = 0; i < 3; i++) {

		numStr[3 - i] = '0' + t % 10;
		t /= 10;
	}
	numStr[0]  = '#';

	prevFont = FntSetFont(boldFont);
	WinDrawChars(numStr, 4, bounds->topLeft.x, bounds->topLeft.y);
	FntSetFont(stdFont);
	WinDrawChars(pokeName, StrLen(pokeName), bounds->topLeft.x + sharedVars->listNumsWidth, bounds->topLeft.y);
	FntSetFont(prevFont);
}

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

static void FilterDataSet(void)
{
	const char *searchStr = FldGetTextPtr(GetObjectPtr(MainSearchField));
	SharedVariables *sharedVars;
	UInt16 i;

	FtrGet(appFileCreator, ftrShrdVarsNum, (UInt32*)&sharedVars);

	if (!searchStr || !searchStr[0]) {	//no search

		sharedVars->sizeAfterFiltering = pokeGetNumber();
	}
	else {								//we have a search

		UInt16 potentialPokeID;
		char firstLetter;

		//find the first letter of the search, uppercase it, verify it IS a letter
		firstLetter = searchStr[0];
		if (firstLetter >= 'a' && firstLetter <= 'z')
			firstLetter += 'A' - 'a';

		if (firstLetter < 'A' || firstLetter > 'Z') {	//not a letter - no pokemon names match!

			sharedVars->sizeAfterFiltering = 0;
		}
		else {

			const UInt16 *potentialMatches = sharedVars->pokeIdsPerEachStartingLetter[firstLetter - 'A'];
			UInt16 L = StrLen(searchStr), matchCount = 0;

			//check each
			for (i = 0; (potentialPokeID = potentialMatches[i]) != 0; i++) {

				if (myCaselessStringNcmp(pokeNameGet(potentialPokeID), searchStr, L)) {

					sharedVars->filteredPkmnNumbers[matchCount] = potentialPokeID;
					matchCount++;

					if (matchCount == MAX_SEARCH_RESULT_LEN)
					{
						sharedVars->filteredPkmnNumbers[MAX_SEARCH_RESULT_LEN - 1] = MAX_SEARCH_PKMN_NUM;
						break;
					}
				}
			}
			sharedVars->sizeAfterFiltering = matchCount;
		}
	}
}

void OpenAboutDialog()
{
	FormType * frmP;

	/* Clear the menu status from the display */
	MenuEraseStatus(0);

	/* Display the About Box. */
	frmP = FrmInitForm (AboutForm);
	if (isHanderaHiRes())
			VgaFormModify(frmP, vgaFormModify160To240);
	FrmDoDialog (frmP);
	FrmDeleteForm (frmP);
}

static void UpdateList(void)
{
	SharedVariables *sharedVars;
	ListType *list;

	FilterDataSet();
	FtrGet(appFileCreator, ftrShrdVarsNum, (UInt32*)&sharedVars);

	list = GetObjectPtr(MainSearchList);
	// Set custom list drawing callback function.
	LstSetDrawFunction(list, PokemonListDraw);
	// Set list item number. pass "shared variables" as text - it can be quickly retrieved the the draw code (faster than FtrGet)
	LstSetListChoices(list, (char**)sharedVars, GetCurrentListSize());
	LstSetSelection(list, -1);
	LstDrawList(list);
}

Int16 GetCurrentListSize()
{
	UInt32 pstSharedInt;
	SharedVariables *sharedVars;
	Err err = errNone;

	err = FtrGet(appFileCreator, ftrShrdVarsNum, &pstSharedInt);
	ErrFatalDisplayIf (err != errNone, "Failed to load shared variables");
	sharedVars = (SharedVariables *)pstSharedInt;

	return sharedVars->sizeAfterFiltering;
}

static Boolean IsSelectionValid(UInt16 selection)
{
	return selection != MAX_SEARCH_PKMN_NUM;
}

static void calcPokemonNumberWidth(void)
{
	SharedVariables *sharedVars;
	UInt8 i, maxWidth = 0;
	FontID oldFont;
	char ch[4];

	FtrGet(appFileCreator, ftrShrdVarsNum, (UInt32*)&sharedVars);

	//calculate list font width (palmos has no kerning)
	oldFont = FntSetFont(boldFont);
	ch[3] = '#';
	for (i = 0; i < 10; i++) {
		UInt8 nowWidth;

		ch[0] = ch[1] = ch[2] = '0' + i;
		nowWidth = FntCharsWidth(ch, 4);

		if (nowWidth > maxWidth)
			maxWidth = nowWidth;
	}
	sharedVars->listNumsWidth = maxWidth + 2;
	FntSetFont(oldFont);
}

void OpenMainPkmnForm(Int16 selection)
{
	SharedVariables *sharedVars;
	UInt16 selectedPkmn;

	FtrGet(appFileCreator, ftrShrdVarsNum, (UInt32*)&sharedVars);
	selectedPkmn = GetPkmnId(selection);

	if (IsSelectionValid((UInt16) selectedPkmn))
	{
		sharedVars->selectedPkmnId = selectedPkmn;
		FrmGotoForm(PkmnMainForm);
	} else {
		FrmAlert (InvalidPokemonAlert);
		UpdateList();
	}
}

UInt16 GetPkmnId(Int16 selection)
{
	SharedVariables *sharedVars;

	FtrGet(appFileCreator, ftrShrdVarsNum, (UInt32*)&sharedVars);
	if (sharedVars->sizeAfterFiltering == pokeGetNumber())
	{
		return selection + 1;
	} else {
		return sharedVars->filteredPkmnNumbers[selection];
	}
}

/*
 * FUNCTION: MainFormDoCommand
 *
 * DESCRIPTION: This routine performs the menu command specified.
 *
 * PARAMETERS:
 *
 * command
 *     menu item id
 */

static Boolean MainFormDoCommand(UInt16 command)
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
	}

	return handled;
}

static Boolean resizeMainForm(FormPtr fp)
{
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
			case MainSearchField:
				rect.extent.x += newW - oldW;
				break;

			case MainSearchList:
				rect.extent.x += newW - oldW;
				rect.extent.y += newH - oldH;
				break;

			default:
				continue;
		}

		FrmSetObjectBounds(fp, idx, &rect);
	}

	return true;
}

/*
 * FUNCTION: MainFormHandleEvent
 *
 * DESCRIPTION:
 *
 * This routine is the event handler for the "MainForm" of this
 * application.
 *
 * PARAMETERS:
 *
 * eventP
 *     a pointer to an EventType structure
 *
 * RETURNED:
 *     true if the event was handled and should not be passed to
 *     FrmHandleEvent
 */

Boolean MainFormHandleEvent(EventType * eventP)
{
	FormPtr fp = FrmGetActiveForm();
	UInt32 pinsVersion;

	switch (eventP->eType)
	{
		case menuEvent:
			return MainFormDoCommand(eventP->data.menu.itemID);

		case ctlSelectEvent:
			return MainFormDoCommand(eventP->data.menu.itemID);

		case frmOpenEvent:
			if (errNone == FtrGet(pinCreator, pinFtrAPIVersion, &pinsVersion) && pinsVersion) {
				FrmSetDIAPolicyAttr(fp, frmDIAPolicyCustom);
				WinSetConstraintsSize(FrmGetWindowHandle(fp), 160, 240, 640, 160, 240, 640);
				PINSetInputTriggerState(pinInputTriggerEnabled);
			}
			if (isHanderaHiRes())
				VgaFormModify(fp, vgaFormModify160To240);
			resizeMainForm(fp);
			calcPokemonNumberWidth();
			FrmDrawForm(fp);
			UpdateList();
			return true;

        case lstSelectEvent:
			OpenMainPkmnForm(eventP->data.lstSelect.selection);
			break;

		case keyDownEvent:
			//the key will change the field, but it has not yet done so
			//the way it works is that the field will be told to handle
			//the event if it is in focus, and it'l self update. It is
			//a pain to try to wait for that, so we give the Field code
			//the event now, and then update ourselves. It is important
			//to mark the event as handled, to avoid the field getting
			//it again.

			if (FrmGetFocus(fp) == FrmGetObjectIndex(fp, MainSearchField)) {

				FldHandleEvent(GetObjectPtr(MainSearchField), eventP);
				UpdateList();
				return true;
			}
			break;

		case winEnterEvent:
			if (isHanderaHiRes())
				break;
			//fallthrough except for handera
			//fallthrough

		case displayExtentChangedEvent:
		case winDisplayChangedEvent:
		case frmUpdateEvent:
			if (resizeMainForm(fp)) {
				WinEraseWindow();
				FrmDrawForm(fp);
			}
			return true;

		default:
			break;
	}

	return false;
}

