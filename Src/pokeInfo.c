#include "pokeInfo.h"
#include "Src/Palmkedex.h"


//for compression used for descriptions
#define MIN_VALID_CHAR			0x20
#define MIN_VALID_INPUT_CHAR	0x20
#define MAX_VALID_INPUT_CHAR	0x7e
#define CHAR_POKEMON			(MAX_VALID_INPUT_CHAR + 1)
#define MAX_VALID_CHAR			CHAR_POKEMON		//for compression
#define TERMINATOR_CHAR			(MAX_VALID_CHAR + 1)
#define NUM_VALID_CHARS			(TERMINATOR_CHAR - MIN_VALID_CHAR + 1)

struct PokeInfoRes {
	UInt16 numPokes;
	UInt8 offsets[];	//12 bits each, FAT12-like LE
	//data
};

/*
	each pokemon info is stored as:
		uint8 hp, atk, def, spAtk, spDef, speed
		then bit-packed data:
			3 bit name len (offset by 4)
			6-bit chars of name from the charset of " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-" as per the length given above. Padded with spaces at end of Len is under 4 (the minimum representable length)
			5-bit type1
			1-bit flag set to 1 if we have a type 2
			5-bit type 2
*/

struct PerPokeCompressedStruct {
	struct PokeStats stats;
	UInt8 packedData[];
};

struct PerPokeDecompressedStruct {
	struct PokeInfo info;
	char name[POKEMON_NAME_LEN - 1];
};

struct PerItemDecompressedStruct {
	UInt8 type;
	char name[POKEMON_NAME_LEN - 1];
};

struct CompressedDescrs {
	UInt16 numStrings;
	UInt16 rangeLengths[NUM_VALID_CHARS - 1];
	UInt16 stringOffsets[];	//	[numStrings + 1]
//	UInt8 data[]
};

struct BitBufferR {		//for compressed descr
	const UInt8 *src, *srcEnd;
	UInt8 bitBuf;
	UInt8 numBitsHere;
};

