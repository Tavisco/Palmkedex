#ifndef _POKE_INFO_H_
#define _POKE_INFO_H_

#include <PalmOS.h>


//this module is responsible for per-pokemon info
//indices start at 1, precisely as pokemon do

#define POKEMON_NAME_LEN		11

enum PokeType {
	PokeTypeNone,	//used as second type for pokes that lack a first
	PokeTypeNormal = 1,
	PokeTypeFire,
	PokeTypeWater,
	PokeTypeGrass,
	PokeTypeElectric,
	PokeTypeRock,
	PokeTypeGround,
	PokeTypeIce,
	PokeTypeFlying,
	PokeTypeFighting,
	PokeTypeGhost,
	PokeTypeBug,
	PokeTypePoison,
	PokeTypePsychic,
	PokeTypeSteel,
	PokeTypeDark,
	PokeTypeDragon,
	PokeTypeFairy,
	PokeTypeUnknown,
	PokeTypeShadow,
	PokeTypesCount,
	PokeTypeNone21 = 21,	//used as second type for pokes that lack a first (BEFORE REFACTORING)
};

struct PokeInfo {	//must match provided resource data
	UInt8 hp, atk, def, spAtk, spDef, speed, type[2];
};


UInt16 pokeGetNumber(void);

MemHandle pokeImageGet(UInt16 pokeID);		//call pokeImageRelease() when done!
void pokeImageRelease(MemHandle pokeImage);

void pokeNameGet(char *dst, UInt16 pokeID);	//buffer shoudl be >= POKEMON_NAME_LEN + 1 bytes long...

char* pokeDescrGet(UInt16 pokeID);			//returns a pointer that the caller MUST free

void pokeInfoGet(struct PokeInfo *info, UInt16 pokeID);

UInt8 pokeGetTypeEffectiveness(enum PokeType of, enum PokeType on);

#endif
