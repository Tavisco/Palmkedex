#include <PalmOS.h>

#include "BUILD_TYPE.h"
#include "Palmkedex.h"
#include "pokeInfo.h"
#include "UiResourceIDs.h"
#ifdef HANDERA_SUPPORT
#include "myTrg.h"
#endif


static void PokemonItemsDraw(Int16 itemNum, RectangleType *bounds, Char **sharedVarsPtr)
{
    SharedVariables *sharedVars = (SharedVariables*)sharedVarsPtr;
    char pokeName[33];
    UInt16 pokeNum, t, i;
    FontID prevFont;
    char numStr[5];

    if (sharedVars->sizeAfterFiltering == TOTAL_POKE_COUNT_ZERO_BASED)
        pokeNum = itemNum + 1;
    else
        pokeNum = sharedVars->filteredItemNumbers[itemNum];

    if (pokeNum == MAX_SEARCH_PKMN_NUM)
        StrCopy(pokeName, MAX_SEARCH_STR);
    else
        itemNameGet(pokeName, pokeNum);

    //to string with a hash up front
    for (t = pokeNum, i = 0; i < 4; i++) {

        numStr[4 - i] = '0' + t % 10;
        t /= 10;
    }
    numStr[0]  = '#';

    prevFont = FntSetFont(boldFont);
    WinDrawChars(numStr, 5, bounds->topLeft.x, bounds->topLeft.y);
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

void FilterItemDataSet(const char *searchStr)
{
    SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
    UInt16 i;


    if (!searchStr || !searchStr[0]) {	//no search

        sharedVars->sizeAfterFiltering = TOTAL_POKE_COUNT_ZERO_BASED;
    }
    else {								//we have a search

        UInt16 potentialItemID;
        char firstLetter;

        //find the first letter of the search, uppercase it, verify it IS a letter
        firstLetter = searchStr[0];
        if (firstLetter >= 'a' && firstLetter <= 'z')
            firstLetter += 'A' - 'a';

        if (firstLetter < 'A' || firstLetter > 'Z') {	//not a letter - no pokemon names match!
            sharedVars->sizeAfterFiltering = 0;
        } else {
            const UInt16 *potentialMatches = sharedVars->ItemIdsPerEachStartingLetter[firstLetter - 'A'];
            UInt16 L = StrLen(searchStr), matchCount = 0;

            //check each
            for (i = 0; (potentialItemID = potentialMatches[i]) != 0; i++) {

                char potentialItemName[POKEMON_NAME_LEN + 1];

                itemNameGet(potentialItemName, potentialItemID);

                if (myCaselessStringNcmp(potentialItemName, searchStr, L)) {

                    sharedVars->filteredItemNumbers[matchCount] = potentialItemID;
                    matchCount++;

                    if (matchCount == MAX_SEARCH_RESULT_LEN)
                    {
                        sharedVars->filteredItemNumbers[MAX_SEARCH_RESULT_LEN - 1] = MAX_SEARCH_PKMN_NUM;
                        break;
                    }
                }
            }
            sharedVars->sizeAfterFiltering = matchCount;
        }
    }
}

// void OpenAboutDialog(void)
// {
//     FormType * frmP;

//     /* Clear the menu status from the display */
//     MenuEraseStatus(0);

//     /* Display the About Box. */
//     frmP = FrmInitForm (AboutForm);
//     #ifdef HANDERA_SUPPORT
//     if (isHanderaHiRes())
//         VgaFormModify(frmP, vgaFormModify160To240);
//     #endif
//     FrmDoDialog (frmP);
//     FrmDeleteForm (frmP);
// }

static void UpdateList(void)
{
    SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
    FormPtr fp = FrmGetActiveForm();
    ListType *list;

    FilterItemDataSet(FldGetTextPtr(GetObjectPtr(ItemsSearchField)));
    list = GetObjectPtr(ItemsSearchList);
    // Set custom list drawing callback function.
    LstSetDrawFunction(list, PokemonItemsDraw);
    // Set list item number. pass "shared variables" as text - it can be quickly retrieved the the draw code (faster than FtrGet)
    LstSetListChoices(list, (char**)sharedVars, sharedVars->sizeAfterFiltering);
    if (sharedVars->sizeAfterFiltering > 0)
        LstSetTopItem(list, 0);
    LstSetSelection(list, -1);
    LstDrawList(list);
    FrmSetFocus(fp, FrmGetObjectIndex(fp, ItemsSearchField));
}

static Boolean IsSelectionValid(UInt16 selection)
{
    return selection != MAX_SEARCH_PKMN_NUM;
}

static void calcPokemonNumberWidth(void)
{
    SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
    UInt8 i, maxWidth = 0;
    FontID oldFont;
    char ch[5];

    //calculate list font width (palmos has no kerning)
    oldFont = FntSetFont(boldFont);
    ch[4] = '#';
    for (i = 0; i < 10; i++) {
        UInt8 nowWidth;

        ch[0] = ch[1] = ch[2] = ch[3] = '0' + i;
        nowWidth = FntCharsWidth(ch, 5);

        if (nowWidth > maxWidth)
            maxWidth = nowWidth;
    }
    sharedVars->listNumsWidth = maxWidth + 2;
    FntSetFont(oldFont);
}

void ShowItemDetails(Int16 selection)
{
    SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
    const char *searchStr = FldGetTextPtr(GetObjectPtr(ItemsSearchField));
    UInt16 selectedPkmn;
    MemHandle hndl;
    char *dexEntry = NULL;

    selectedPkmn = GetItemId(selection);

    if (IsSelectionValid((UInt16) selectedPkmn))
    {
        DmOpenRef dbRef = DmOpenDatabaseByTypeCreator('ITEM', appFileCreator, dmModeReadOnly);
        if (!dbRef)
        {
            ErrFatalDisplay("Failed to find item database!");
            return;
        }

        hndl = DmGet1Resource('DESC', 0);
        dexEntry = pokeDescrGet(hndl, selectedPkmn);

        if (dexEntry == NULL)
        {
            FrmCustomAlert(DexEntryAlert, "oops", "", "");
            return;
        }

        FrmCustomAlert(DexEntryAlert, dexEntry, " ", "");
        MemPtrFree(dexEntry);
    } else {
        FrmAlert (InvalidPokemonAlert);
        UpdateList();
    }
}

UInt16 GetItemId(Int16 selection)
{
    SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);

    if (sharedVars->sizeAfterFiltering == TOTAL_POKE_COUNT_ZERO_BASED)
        return selection + 1;
    else
        return sharedVars->filteredItemNumbers[selection];
}

