/*
 * Palmkedex.h
 *
 * header file for Palmkedex
 *
 * This wizard-generated code is based on code adapted from the
 * stationery files distributed as part of the Palm OS SDK 4.0.
 *
 * Copyright (c) 1999-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 */
 
#ifndef PALMKEDEX_H_
#define PALMKEDEX_H_


/*********************************************************************
 * Internal Constants
 *********************************************************************/

#define appFileCreator			'PKDX'
#define appName					"Pokedex"
#define appVersionNum			0x01
#define appPrefID				0x00
#define appPrefVersionNum		0x01

#define ftrPkmnNamesNum (UInt16)1
#define ftrShrdVarsNum (UInt16)2


#define PKMN_QUANTITY 256
typedef struct SpeciesNames
{
    char name[18];
} SpeciesNames;

typedef struct Species
{
    SpeciesNames nameList[PKMN_QUANTITY];
} Species;

typedef struct SharedVariables
{
	Int16 selectedPkmnId; // DB Index of selected homework on the list
} SharedVariables;

// Palmkedex.c
void *GetObjectPtr(UInt16 objectID);

// Main.c
Boolean MainFormHandleEvent(EventType * eventP);
Boolean PkmnMainFormHandleEvent(EventType * eventP);
void OpenAboutDialog();
void PopulateList();
void OpenMainPkmnForm(Int16 selection);

// PkmnMain.c
void LoadPkmnStats();
void SetFormTitle(Int16 pkmn);
void SetLabelInfo(UInt16 labelId, UInt8 stat, FormType *frm);

#endif /* PALMKEDEX_H_ */