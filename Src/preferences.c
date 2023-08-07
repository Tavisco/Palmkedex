#include <PalmOS.h>
#include "Palmkedex.h"
#include "UiResourceIDs.h"

static Boolean PrefsFormDoCommand(UInt16 command)
{
	Boolean handled = false;

	switch (command)
	{
	case PrefsFormOKButton:
	{
		FrmReturnToForm(0);
		handled = true;
		break;
	}


	default:
		break;
	}

	return handled;
}

Boolean PrefsFormHandleEvent(EventType *eventP)
{
	Boolean handled = false;
	FormPtr fp = FrmGetActiveForm();

	switch (eventP->eType)
	{
	case frmOpenEvent:
		FrmDrawForm(fp);
		handled = true;
		break;

	case ctlSelectEvent:
		return PrefsFormDoCommand(eventP->data.ctlSelect.controlID);

	default:
		break;
	}

	return handled;
}
