#include <PalmOS.h>

#include "Palmkedex.h"
#include "Rsc/Palmkedex_Rsc.h"

void LoadPkmnStats()
{
	UInt32 pstSharedInt;
	SharedVariables *sharedVars;
	Err err = errNone;

	err = FtrGet(appFileCreator, ftrShrdVarsNum, &pstSharedInt);
	ErrFatalDisplayIf (err != errNone, "Failed to load feature memory");
	sharedVars = (SharedVariables *)pstSharedInt;

	SetFormTitle(sharedVars);

	MemHandle hndl = DmGet1Resource(0x70494e46, sharedVars->selectedPkmnId);
	UInt8* pkmnBytes = MemHandleLock(hndl);
	FormType *frm = FrmGetActiveForm();

	SetLabelInfo(PkmnMainHPValueLabel, pkmnBytes[0], frm);
	SetLabelInfo(PkmnMainAtkValueLabel, pkmnBytes[1], frm);
	SetLabelInfo(PkmnMainDefValueLabel, pkmnBytes[2], frm);
	SetLabelInfo(PkmnMainSPAtkValueLabel, pkmnBytes[3], frm);
	SetLabelInfo(PkmnMainSPDefValueLabel, pkmnBytes[4], frm);
	SetLabelInfo(PkmnMainSpeedValueLabel, pkmnBytes[5], frm);

	MemHandleUnlock(hndl);
}

void SetLabelInfo(UInt16 labelId, UInt8 stat, FormType *frm)
{
	Char *str;

	str = (Char *)MemPtrNew(sizeof(Char[4]));
	if ((UInt32)str == 0)
		return;
	MemSet(str, sizeof(Char[4]), 0);

	StrIToA(str, stat);
	FrmCopyLabel(frm, labelId, str);

	MemPtrFree(str);
}

void SetFormTitle(SharedVariables *sharedVars)
{
	UInt32 pstSpeciesInt;
	Species *species;
	Err err = errNone;

	err = FtrGet(appFileCreator, ftrPkmnNamesNum, &pstSpeciesInt);
	ErrFatalDisplayIf (err != errNone, "Failed to load feature memory");
	species = (Species*)pstSpeciesInt;

	Char *numStr;

	if ((UInt32)sharedVars->pkmnFormTitle != 0)
	{
		MemPtrFree(sharedVars->pkmnFormTitle);
	}

	sharedVars->pkmnFormTitle = (Char *)MemPtrNew(sizeof(Char[18]));
	if ((UInt32)sharedVars->pkmnFormTitle == 0)
		return;
	MemSet(sharedVars->pkmnFormTitle, sizeof(Char[18]), 0);

	 numStr = (Char *)MemPtrNew(sizeof(Char[5]));
	 if ((UInt32)numStr == 0)
	 	return;
	 MemSet(numStr, sizeof(Char[5]), 0);

	StrCopy(sharedVars->pkmnFormTitle, species->nameList[sharedVars->selectedPkmnId-1].name);
	StrCat(sharedVars->pkmnFormTitle, " #");
	StrIToA(numStr, sharedVars->selectedPkmnId);
	StrCat(sharedVars->pkmnFormTitle, numStr);
	
	FrmSetTitle(FrmGetActiveForm(), sharedVars->pkmnFormTitle);

	MemPtrFree(numStr);
}

/*
 * FUNCTION: PkmnMainFormDoCommand
 *
 * DESCRIPTION: This routine performs the menu command specified.
 *
 * PARAMETERS:
 *
 * command
 *     menu item id
 */

static Boolean PkmnMainFormDoCommand(UInt16 command)
{
	Boolean handled = false;

	switch (command)
	{
		case PkmnMainBackButton:
		{
			FrmGotoForm(MainForm);
			handled = true;
			break;
		}

		default:
			break;
	}

	return handled;
}

/*
 * FUNCTION: PkmnMainFormHandleEvent
 *
 * DESCRIPTION:
 *
 * This routine is the event handler for the "PkmnMainForm" of this 
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

Boolean PkmnMainFormHandleEvent(EventType * eventP)
{
	Boolean handled = false;
	FormType * frmP;

	switch (eventP->eType) 
	{	
		case ctlSelectEvent:
			return PkmnMainFormDoCommand(eventP->data.menu.itemID);

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			LoadPkmnStats();
			FrmDrawForm(frmP);
			handled = true;
			break;
		default:
			break;
	}
    
	return handled;
}
