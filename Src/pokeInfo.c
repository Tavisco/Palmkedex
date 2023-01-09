#include "pokeInfo.h"
#include "Rsc/pkmn_names.h"


//for compression used for descriptions
#define MIN_VALID_CHAR			0x20
#define MIN_VALID_INPUT_CHAR	0x20
#define MAX_VALID_INPUT_CHAR	0x7e
#define CHAR_POKEMON			(MAX_VALID_INPUT_CHAR + 1)
#define MAX_VALID_CHAR			CHAR_POKEMON		//for compression
#define TERMINATOR_CHAR			(MAX_VALID_CHAR + 1)
#define NUM_VALID_CHARS			(TERMINATOR_CHAR - MIN_VALID_CHAR + 1)

#define MAX_DESCR_LEN		256		//compressor can handle more but we assume no more than this here


struct PokeStruct {
	char name[12];
	struct PokeInfo info;
} __attribute__((packed));

struct CompressedDescrs {
	UInt16 numStrings;
	UInt16 rangeLengths[NUM_VALID_CHARS - 1];
	UInt16 stringOffsets[];	//	[numStrings + 1]
//	UInt8 data[]
};

struct BitBufferR {
	const UInt8 *src, *srcEnd;
	UInt8 bitBuf;
	UInt8 numBitsHere;
};

static const UInt8 mTypeEffectiveness[PokeTypesCount][PokeTypesCount] = {
	//effectiveness of type N on type M is encoded in [N][M]
	//16 = 4x, 8x = 2x, 4 = 1x, 2 = 0.5x, 1 = 0.25x, 0 = 0x
	[PokeTypeNone]		= {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4},
	[PokeTypeNormal]	= {4,4,4,4,4,4,2,4,4,4,4,0,4,4,4,2,4,4,4,4,4},
	[PokeTypeFire]		= {4,4,2,2,8,4,2,4,8,4,4,4,8,4,4,8,4,2,4,4,4},
	[PokeTypeWater]		= {4,4,8,2,2,4,8,8,4,4,4,4,4,4,4,4,4,2,4,4,4},
	[PokeTypeGrass]		= {4,4,2,8,2,4,8,8,4,2,4,4,2,2,4,2,4,2,4,4,4},
	[PokeTypeElectric]	= {4,4,4,8,2,2,4,0,4,8,4,4,4,4,4,4,4,2,4,4,4},
	[PokeTypeRock]		= {4,4,8,4,4,4,4,2,8,8,2,4,8,4,4,2,4,4,4,4,4},
	[PokeTypeGround]	= {4,4,8,4,2,8,8,4,4,0,4,4,2,8,4,8,4,4,4,4,4},
	[PokeTypeIce]		= {4,4,2,2,8,4,4,8,2,8,4,4,4,4,4,2,4,8,4,4,4},
	[PokeTypeFlying]	= {4,4,4,4,8,2,2,4,4,4,8,4,8,4,4,2,4,4,4,4,4},
	[PokeTypeFighting]	= {4,8,4,4,4,4,8,4,8,2,4,0,2,2,2,8,8,4,2,4,4},
	[PokeTypeGhost]		= {4,0,4,4,4,4,4,4,4,4,4,8,4,4,8,4,2,4,4,4,4},
	[PokeTypeBug]		= {4,4,2,4,8,4,4,4,4,2,2,2,4,2,8,2,8,4,2,4,4},
	[PokeTypePoison]	= {4,4,4,4,8,4,2,2,4,4,4,2,4,2,4,0,4,4,8,4,4},
	[PokeTypePsychic]	= {4,4,4,4,4,4,4,4,4,4,8,4,4,8,2,2,0,4,4,4,4},
	[PokeTypeSteel]		= {4,4,2,2,4,2,8,4,8,4,4,4,4,4,4,2,4,4,8,4,4},
	[PokeTypeDark]		= {4,4,4,4,4,4,4,4,4,4,2,8,4,4,8,4,2,4,2,4,4},
	[PokeTypeDragon]	= {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,2,4,8,0,4,4},
	[PokeTypeFairy]		= {4,4,2,4,4,4,4,4,4,4,8,4,4,2,4,2,8,8,4,4,4},
	[PokeTypeUnknown]	= {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4},
	[PokeTypeShadow]	= {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4},
};

