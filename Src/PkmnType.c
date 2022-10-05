#include <PalmOS.h>

#include "Palmkedex.h"
#include "Rsc/Palmkedex_Rsc.h"

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

static Boolean PkmnTypeFormDoCommand(UInt16 command)
{
	Boolean handled = false;

	switch (command)
	{
		// case PkmnMainBackButton:
		// {
		// 	FrmGotoForm(MainForm);
		// 	handled = true;
		// 	break;
		// }

		default:
			break;
	}

	return handled;
}

Boolean PkmnTypeFormHandleEvent(EventType * eventP)
{
	Boolean handled = false;
	FormType * frmP;

	switch (eventP->eType) 
	{	
		case ctlSelectEvent:
			return PkmnTypeFormDoCommand(eventP->data.menu.itemID);

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm(frmP);
			handled = true;
			break;

        case popSelectEvent:
			if (eventP->data.popSelect.selection == 0)
			{
				FrmGotoForm(PkmnMainForm);
			}
			break;

		default:
			break;
	}
    
	return handled;
}
