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

#include "Src/pokeInfo.h"

#define POKEMON_TYPE_IMAGES_BASE		9000

#define appFileCreator 'PKDX'
#define appName "Pokedex"
#define appVersionNum 0x01
#define appPrefID 0x00
#define appPrefVersionNum 0x01

#define ftrPokeImage			0
#define ftrShrdVarsNum			1
#define ftrOsPatchState			2
#define ftrPokeInfoState		3

#define QUADRUPLE_DAMAGE 400
#define DOUBLE_DAMAGE    200
#define HALF_DAMAGE      50
#define QUARTER_DAMAGE   25
#define NO_DAMAGE        0

#define BACKSPACE_CHAR 8
#define MAX_PKMN_NAME_LEN 11
#define MAX_SEARCH_RESULT_LEN 14
#define MAX_SEARCH_STR "...        "
#define MAX_SEARCH_PKMN_NUM 0

typedef struct SpeciesName {
	char name[POKEMON_NAME_LEN + 1];
}SpeciesName;

typedef struct SharedVariables
{
    UInt16 selectedPkmnId;
    Char nameFilter[POKEMON_NAME_LEN + 1];  	 // The current filter
    UInt8 listNumsWidth;						//width of numbers in list view
    UInt16 sizeAfterFiltering;
    UInt16 filteredPkmnNumbers[MAX_SEARCH_RESULT_LEN];


    MemHandle indexHandle;
    const UInt16 *pokeIdsPerEachStartingLetter[26];	// A 0-terminated array of pokemon names fore each possible starting letter
} SharedVariables;

// Palmkedex.c
void *GetObjectPtr(UInt16 objectID);

// Main.c
Boolean MainFormHandleEvent(EventType *eventP);
Boolean PkmnMainFormHandleEvent(EventType *eventP);
void OpenAboutDialog(void);
void OpenMainPkmnForm(Int16 selection);
void subString(const Char *input, int offset, int len, Char *dest);
Int16 GetCurrentListSize(void);
UInt16 GetPkmnId(Int16 selection);

// PkmnMain.c
Boolean PkmnMainFormHandleEvent(EventType *eventP);
void LoadPkmnStats(void);
void SetFormTitle(SharedVariables *sharedVars);	//used by PkmnType.c too
void SetLabelInfo(UInt16 labelId, UInt8 stat, FormType *frm);
void SetDescriptionField(UInt16 selectedPkmnId);

// PkmnType.c
Boolean PkmnTypeFormHandleEvent(EventType *eventP);

// glue.c
void BmpGlueGetDimensions(const BitmapType *bitmapP, Coord *widthP, Coord *heightP, UInt16 *rowBytesP);

// glue.c
void BmpGlueGetDimensions(const BitmapType *bitmapP, Coord *widthP, Coord *heightP, UInt16 *rowBytesP);

#endif /* PALMKEDEX_H_ */