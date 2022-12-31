#include "pokeInfo.h"
#include "Rsc/pkmn_names.h"


//for compression used for descriptions
#define MIN_VALID_CHAR		0x20
#define MAX_VALID_CHAR		0x7e
#define TERMINATOR_CHAR		(MAX_VALID_CHAR + 1)
#define NUM_VALID_CHARS		(TERMINATOR_CHAR - MIN_VALID_CHAR + 1)

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



UInt16 pokeGetNumber(void)
{
	return sizeof(pkmnsNames) / sizeof(*pkmnsNames);
}

MemHandle pokeImageGet(UInt16 pokeID)
{
	MemHandle imgMemHandle = NULL;
	DmOpenRef dbRef;

	dbRef = DmOpenDatabaseByTypeCreator('pSPR', 'PKSP', dmModeReadOnly);
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

static Boolean pokeGetStruct(struct PokeStruct *ps, UInt16 pokeID)
{
	const struct PokeStruct *src;
	Boolean ret = false;
	MemHandle mh;

	if (!pokeID--)
		return false;

	mh = DmGet1Resource('INFO', 0);
	if (!mh)
		return false;

	src = MemHandleLock(mh);
	if (pokeID < MemHandleSize(mh) / sizeof(struct PokeStruct)) {

		ret = true;
		*ps = src[pokeID];
	}
	MemHandleUnlock(mh);
	DmReleaseResource(mh);

	return ret;
}

void pokeNameGet(char *dst, UInt16 pokeID)
{
	struct PokeStruct ps;

	if (!pokeGetStruct(&ps, pokeID))
		StrCopy(dst, "<UNKNOWN>");
	else
		StrCopy(dst, ps.name);
}

void pokeInfoGet(struct PokeInfo *info, UInt16 pokeID)
{
	struct PokeStruct ps;

	if (!pokeGetStruct(&ps, pokeID))
		MemSet(info, sizeof(struct PokeInfo), 0);
	else
		*info = ps.info;

	//FOR NOW, adjust "none" type to 21, as existing code expects
	if (info->type[0] == PokeTypeNone)
		info->type[0] = PokeTypeNone21;
	if (info->type[1] == PokeTypeNone)
		info->type[1] = PokeTypeNone21;

}

UInt8 pokeGetTypeEffectiveness(enum PokeType of, enum PokeType on)
{
	MemHandle pEffHndl = DmGet1Resource('pEFF', (UInt8)of);
	const UInt8 *vals = MemHandleLock(pEffHndl);
	UInt8 ret = vals[((UInt8)on) - 1];

	MemHandleUnlock(pEffHndl);
	DmReleaseResource(pEffHndl);

	return ret;
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
	
	hndl = DmGet1Resource('DESC', 0);
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
			end[i] = 0x8000;
			
			//extract a single string
			
			//fill initial value
			for (i = 0; i < 16; i++)
				val = val * 2 + bbRead(&bb);
			
			//decompress symbols
			while (retPos < MAX_DESCR_LEN) {
				
				UInt32 width = (UInt32)max - min + 1;
				UInt32 above = val - min;
				UInt16 now = ((above + 1) * 0x8000U - 1) / width;
				UInt8 idxNow;
				
				//the math is as follows, width range is 0x00001..0x10000
				//now = ((above + 1) * 0x8000U - 1) / width;
				if (!(UInt16)width) {
					
					now = ((above + 1) * 0x8000U - 1) >> 16;
				}
				else {
					
					now = div32_16((above + 1) * 0x8000U - 1, width);
				}
				
				//could be faster ... later
				for (idxNow = MIN_VALID_CHAR; idxNow <= TERMINATOR_CHAR; idxNow++) {
					
					if (now >= start[idxNow - MIN_VALID_CHAR] && now < end[idxNow - MIN_VALID_CHAR])
						break;
				}
				
				//emit byte (or handle the terminator)
				if (idxNow == TERMINATOR_CHAR)
					break;
				ret[retPos++] = idxNow;
				
				//calc new range
				//the math is as folows, width range is 0x00001..0x10000
				//max = min + width * end[idxNow - MIN_VALID_CHAR] / 0x8000U - 1;
				//min = min + width * start[idxNow - MIN_VALID_CHAR] / 0x8000U;
				if (!(UInt16)width) {
					max = min + end[idxNow - MIN_VALID_CHAR] * 2 - 1;
					min = min + start[idxNow - MIN_VALID_CHAR] * 2;
				}
				else {
					max = min + mul16x16_32(width, end[idxNow - MIN_VALID_CHAR]) / 0x8000U - 1;
					min = min + mul16x16_32(width, start[idxNow - MIN_VALID_CHAR]) / 0x8000U;
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