static Boolean ItemsFormDoCommand(UInt16 command)
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
        case OptionsPreferences:
        {
            FrmPopupForm(PrefsForm);
            handled = true;
            break;
        }
        case ItemsSearchClearButton:
        {
            SetFieldText(ItemsSearchField,"");
            UpdateList();
            handled = true;
            break;
        }
    }

    return handled;
}

static Boolean resizeItemsForm(FormPtr fp)
{
    #ifdef SCREEN_RESIZE_SUPPORT
    WinHandle wh = FrmGetWindowHandle(fp);
    Coord newW, newH, oldW, oldH;
    RectangleType rect;
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
        case ItemsSearchField:
            rect.extent.x += newW - oldW;
            break;

        case ItemsSearchClearButton:
            rect.topLeft.x += newW - oldW;
            break;

        case ItemsSearchList:
            rect.extent.x += newW - oldW;
            rect.extent.y += newH - oldH;
            break;

        default:
            continue;
        }

        FrmSetObjectBounds(fp, idx, &rect);
    }

    return true;
    #else
    return false;
    #endif
}

static void RecoverPreviousFilter(void)
{
    Boolean foundPrefs;
    struct PalmkedexPrefs *prefs;
    SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
    UInt16 latestPrefSize;

    latestPrefSize = sizeof(struct PalmkedexPrefs);

    prefs = MemPtrNew(latestPrefSize);
    if (!prefs)
    {
        SysFatalAlert("Failed to allocate memory to store preferences!");
        MemPtrFree(prefs);
        return;
    }
    MemSet(prefs, latestPrefSize, 0);

    foundPrefs = PrefGetAppPreferencesV10(appFileCreator, appPrefVersionNum, prefs, latestPrefSize);
    if (!foundPrefs)
    {
        ErrAlertCustom(0, "Failed to load preferences! Cannot recover search.", NULL, NULL);
        MemPtrFree(prefs);
        return;
    }

    if (prefs->shouldRememberSearch){
        SetFieldText(ItemsSearchField, sharedVars->nameFilter);
    } else {
        SetFieldText(ItemsSearchField, "");
    }

    MemPtrFree(prefs);
    return;
}