UInt16 pokeGetNumber(void)
{
	return sizeof(pkmnsNames) / sizeof(*pkmnsNames);
}

MemHandle pokeImageGet(UInt16 pokeID)
{
	MemHandle imgMemHandle = NULL;
	DmOpenRef dbRef;

	dbRef = DmOpenDatabaseByTypeCreator('pSPR', appFileCreator, dmModeReadOnly);
	if (dbRef) {

		imgMemHandle = DmGet1Resource('pSPT', pokeID);
		DmCloseDatabase(dbRef);
	}

	return imgMemHandle;
}

void pokeImageRelease(MemHandle pokeImage)
{
	DmReleaseResource(pokeImage);
}

static const struct PokeStruct* pokeGetStruct(UInt16 pokeID)
{
	const struct PokeStruct *structs = globalsSlotVal(GLOBALS_SLOT_POKE_INFO_STATE);

	if (!pokeID--)
		return NULL;

	if (MemPtrSize((void*)structs) <= sizeof(struct PokeStruct[pokeID]))
		return NULL;

	return &structs[pokeID];
}

const char* pokeNameGet(UInt16 pokeID)
{
	const struct PokeStruct *ps = pokeGetStruct(pokeID);

	return ps ? ps->name : "<UNKNOWN>";
}

void pokeInfoGet(struct PokeInfo *info, UInt16 pokeID)
{
	const struct PokeStruct *ps = pokeGetStruct(pokeID);

	if (!ps)
		MemSet(info, sizeof(struct PokeInfo), 0);
	else
		*info = ps->info;
}

UInt8 pokeGetTypeEffectiveness(enum PokeType of, enum PokeType on)
{
	return mTypeEffectiveness[of][on] * 25;
}

static UInt8 bbRead(struct BitBufferR *bb)	//read a bit
{
	UInt8 ret;
	
	if (!bb->numBitsHere) {
		
		bb->numBitsHere = 8;
		if (bb->src == bb->srcEnd)
			bb->bitBuf = 0;
		else
			bb->bitBuf = *(bb->src)++;
	}
	
	bb->numBitsHere--;
	ret = bb->bitBuf & 1;
	bb->bitBuf >>= 1;

	return ret;
}

static UInt32 __attribute__((always_inline))  mul16x16_32(UInt16 a, UInt16 b)
{
	UInt32 ret;
	
	//efficiently does what this would also do:
	//return (UInt32)(UInt16)a * (UInt32)(UInt16)b;
	
	asm(
		"move.w %1, %0	\n"
		"mulu.w %2, %0	\n"
		:"=&d"(ret)
		:"d"(a), "Ac"(b)
		:"cc"
	);
	
	return ret;
}

static UInt16 __attribute__((always_inline)) div32_16(UInt32 num, UInt16 denom)
{
	
	//efficiently does what this would also do:
	//return (UInt16)((UInt32)num / (UInt16)denom);
	
	asm(
		"divu.w	%1, %0"
		:"+d"(num)
		:"d"(denom)
		:"cc"
	);
	
	return num;
}

