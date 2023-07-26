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

#ifndef __ARM__

	//globals (8 slots maximum, each stores a void*, zero-inited at app start)

	#define NUM_GLOBALS_SLOTS		8

	register void** a5 asm("a5");

	static inline void** globalsSlotPtr(UInt8 slotID)	//[0] is reserved
	{
		if (!slotID || slotID > NUM_GLOBALS_SLOTS)
			return NULL;

		return a5 + slotID;
	}

	static inline void* globalsSlotVal(UInt8 slotID)	//[0] is reserved
	{
		if (!slotID || slotID > NUM_GLOBALS_SLOTS)
			return NULL;

		return a5[slotID];
	}

	#define GLOBALS_SLOT_POKE_IMAGE			1
	#define GLOBALS_SLOT_SHARED_VARS		2
	#define GLOBALS_SLOT_OS_PATCH_STATE		3
	#define GLOBALS_SLOT_POKE_INFO_STATE_H	4
	#define GLOBALS_SLOT_IMG_DB				5
	#define GLOBALS_SLOT_PCE_CALL_FUNC		6

#endif

#include "Src/pokeInfo.h"

#define POKEMON_TYPE_IMAGES_BASE		9000

#define appFileCreator 'PKDX'
#define appName "Pokedex"
#define appVersionNum 0x01
#define appPrefID 0x00
#define appPrefVersionNum 0x01

#define QUADRUPLE_DAMAGE 400
#define DOUBLE_DAMAGE    200
#define HALF_DAMAGE      50
#define QUARTER_DAMAGE   25
#define NO_DAMAGE        0

#define BACKSPACE_CHAR 8
#define MAX_PKMN_NAME_LEN 11
#define MAX_SEARCH_RESULT_LEN 64
#define MAX_SEARCH_STR "...        "
#define MAX_SEARCH_PKMN_NUM 0
#define MAX_DESCR_LEN		256		//compressor can handle more but we assume no more than this here

#define POKE_SPRITE				0
#define POKE_ICON				1
#define SPRITE_RESOURCE_DB		'pSPR'
#define SPRITE_RESOURCE_TYPE	'pSPT'
#define ICON_RESOURCE_DB		'pICR'
#define ICON_RESOURCE_TYPE		'pICT'

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
	Int16 selectedPkmnLstIndex;
	Boolean isQrDisplayed;

    Coord prevDispW, prevDispH;

    MemHandle indexHandle;
    const UInt16 *pokeIdsPerEachStartingLetter[26];	// A 0-terminated array of pokemon names fore each possible starting letter
} SharedVariables;


// Palmkedex.c
void *GetObjectPtr(UInt16 objectID);
Boolean isHanderaHiRes(void);
Boolean isPalmOS1(void);
void drawBackButton(UInt16 buttonID);

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
void drawBmpForType(enum PokeType type, Coord x, Coord y);

// GridMain.c
Boolean GridMainFormHandleEvent(EventType * eventP);

// PkmnType.c
Boolean PkmnTypeFormHandleEvent(EventType *eventP);

// glue.c
void BmpGlueGetDimensions(const BitmapType *bitmapP, Coord *widthP, Coord *heightP, UInt16 *rowBytesP);

// glue.c
void BmpGlueGetDimensions(const BitmapType *bitmapP, Coord *widthP, Coord *heightP, UInt16 *rowBytesP);


#endif /* PALMKEDEX_H_ */