static void RecoverPokemonSelection(void)
{
    Boolean foundPrefs;
    struct PalmkedexPrefs *prefs;
    SharedVariables *sharedVars = (SharedVariables*)globalsSlotVal(GLOBALS_SLOT_SHARED_VARS);
    UInt16 latestPrefSize;

    latestPrefSize = sizeof(struct PalmkedexPrefs);

    prefs = MemPtrNew(latestPrefSize);
    if (!prefs)
    {
        SysFatalAlert("Failed to allocate memory to store preferences!");
        MemPtrFree(prefs);
        return;
    }
    MemSet(prefs, latestPrefSize, 0);

    foundPrefs = PrefGetAppPreferencesV10(appFileCreator, appPrefVersionNum, prefs, latestPrefSize);
    if (!foundPrefs)
    {
        ErrAlertCustom(0, "Failed to load preferences! Cannot recover search.", NULL, NULL);
        MemPtrFree(prefs);
        return;
    }

    if (sharedVars->selectedPkmnLstIndex == noListSelection || !prefs->shouldRememberSearch)
        MemPtrFree(prefs);
    return;

    LstSetSelection(GetObjectPtr(ItemsSearchList), sharedVars->selectedPkmnLstIndex);
}

static void ScrollSearchList(WChar c)
{
    if (isPalmOsAtLeast(sysMakeROMVersion(2,0,0,sysROMStageDevelopment,0)))
        return;

    WinDirectionType direction;
    if (c == vchrPageUp)
        direction = winUp;
    else if (c == vchrPageDown)
        direction = winDown;
    else
        return;

    LstScrollList(GetObjectPtr(ItemsSearchList), direction, 5);
}

Boolean ItemsFormHandleEvent(EventType * eventP)
{
    FormPtr fp = FrmGetActiveForm();
    UInt32 pinsVersion;

    switch (eventP->eType)
    {
        case menuEvent:
            return ItemsFormDoCommand(eventP->data.menu.itemID);

        case ctlSelectEvent:
            return ItemsFormDoCommand(eventP->data.ctlSelect.controlID);

        case frmOpenEvent:
            if (errNone == FtrGet(pinCreator, pinFtrAPIVersion, &pinsVersion) && pinsVersion) {
                FrmSetDIAPolicyAttr(fp, frmDIAPolicyCustom);
                WinSetConstraintsSize(FrmGetWindowHandle(fp), 160, 240, 640, 160, 240, 640);
                PINSetInputTriggerState(pinInputTriggerEnabled);
            }
            #ifdef HANDERA_SUPPORT
            if (isHanderaHiRes())
                VgaFormModify(fp, vgaFormModify160To240);
        #endif
        resizeItemsForm(fp);
        calcPokemonNumberWidth();
        FrmDrawForm(fp);
        // RecoverPreviousFilter();
        UpdateList();
        // RecoverPokemonSelection();
        return true;

        case lstSelectEvent:
            ShowItemDetails(eventP->data.lstSelect.selection);
            break;

        case keyDownEvent:
            if (eventP->data.keyDown.chr == vchrPageUp || eventP->data.keyDown.chr == vchrPageDown)
            {
                ScrollSearchList(eventP->data.keyDown.chr); // TODO: ADD HANDERA JOG SUPPORT AS WELL!
                return true;
            }

            //the key will change the field, but it has not yet done so
            //the way it works is that the field will be told to handle
            //the event if it is in focus, and it'l self update. It is
            //a pain to try to wait for that, so we give the Field code
            //the event now, and then update ourselves. It is important
            //to mark the event as handled, to avoid the field getting
            //it again.

            if (FrmGetFocus(fp) == FrmGetObjectIndex(fp, ItemsSearchField)) {

                FldHandleEvent(GetObjectPtr(ItemsSearchField), eventP);
                UpdateList();
                return true;
            }
            break;

        case winEnterEvent:
            if (isHanderaHiRes()) //fallthrough except for handera
                break;

        #ifdef HANDERA_SUPPORT
        case displayExtentChangedEvent:
            #endif
        case winDisplayChangedEvent:
        case frmUpdateEvent:
            if (resizeItemsForm(fp)) {
                WinEraseWindow();
                FrmDrawForm(fp);
            }
            return true;
        case popSelectEvent:
			if (eventP->data.popSelect.selection == 0)
			{
				FrmGotoForm(GridMainForm);
			}
			break;
        default:
            break;
    }

    return false;
}