struct BitBufferR2 {	//for compressed poke info
	const UInt8 *src;
	UInt16 bitBuf;
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

MemHandle pokeImageGet(UInt16 pokeID, UInt8 type)
{
	DmOpenRef dbRef;
	UInt32 resource, database, globalSlot;

	if (type == POKE_SPRITE) {
		database = SPRITE_RESOURCE_DB;
		resource = SPRITE_RESOURCE_TYPE;
		globalSlot = GLOBALS_SLOT_SPRITE_DB;
	} else if (type == POKE_ICON) {
		database = ICON_RESOURCE_DB;
		resource = ICON_RESOURCE_TYPE;
		globalSlot = GLOBALS_SLOT_ICON_DB;
	} else {
		ErrFatalDisplay("Tried to get invalid type");
		return NULL;
	}

	if (!(dbRef = globalsSlotVal(globalSlot))) {
		dbRef = DmOpenDatabaseByTypeCreator(database, appFileCreator, dmModeReadOnly);
		*globalsSlotPtr(globalSlot) = dbRef;
	}

	if (dbRef)
		return DmGet1Resource(resource, pokeID);

	return NULL;
}

void pokeImageRelease(MemHandle pokeImage, UInt8 type)
{
	DmOpenRef dbRef;

	DmReleaseResource(pokeImage);

	if (type == POKE_SPRITE)
	{
		dbRef = globalsSlotVal(GLOBALS_SLOT_SPRITE_DB);
		*globalsSlotPtr(GLOBALS_SLOT_SPRITE_DB) = NULL;
		DmCloseDatabase(dbRef);
	}
}

static inline UInt8 __attribute__((always_inline)) bbReadN(struct BitBufferR2 *bb, UInt8 n)
{
	UInt8 ret;

	while (bb->numBitsHere < n) {
		bb->bitBuf += ((UInt16)(*(bb->src)++)) << bb->numBitsHere;
		bb->numBitsHere += 8;
	}

	ret = bb->bitBuf & ((1 << n) - 1);
	bb->bitBuf >>= n;
	bb->numBitsHere -= n;

	return ret;
}


static Boolean itemGetAllInfo(struct PerItemDecompressedStruct *infoDst, char *nameDst, UInt16 pokeID)
{
	static const char infoCharset[] = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-.'";
	MemHandle infoResH = NULL;
	const struct PerItemCompressedStruct *src;
	const struct PokeInfoRes *infoRes = NULL;
	UInt16 encodedOffset;
	
	UInt8 nameLen, i;
	const UInt8 *p;

	DmOpenRef dbRef = DmOpenDatabaseByTypeCreator('ITEM', appFileCreator, dmModeReadOnly);
	if (!dbRef)
	{
		ErrFatalDisplay("Failed to find item database!");
		return false;
	}

	infoResH = DmGet1Resource('INFO', 2);
	infoRes = MemHandleLock(infoResH);

	if (!pokeID || pokeID >= infoRes->numPokes + 1) {
		MemHandleUnlock(infoResH);
		return false;
	}

	pokeID--;

	//find where the offset is stored (3 bytes)
	p = (UInt8*)infoRes->offsets;
	p += (pokeID / 2) * 3;

	//get the offset
	if (pokeID & 1)
		encodedOffset = (((UInt16)p[1] & 0xf0) << 4) + p[2];
	else
		encodedOffset = (((UInt16)p[1] & 0x0f) << 8) + p[0];

    // assume infoRes points at the start of the 'INFO' resource in memory
    UInt8 *base      = (UInt8*)infoRes;
    UInt16 numPokes  = infoRes->numPokes;
    UInt16 headerLen = 2 /* numPokes */;
    // offsets are packed 12‑bits each: 3 bytes per pair, +2 if odd
    UInt16 tableLen  = (numPokes/2)*3 + (numPokes&1 ? 2 : 0);
    UInt8 *dataStart = base + headerLen + tableLen;
    UInt32 actualOffset = 8 * pokeID + encodedOffset;

    p = dataStart + actualOffset;

	struct BitBufferR2 bb = {
        .src         = (const UInt8*)p, 
        .bitBuf      = 0,
        .numBitsHere = 0
    };

    nameLen = 4 + bbReadN(&bb, 5);
	char *out = nameDst;             // remember start
	for (UInt8 i = 0; i < nameLen; i++) {
		char ch = infoCharset[ bbReadN(&bb, 6) ];
		if (nameDst) *nameDst++ = ch;
	}

	if (out) {
		while (nameDst > out && nameDst[-1] == ' ')
			nameDst--;
		*nameDst = '\0';
	}

	if (infoDst) {
		infoDst->type = bbReadN(&bb, 5);
	}

	MemHandleUnlock(infoResH);
	DmCloseDatabase(dbRef);

	return true;
}

static Boolean pokeGetAllInfo(struct PokeInfo *infoDst, char *nameDst, UInt16 pokeID)
{
	static const char infoCharset[64] = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-";
	MemHandle infoResH = globalsSlotVal(GLOBALS_SLOT_POKE_INFO_STATE_H);
	const struct PerPokeCompressedStruct *src;
	const struct PokeInfoRes *infoRes = NULL;
	UInt16 encodedOffset, actualOffset = 0;
	struct BitBufferR2 bb = {};
	UInt8 nameLen, i;
	const UInt8 *p;


	infoRes = MemHandleLock(infoResH);

	if (!pokeID || pokeID >= infoRes->numPokes + 1) {
		MemHandleUnlock(infoResH);
		return false;
	}

	//C is 0-based
	pokeID--;

	//find where the offset is stored (3 bytes)
	p = (UInt8*)infoRes->offsets;
	p += (pokeID / 2) * 3;

	//get the offset
	if (pokeID & 1)
		encodedOffset = (((UInt16)p[1] & 0xf0) << 4) + p[2];
	else
		encodedOffset = (((UInt16)p[1] & 0x0f) << 8) + p[0];


	//get data pointer
	actualOffset += 10 * pokeID;							//base per-poke size
	actualOffset += encodedOffset;							//encoded offset
	actualOffset += (infoRes->numPokes * 3 + 1) / 2;		//length of offsets themselves
	p = ((UInt8*)infoRes->offsets) + actualOffset;

	src = (const struct PerPokeCompressedStruct*)p;
	if (infoDst)
		infoDst->stats = src->stats;
	bb.src = src->packedData;

	nameLen = 4 + bbReadN(&bb, 4);
	for (i = 0; i < nameLen; i++) {

		char ch = infoCharset[bbReadN(&bb, 6)];
		if (nameDst)
			*nameDst++ = ch;
	}
	if (nameDst) {
		while (*nameDst == ' ')	//remove end-space-pad
			nameDst--;
		*nameDst++ = 0;
	}
	if (infoDst) {
		infoDst->type[0] = bbReadN(&bb, 5);
		if (bbReadN(&bb, 1))
			infoDst->type[1] = bbReadN(&bb, 5);
		else
			infoDst->type[1] = PokeTypeNone;
	}

	MemHandleUnlock(infoResH);

	return true;
}

void pokeNameGet(char *dst, UInt16 pokeID)
{
	if (!pokeGetAllInfo(NULL, dst, pokeID))
		StrCopy(dst, "<UNKNOWN>");
}

void itemNameGet(char *dst, UInt16 pokeID)
{
	if (!itemGetAllInfo(NULL, dst, pokeID))
		StrCopy(dst, "<UNKNOWN>");
}

void pokeInfoGet(struct PokeInfo *info, UInt16 pokeID)
{
	if (!pokeGetAllInfo(info, NULL, pokeID))
		MemSet(info, sizeof(struct PokeInfo), 0);
}

UInt8 pokeGetTypeEffectiveness(enum PokeType of, enum PokeType on)
{
	return mTypeEffectiveness[of][on] * 25;
}

static UInt8 bbRead1(struct BitBufferR *bb)	//read a bit
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

char* __attribute__((noinline)) pokeDescrGet(MemHandle hndl, UInt16 pokeID)
{
	const struct CompressedDescrs *cd;
	const UInt8 *data;
	char *ret = NULL;

	if (!pokeID)
		return NULL;

	pokeID += 1;

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
				val = val * 2 + bbRead1(&bb);
			
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
					
					val = val * 2 + bbRead1(&bb);
				}
				
				while ((min >> 14) == 1 && (max >> 14) == 2) {
					
					min = (min << 1) ^ 0x8000U;
					max = (max << 1) ^ 0x8001U;
					
					val = (val & 0x8000U) + (val & 0x3fff) * 2 + bbRead1(&bb);
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
	*globalsSlotPtr(GLOBALS_SLOT_POKE_INFO_STATE_H) = DmGet1Resource('INFO', 0);
	*globalsSlotPtr(GLOBALS_SLOT_SPRITE_DB) = NULL;
}

void pokeInfoDeinit(void)
{
	*globalsSlotPtr(GLOBALS_SLOT_POKE_INFO_STATE_H) = NULL;
}
