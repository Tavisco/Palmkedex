#include <PalmOS.h>
#include "Palmkedex.h"
#include "UiResourceIDs.h"


static void LoadPrefs(void)
{
	Boolean foundPrefs;
	struct PalmkedexPrefs *prefs;
	UInt16 latestPrefSize, underGraffitiPushOption;

	latestPrefSize = sizeof(struct PalmkedexPrefs);

	prefs = MemPtrNew(latestPrefSize);
	if (!prefs)
	{
		SysFatalAlert("Failed to allocate memory to store preferences!");
	}
	MemSet(prefs, latestPrefSize, 0);

	foundPrefs = PrefGetAppPreferencesV10(appFileCreator, appPrefVersionNum, prefs, latestPrefSize);
	if (!foundPrefs)
	{
		ErrAlertCustom(0, "Failed to load preferences!", NULL, NULL);
	}

	// Update the checkbox with the current setting
	CtlSetValue(FrmGetObjectPtr(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), PrefsFormGridCheckBox)), prefs->mainFormFormat == 1);

	underGraffitiPushOption = prefs->mainUnderGraffitiType? PrefsPushTypeMatchup : PrefsPushDexEntry;
	CtlSetValue(FrmGetObjectPtr(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), underGraffitiPushOption)), true);

	MemPtrFree(prefs);
}

static void SavePrefs(void)
{
	Boolean foundPrefs;
	struct PalmkedexPrefs *prefs;
	UInt16 latestPrefSize;

	latestPrefSize = sizeof(struct PalmkedexPrefs);

	prefs = MemPtrNew(latestPrefSize);
	if (!prefs)
	{
		SysFatalAlert("Failed to allocate memory to store preferences!");
	}
	MemSet(prefs, latestPrefSize, 0);

	foundPrefs = PrefGetAppPreferencesV10(appFileCreator, appPrefVersionNum, prefs, latestPrefSize);
	if (!foundPrefs)
	{
		ErrAlertCustom(0, "Failed to load preferences!", NULL, NULL);
	}
	// Update the prefs with the current setting
	prefs->mainFormFormat = CtlGetValue(FrmGetObjectPtr(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), PrefsFormGridCheckBox)));
	prefs->mainUnderGraffitiType = CtlGetValue(FrmGetObjectPtr(FrmGetActiveForm(), FrmGetObjectIndex(FrmGetActiveForm(), PrefsPushDexEntry)))? 0 : 1;

	PrefSetAppPreferencesV10(appFileCreator, appPrefVersionNum, prefs, latestPrefSize);
	MemPtrFree(prefs);
}


static Boolean PrefsFormDoCommand(UInt16 command)
{
	Boolean handled = false;

	switch (command)
	{
	case PrefsFormOKButton:
	{
		SavePrefs();
		FrmCloseAllForms();
		GoToPreferredMainForm();
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
