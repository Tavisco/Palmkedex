#include <PalmOS.h>

#include "Palmkedex.h"
#include "Rsc/Palmkedex_Rsc.h"

static void PokemonListDraw(Int16 itemNum, RectangleType *bounds, Char **unused)
{
	UInt32 pstSpeciesInt, pstSharedInt;
	Species *species;
	SharedVariables *sharedVars;
	Err err = errNone;

	err = FtrGet(appFileCreator, ftrPkmnNamesNum, &pstSpeciesInt);
	ErrFatalDisplayIf (err != errNone, "Failed to load feature memory");
	species = (Species*)pstSpeciesInt;

	err = FtrGet(appFileCreator, ftrShrdVarsNum, &pstSharedInt);
	ErrFatalDisplayIf (err != errNone, "Failed to load feature memory");
	sharedVars = (SharedVariables *)pstSharedInt;

	if (sharedVars->sizeAfterFiltering == PKMN_QUANTITY) {
		WinDrawChars(species->nameList[itemNum].name, 11, bounds->topLeft.x, bounds->topLeft.y);
	} else {
		WinDrawChars(sharedVars->filteredList[itemNum].name, 11, bounds->topLeft.x, bounds->topLeft.y);
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

		case MainSearchButton:
		{
			UpdateList();
			handled = true;
			break;
		}
		
	}

	return handled;
}

void FilterDataSet()
{
	Char *searchStr;
	FieldType *fldSearch = GetObjectPtr(MainSearchField);
	UInt16 searchLen, matchCount, secondMatchCount;
	UInt32 pstSpeciesInt, pstSharedInt;
	Species *species;
	SharedVariables *sharedVars;
	Err err = errNone;

	err = FtrGet(appFileCreator, ftrPkmnNamesNum, &pstSpeciesInt);
	ErrFatalDisplayIf (err != errNone, "Failed to load feature memory");
	species = (Species*)pstSpeciesInt;

	err = FtrGet(appFileCreator, ftrShrdVarsNum, &pstSharedInt);
	ErrFatalDisplayIf (err != errNone, "Failed to load feature memory");
	sharedVars = (SharedVariables *)pstSharedInt;

	searchStr = FldGetTextPtr(fldSearch);
	if (searchStr == NULL)
	{
		sharedVars->sizeAfterFiltering = PKMN_QUANTITY;
		return;
	}

	searchLen = StrLen(searchStr)+1;
	matchCount = 0;
	secondMatchCount = 0;

	Char *str;

	str = (Char *)MemPtrNew(sizeof(Char[searchLen]));
	ErrFatalDisplayIf (((UInt32)str == 0), "Out of memory");

	// First, we determine the quantity of pokemons that
	// matches the filter
	for (Int16 i = 0; i < PKMN_QUANTITY; i++)
	{
		MemSet(str, sizeof(Char[searchLen]), 0);
		subString(species->nameList[i].name, 0, searchLen-1, str);
		if (StrCaselessCompare(str, searchStr) == 0)
		{
			matchCount++;
		}
	}

	sharedVars->sizeAfterFiltering = matchCount;

	// We create an array of the size we found
	sharedVars->filteredList = (SpeciesNames *)MemPtrNew(sizeof(SpeciesNames[matchCount]));
	ErrFatalDisplayIf (((UInt32)str == 0), "Out of memory");

	// Then iterate again copying the pokemons to that new array
	// This is so stupid lol there must be a way to not iterate again
	for (Int16 i = 0; i < PKMN_QUANTITY; i++)
	{
		MemSet(str, sizeof(Char[searchLen]), 0);
		subString(species->nameList[i].name, 0, searchLen-1, str);
		if (StrCaselessCompare(str, searchStr) == 0)
		{
			StrCopy(sharedVars->filteredList[secondMatchCount].name, species->nameList[i].name);
			secondMatchCount++;
		}

		if (matchCount == secondMatchCount){
			break;
		}
	}

	MemPtrFree(str);
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

void UpdateList()
{
	FilterDataSet();

	ListType *list = GetObjectPtr(MainSearchList);
	// Set custom list drawing callback function.
	LstSetDrawFunction(list, PokemonListDraw);
	// Set list item number
	LstSetListChoices(list, NULL, GetCurrentListSize());
	LstSetSelection(list, -1);
	LstDrawList(list);
}

Int16 GetCurrentListSize()
{
	UInt32 pstSharedInt;
	SharedVariables *sharedVars;
	Err err = errNone;

	err = FtrGet(appFileCreator, ftrShrdVarsNum, &pstSharedInt);
	ErrFatalDisplayIf (err != errNone, "Failed to load feature memory");
	sharedVars = (SharedVariables *)pstSharedInt;

	return sharedVars->sizeAfterFiltering;
}

void OpenMainPkmnForm(Int16 selection)
{
	UInt32 pstSharedInt;
	SharedVariables *sharedVars;
	Err err = errNone;

	err = FtrGet(appFileCreator, ftrShrdVarsNum, &pstSharedInt);
	ErrFatalDisplayIf (err != errNone, "Failed to load feature memory");
	sharedVars = (SharedVariables *)pstSharedInt;

	sharedVars->selectedPkmnId = GetPkmnId(selection);

	FrmGotoForm(PkmnMainForm);
}

Int16 GetPkmnId(Int16 selection)
{
	return selection + 1;
	// ListType *list = GetObjectPtr(MainSearchList);

	// Char *str;

	// str = (Char *)MemPtrNew(sizeof(Char[5]));
	// ErrFatalDisplayIf (((UInt32)str == 0), "Out of memory");

	// MemSet(str, sizeof(Char[5]), 0);

	// subString(LstGetSelectionText(list, 0), 13, 4, str);

	// ErrDisplay(str);

	// MemPtrUnlock(str);

	// return StrAToI(str);
}

void subString (const Char* input, int offset, int len, Char* dest)
{
	int input_len = StrLen(input);

	if (offset + len > input_len)
	{
		return;
	}

	StrNCopy(dest, input + offset, len);
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

	switch (eventP->eType) 
	{
		case menuEvent:
			return MainFormDoCommand(eventP->data.menu.itemID);
		
		case ctlSelectEvent:
			return MainFormDoCommand(eventP->data.menu.itemID);

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm(frmP);
			UpdateList();
			handled = true;
			break;
            
        case lstSelectEvent:
			OpenMainPkmnForm(eventP->data.lstSelect.selection);
			break;

		// case keyDownEvent:
		// {
		// 	FilterList();
		// 	break;
		// }

		default:
			break;
	}
    
	return handled;
}

