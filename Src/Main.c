#include <PalmOS.h>

#include "Palmkedex.h"
#include "Rsc/Palmkedex_Rsc.h"

static void PokemonListDraw(Int16 itemNum, RectangleType *bounds, Char **unused)
{
	UInt32 pstSpeciesInt;
	Species *species;
	Err err = errNone;

	err = FtrGet(appFileCreator, ftrPkmnNamesNum, &pstSpeciesInt);
	ErrFatalDisplayIf (err != errNone, "Failed to load feature memory");
	species = (Species*)pstSpeciesInt;
	WinDrawChars(species->nameList[itemNum].name, 11, bounds->topLeft.x, bounds->topLeft.y);
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

void PopulateList()
{
	ListType *list = GetObjectPtr(MainSearchList);
	// Set custom list drawing callback function.
	LstSetDrawFunction(list, PokemonListDraw);
	// Set list item number
	LstSetListChoices(list, NULL, PKMN_QUANTITY);
	LstSetSelection(list, -1);
	LstDrawList(list);
}

void OpenMainPkmnForm(Int16 selection)
{
	SharedVariables *sharedVars;
	Err err = errNone;

	sharedVars = (SharedVariables *)MemPtrNew(sizeof(SharedVariables));
	ErrFatalDisplayIf ((!sharedVars), "Out of memory");
	MemSet(sharedVars, sizeof(SharedVariables), 0);

	sharedVars->selectedPkmnId = selection+1;

	err = FtrSet(appFileCreator, ftrShrdVarsNum, (UInt32)sharedVars);
	ErrFatalDisplayIf (err != errNone, "Failed to set feature memory");

	FrmGotoForm(PkmnMainForm);
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
			PopulateList();
			handled = true;
			break;
            
        case lstSelectEvent:
			OpenMainPkmnForm(eventP->data.lstSelect.selection);
			break;
		
		default:
			break;
	}
    
	return handled;
}

