#ifndef _POKE_INFO_H_
#define _POKE_INFO_H_

#include <PalmOS.h>


//this module is responsible for per-pokemon info
//indices start at 1, precisely as pokemon do

#define TOTAL_POKE_COUNT_ZERO_BASED 1009 // used as convenience for filtering pokes. This is POKE_COUNT - 1

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
	
	PokeTypeFirst = PokeTypeNormal,
	PokeTypeLast = PokeTypeShadow,
};

struct PokeStats {
	UInt8 hp, atk, def, spAtk, spDef, speed;
};

struct PokeInfo {
	struct PokeStats stats;
	UInt8 type[2];
};

void pokeInfoInit(void);

MemHandle pokeImageGet(UInt16 pokeID);		//call pokeImageRelease() when done!
void pokeImageRelease(MemHandle pokeImage);

char* pokeDescrGet(UInt16 pokeID);			//returns a pointer that the caller MUST free

//FAST calls. These next three functions ae VERY FAST, you need not cache their results, just call them as needed!
void pokeNameGet(char *dst, UInt16 pokeID);	//buffer should be >= POKEMON_NAME_LEN + 1 bytes long...
void pokeInfoGet(struct PokeInfo *info, UInt16 pokeID);
UInt8 pokeGetTypeEffectiveness(enum PokeType of, enum PokeType on);

void pokeInfoDeinit(void);

#endif
