#include <PalmOS.h>

#include "Palmkedex.h"
#include "Src/pokeInfo.h"
#include "Rsc/Palmkedex_Rsc.h"


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
	WinDrawChars(pokeName, StrLen(pokeName), bounds->topLeft.x + 32, bounds->topLeft.y);
	FntSetFont(prevFont);
}

static void ParseSearchString(Char *searchStr, Char charInserted)
{
	Char *fieldStr;
	FieldType *fldSearch = GetObjectPtr(MainSearchField);

	fieldStr = FldGetTextPtr(fldSearch);

	if (fieldStr != NULL)
	{
		if (charInserted == BACKSPACE_CHAR)
		{
			StrNCat(searchStr, fieldStr, StrLen(fieldStr)); // Copy N-1 char if backspace.
		} else {
			StrCat(searchStr, fieldStr);
		}
	}

	// And, the inputed char, if it's not a backspace
	if (charInserted != BACKSPACE_CHAR)
	{
		searchStr[StrLen(searchStr)] = charInserted;
	}
}

static void PrepareMemoryForSearch(SharedVariables *sharedVars)
{
	if ((UInt32)sharedVars->filteredPkmnNumbers != 0)
	{
		MemPtrFree(sharedVars->filteredPkmnNumbers);
	}

	sharedVars->filteredPkmnNumbers = (UInt16 *)MemPtrNew(sizeof(UInt16[MAX_SEARCH_RESULT_LEN]));
	ErrFatalDisplayIf (((UInt32)sharedVars->filteredPkmnNumbers == 0), "Out of memory");
	MemSet(sharedVars->filteredPkmnNumbers, sizeof(UInt16[MAX_SEARCH_RESULT_LEN]), 0);
}

static Boolean IsNameShorterThanQuery(const Char *pkmnName, UInt16 searchLen)
{
	return StrLen(pkmnName) < searchLen-1;
}

static Boolean NameMatchesQuery(const Char *pkmnName, const Char *searchStr, UInt16 searchLen)
{
	UInt16 i;

	for (i = 0; i < searchLen; i++)
	{
		if (searchStr[i] != pkmnName[i])
		{
			break;
		}
	}

	// If the iterator have the same lenght
	// as the search string, it means that all
	// the chars are equal, and thus, it's a match
	return i == searchLen-1;
}

static void FilterDataSet(Char charInserted)
{
	UInt16 searchLen, matchCount, i;
	SharedVariables *sharedVars;
	Char searchStr[MAX_PKMN_NAME_LEN+1] = "";
	Char substringPkmnName[MAX_PKMN_NAME_LEN+1] = "";
	Err err = errNone;

	err = FtrGet(appFileCreator, ftrShrdVarsNum, (UInt32*)&sharedVars);
	ErrFatalDisplayIf (err != errNone, "Failed to load shared variables");

	ParseSearchString(searchStr, charInserted);

	if (StrLen(searchStr) == 0)
	{
		// If nothing is being searched, no need to filter :)
		sharedVars->sizeAfterFiltering = pokeGetNumber();
		return;
	}

	PrepareMemoryForSearch(sharedVars);

	searchLen = StrLen(searchStr)+1;
	matchCount = 0;

	for (i = 0; i < pokeGetNumber(); i++)
	{
		if (IsNameShorterThanQuery(pokeNameGet(i + 1), searchLen))
		{
			continue;
		}

		if (NameMatchesQuery(pokeNameGet(i + 1), searchStr, searchLen))
		{
			sharedVars->filteredPkmnNumbers[matchCount] = i + 1;
			matchCount++;
		}

		if (matchCount == MAX_SEARCH_RESULT_LEN)
		{
			sharedVars->filteredPkmnNumbers[MAX_SEARCH_RESULT_LEN-1] = MAX_SEARCH_PKMN_NUM;
			break;
		}
	}

	sharedVars->sizeAfterFiltering = matchCount;
}

void OpenAboutDialog()
{
	FormType * frmP;

	/* Clear the menu status from the display */
	MenuEraseStatus(0);

	/* Display the About Box. */
	frmP = FrmInitForm (AboutForm);
	FrmDoDialog (frmP);
	FrmDeleteForm (frmP);
}

static void UpdateList(Char charInserted)
{
	SharedVariables *sharedVars;
	ListType *list;

	FilterDataSet(charInserted);
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

void OpenMainPkmnForm(Int16 selection)
{
	UInt32 pstSharedInt;
	UInt16 selectedPkmn;
	SharedVariables *sharedVars;
	Err err = errNone;

	selectedPkmn = GetPkmnId(selection);

	if (IsSelectionValid((UInt16) selectedPkmn))
	{
		err = FtrGet(appFileCreator, ftrShrdVarsNum, &pstSharedInt);
		ErrFatalDisplayIf (err != errNone, "Failed to load shared variables");
		sharedVars = (SharedVariables *)pstSharedInt;

		sharedVars->selectedPkmnId = selectedPkmn;

		FrmGotoForm(PkmnMainForm);
	} else {
		FrmAlert (InvalidPokemonAlert);
		UpdateList(NULL);
	}
}

UInt16 GetPkmnId(Int16 selection)
{
	UInt32 pstSharedInt;
	SharedVariables *sharedVars;
	Err err = errNone;

	err = FtrGet(appFileCreator, ftrShrdVarsNum, &pstSharedInt);
	ErrFatalDisplayIf (err != errNone, "Failed to load shared variables");
	sharedVars = (SharedVariables *)pstSharedInt;

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
	Boolean handled = false;
	FormType * frmP;
	UInt16 focus;

	switch (eventP->eType)
	{
		case menuEvent:
			return MainFormDoCommand(eventP->data.menu.itemID);

		case ctlSelectEvent:
			return MainFormDoCommand(eventP->data.menu.itemID);

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm(frmP);
			UpdateList(NULL);
			handled = true;
			break;

        case lstSelectEvent:
			OpenMainPkmnForm(eventP->data.lstSelect.selection);
			break;

		case keyDownEvent:
		{
			focus = FrmGetFocus(FrmGetActiveForm());
			if (focus != noFocus)
			{
				UpdateList(eventP->data.keyDown.chr);
			}
			break;
		}

		default:
			break;
	}

	return handled;
}
