#include "armcalls.h"
#include <PceNativeCall.h>
#include <CoreTraps.h>
#include <stdint.h>


register Call68KFuncType *call68KFuncP asm ("r11");
register void *emulStateP asm ("r10");


void armCallsInit(void *set_emulStateP, void *set_call68KFuncP)
{
	emulStateP = set_emulStateP;
	call68KFuncP = set_call68KFuncP;
}

unsigned long armCallDo(unsigned long m68kFunc, const void *stackParams, unsigned long paramLen)
{
	return call68KFuncP(emulStateP, m68kFunc, stackParams, paramLen);
}

void* MemPtrNew(uint32_t size)
{
	uint32_t stackParam = __builtin_bswap32(size);

	return (void*)call68KFuncP(emulStateP, PceNativeTrapNo(sysTrapMemPtrNew), &stackParam, sizeof(stackParam) | kPceNativeWantA0);
}

void MemChunkFree(void *ptr)
{
	uint32_t stackParam = __builtin_bswap32((uintptr_t)ptr);

	call68KFuncP(emulStateP, PceNativeTrapNo(sysTrapMemChunkFree), &stackParam, sizeof(stackParam));
}

Err MemSet(void *dst, uint32_t len, uint8_t val)
{
	struct {
		uint32_t dst;
		uint32_t len;
		uint16_t val;
	} stackParams = {
		.dst = __builtin_bswap32((uintptr_t)dst),
		.len = __builtin_bswap32(len),
		.val = val,
	};

	return call68KFuncP(emulStateP, PceNativeTrapNo(sysTrapMemSet), &stackParams, sizeof(stackParams));
}

Err MemMove(void *dst, void *src, uint32_t len)
{
	struct {
		uint32_t dst;
		uint32_t src;
		uint32_t len;
	} stackParams = {
		.dst = __builtin_bswap32((uintptr_t)dst),
		.src = __builtin_bswap32((uintptr_t)src),
		.len = __builtin_bswap32(len),
	};

	return call68KFuncP(emulStateP, PceNativeTrapNo(sysTrapMemMove), &stackParams, sizeof(stackParams));
}

uint16_t MemPtrResize(void *ptr, uint32_t len)
{
	struct {
		uint32_t ptr;
		uint32_t len;
	} stackParams = {
		.ptr = __builtin_bswap32((uintptr_t)ptr),
		.len = __builtin_bswap32(len),
	};

	return call68KFuncP(emulStateP, PceNativeTrapNo(sysTrapMemPtrResize), &stackParams, sizeof(stackParams));
}

uint32_t MemPtrSize(void *ptr)
{
	uint32_t stackParam = __builtin_bswap32((uintptr_t)ptr);

	return call68KFuncP(emulStateP, PceNativeTrapNo(sysTrapMemPtrSize), &stackParam, sizeof(stackParam));
}

uint16_t FrmCustomAlert(uint16_t id, char *s1, char *s2, char *s3)
{
	struct {
		uint16_t id;
		uint32_t s1, s2, s3;
	} __attribute__((packed)) stackParams = {
		.id = __builtin_bswap16(id),
		.s1 = __builtin_bswap32((uintptr_t)s1),
		.s2 = __builtin_bswap32((uintptr_t)s2),
		.s3 = __builtin_bswap32((uintptr_t)s3),
	};

	return call68KFuncP(emulStateP, PceNativeTrapNo(sysTrapFrmCustomAlert), &stackParams, sizeof(stackParams));
}