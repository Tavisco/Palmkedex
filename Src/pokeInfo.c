#include "pokeInfo.h"
#include "Rsc/pkmn_names.h"


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

void pokeNameGet(char *dst, UInt16 pokeID)
{
	if (!pokeID || pokeID > pokeGetNumber())
		dst[0] = 0;
	else
		StrCopy(dst, pkmnsNames[pokeID - 1].name);
}

void pokeInfoGet(struct PokeInfo *info, UInt16 pokeID)
{
	MemHandle hndl = DmGet1Resource('pINF', pokeID);

	if (!hndl) {
		MemSet(info, sizeof(struct PokeInfo), 0);
		return;
	}

	*info = *(const struct PokeInfo*)MemHandleLock(hndl);
	MemHandleUnlock(hndl);
	DmReleaseResource(hndl);
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

char* pokeDescrGet(UInt16 pokeID)
{
	MemHandle hndl = DmGet1Resource('pDSC', pokeID);
	const char *srcPtr;
	char *dst;

	if (!hndl)
		return NULL;

	srcPtr = MemHandleLock(hndl);	//we assume the resrouce is 0-terminated

	dst = MemPtrNew(StrLen(srcPtr) + 1);

	if (dst)
		StrCopy(dst, srcPtr);

	MemHandleUnlock(hndl);
	DmReleaseResource(hndl);

	return dst;
}