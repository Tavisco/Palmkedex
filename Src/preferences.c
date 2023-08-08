#include <PalmOS.h>
#include "Palmkedex.h"
#include "UiResourceIDs.h"


static void LoadPrefs(void)
{
	UInt16 latestPrefSize;
	struct PalmkedexPrefs *prefs;

	latestPrefSize = sizeof(struct PalmkedexPrefs);
	prefs = MemPtrNew(latestPrefSize);
	MemSet(prefs, latestPrefSize, 0);

	PrefGetAppPreferences(appFileCreator, appPrefID, prefs, &latestPrefSize, true);

	// Update the checkbox with the current setting
	CtlSetValue(FrmGetObjectPtr(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), PrefsFormGridCheckBox)), prefs->mainFormFormat == 1);
}

static void SavePrefs(void)
{
	UInt16 latestPrefSize;
	struct PalmkedexPrefs *prefs;

	latestPrefSize = sizeof(struct PalmkedexPrefs);
	prefs = MemPtrNew(latestPrefSize);
	MemSet(prefs, latestPrefSize, 0);

	PrefGetAppPreferences(appFileCreator, appPrefID, prefs, &latestPrefSize, true);

	// Update the prefs with the current setting
	prefs->mainFormFormat = CtlGetValue(FrmGetObjectPtr(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), PrefsFormGridCheckBox)));

	PrefSetAppPreferences(appFileCreator, appPrefID, appPrefVersionNum, prefs, latestPrefSize, true);
}


static Boolean PrefsFormDoCommand(UInt16 command)
{
	Boolean handled = false;

	switch (command)
	{
	case PrefsFormOKButton:
	{
		SavePrefs();
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
		LoadPrefs();
		handled = true;
		break;

	case ctlSelectEvent:
		return PrefsFormDoCommand(eventP->data.ctlSelect.controlID);

	default:
		break;
	}

	return handled;
}