char* __attribute__((noinline)) pokeDescrGet(UInt16 pokeID)
{
	const struct CompressedDescrs *cd;
	const UInt8 *data;
	MemHandle hndl;
	char *ret = NULL;

	if (!pokeID)
		return NULL;

	hndl = DmGetResource('DESC', 0);
	if (!hndl)
		return NULL;

	cd = MemHandleLock(hndl);
	data = (const UInt8*)cd;		//offsets start at the start of the data structure
	
	if (pokeID <= cd->numStrings) {
		
		struct BitBufferR bb = {.src = data + cd->stringOffsets[pokeID - 1], .srcEnd = data + cd->stringOffsets[pokeID], };
		UInt16 *start = MemPtrNew(NUM_VALID_CHARS * 2);
		UInt16 *end = MemPtrNew(NUM_VALID_CHARS * 2);
		
		ret = MemPtrNew(MAX_DESCR_LEN + 1);
		
		if (start && end && ret) {
			UInt16 i, min = 0x0000, max = 0xffff, val = 0, retPos = 0;
			
			//read ranges
			start[0] = 0;
			for (i = 0; i < NUM_VALID_CHARS - 1; i++)
				end[i] = start[i + 1] = start[i] + cd->rangeLengths[i];
			end[i] = 0x4000;
			
			//extract a single string
			
			//fill initial value
			for (i = 0; i < 16; i++)
				val = val * 2 + bbRead(&bb);
			
			//decompress symbols
			while (retPos < MAX_DESCR_LEN) {
				
				UInt32 width = (UInt32)max - min + 1;
				UInt32 above = val - min;
				UInt16 now = ((above + 1) * 0x4000U - 1) / width;
				UInt8 idxNow;
				
				//the math is as follows, width range is 0x00001..0x10000
				//now = ((above + 1) * 0x4000U - 1) / width;
				if (!(UInt16)width) {
					
					now = ((above + 1) * 0x4000U - 1) >> 16;
				}
				else {
					
					now = div32_16((above + 1) * 0x4000U - 1, width);
				}
				
				//could be faster ... later
				for (idxNow = MIN_VALID_CHAR; idxNow <= TERMINATOR_CHAR; idxNow++) {
					
					if (now >= start[idxNow - MIN_VALID_CHAR] && now < end[idxNow - MIN_VALID_CHAR])
						break;
				}
				
				//emit byte (or handle the terminator)
				if (idxNow == TERMINATOR_CHAR)
					break;
				else if (idxNow == CHAR_POKEMON) {
					
					StrCopy(ret + retPos, "POKEMON");
					retPos += 7;
				}
				else {
					
					ret[retPos++] = idxNow;
				}
				
				//calc new range
				//the math is as folows, width range is 0x00001..0x10000
				//max = min + width * end[idxNow - MIN_VALID_CHAR] / 0x4000U - 1;
				//min = min + width * start[idxNow - MIN_VALID_CHAR] / 0x4000U;
				if (!(UInt16)width) {
					max = min + end[idxNow - MIN_VALID_CHAR] * 4 - 1;
					min = min + start[idxNow - MIN_VALID_CHAR] * 4;
				}
				else {
					max = min + mul16x16_32(width, end[idxNow - MIN_VALID_CHAR]) / 0x4000U - 1;
					min = min + mul16x16_32(width, start[idxNow - MIN_VALID_CHAR]) / 0x4000U;
				}
				while ((min >> 15) == (max >> 15)) {
					
					min = min * 2;
					max = max * 2 + 1;
					
					val = val * 2 + bbRead(&bb);
				}
				
				while ((min >> 14) == 1 && (max >> 14) == 2) {
					
					min = (min << 1) ^ 0x8000U;
					max = (max << 1) ^ 0x8001U;
					
					val = (val & 0x8000U) + (val & 0x3fff) * 2 + bbRead(&bb);
				}
			}
			if (retPos && ret[retPos - 1] != '!')
				ret[retPos++] = '.';
			ret[retPos++] = 0;
		}
		else if (ret) {
			
			//we could have failed to alloc start or end, but did alloc ret. do not return uninitialized memory to caller
			MemPtrFree(ret);
			ret = NULL;
		}
		if (start)
			MemPtrFree(start);
		if (end)
			MemPtrFree(end);
	}
	
	MemHandleUnlock(hndl);
	DmReleaseResource(hndl);

	return ret;
}

void pokeInfoInit(void)
{
	MemHandle mh = DmGet1Resource('INFO', 0);
	void *mp = MemHandleLock(mh);

	*globalsSlotPtr(GLOBALS_SLOT_POKE_INFO_STATE) = mp;
}

void pokeInfoDeinit(void)
{
	void *memPtr = globalsSlotVal(GLOBALS_SLOT_POKE_INFO_STATE);
	MemHandle mh;

	*globalsSlotPtr(GLOBALS_SLOT_POKE_INFO_STATE) = NULL;

	mh = MemPtrRecoverHandle(memPtr);
	MemHandleUnlock(mh);
	DmReleaseResource(mh);
}