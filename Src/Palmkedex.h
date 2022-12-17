/*
 * Palmkedex.h
 *
 * header file for Palmkedex
 *
 */

#ifndef PALMKEDEX_H_
#define PALMKEDEX_H_

/*********************************************************************
 * Internal Constants
 *********************************************************************/

#define appFileCreator 'PKDX'
#define appName "Pokedex"
#define appVersionNum 0x01
#define appPrefID 0x00
#define appPrefVersionNum 0x01

#define ftrPkmnNamesNum (UInt16)1
#define ftrShrdVarsNum (UInt16)2

#define PKMN_QUANTITY    905
#define UNKNOWN_TYPE     21

#define QUADRUPLE_DAMAGE 400
#define DOUBLE_DAMAGE    200
#define HALF_DAMAGE      50
#define QUARTER_DAMAGE   25
#define NO_DAMAGE        0

#define BACKSPACE_CHAR 8
#define MAX_PKMN_NAME_LEN 11
#define MAX_SEARCH_RESULT_LEN 12
#define MAX_SEARCH_STR "...        "
#define MAX_SEARCH_PKMN_NUM 65534

typedef struct DrawState {
    BitmapPtr b;
    UInt16 rowHalfwords;
    UInt16 *bits;
} DrawState;

typedef struct SpeciesNames
{
    Char name[12];
} SpeciesNames;

typedef struct Species
{
    SpeciesNames nameList[PKMN_QUANTITY];
} Species;

typedef struct SharedVariables
{
    UInt16 selectedPkmnId;
    Char nameFilter[12];   // The current filter
    Int16 sizeAfterFiltering;
    SpeciesNames *filteredList;
    UInt16 *filteredPkmnNumbers;
    Char *pkmnFormTitle;
    Char *pkmnLstNumStr;
    Char *pkmnLstNameStr;
} SharedVariables;

// Palmkedex.c
void *GetObjectPtr(UInt16 objectID);

// Main.c
Boolean MainFormHandleEvent(EventType *eventP);
Boolean PkmnMainFormHandleEvent(EventType *eventP);
void OpenAboutDialog();
void OpenMainPkmnForm(Int16 selection);
void subString(const Char *input, int offset, int len, Char *dest);
Int16 GetCurrentListSize();
UInt16 GetPkmnId(Int16 selection);

// PkmnMain.c
Boolean PkmnMainFormHandleEvent(EventType *eventP);
void LoadPkmnStats();
void SetFormTitle(SharedVariables *sharedVars);
void SetLabelInfo(UInt16 labelId, UInt8 stat, FormType *frm);
void DrawTypes(UInt8 *pkmnBytes);
void SetDescriptionField(UInt16 selectedPkmnId);

// PkmnType.c
Boolean PkmnTypeFormHandleEvent(EventType *eventP);

#endif /* PALMKEDEX_H_ */