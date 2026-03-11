#include "pnoRuntime.h"
#include <PceNativeCall.h>
#include <PalmOS.h>
#include <CoreTraps.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(__ARM__)
	register Call68KFuncType *call68KFuncP asm ("r11");
	register void *emulStateP asm ("r10");
#elif defined(__mips__)
	register Call68KFuncType *call68KFuncP asm ("s6");
	register void *emulStateP asm ("s7");
#elif defined(__i386__)
	static Call68KFuncType *_call68KFuncP;
	static void *_emulStateP;

	#define REDIRECTOR(x)	(*(typeof(_##x)*)(__get_ptr(&_##x)))

	//good news: we have real globals on x86
	//bad news: gcc has no idea where they are and assume they are at 0x10000000
	//which is true, IFF we are loaded at 0x10000000
	//i coudl make relocs work, but i did enough work just making gcc emit PE files
	//so this'll do :D
	#define call68KFuncP	REDIRECTOR(call68KFuncP)
	#define emulStateP		REDIRECTOR(emulStateP)

#else
	#error "not sure how to hndle this platform"
#endif


uint32_t read32(const void *fromP)			//read unaligned 32 bit in BE
{
	const uint8_t *from = fromP;
	uint32_t ret = 0;

	ret = (ret << 8) + from[0];
	ret = (ret << 8) + from[1];
	ret = (ret << 8) + from[2];
	ret = (ret << 8) + from[3];

	return ret;
}

uint16_t read16(const void *fromP)			//read unaligned 16 bit in BE
{
	const uint8_t *from = fromP;
	uint16_t ret = 0;

	ret = (ret << 8) + from[0];
	ret = (ret << 8) + from[1];

	return ret;
}

void write32(void *dstP, uint32_t val)		//write unaligned 32 bit in LE
{
	uint8_t *dst = dstP;

	dst[0] = val;
	dst[1] = val >> 8;
	dst[2] = val >> 16;
	dst[3] = val >> 24;
}

void write16(void *dstP, uint16_t val)		//write unaligned 16 bit in LE
{
	uint8_t *dst = dstP;

	dst[0] = val;
	dst[1] = val >> 8;
}

void armCallsInit(void *set_emulStateP, void *set_call68KFuncP)
{
	emulStateP = set_emulStateP;
	call68KFuncP = set_call68KFuncP;
}

unsigned long armCallDo(unsigned long m68kFunc, const void *stackParams, unsigned long paramLen)
{
	return call68KFuncP(emulStateP, m68kFunc, stackParams, paramLen);
}

void* MemPtrNew(UInt32 size)
{
	uint32_t stackParam = __builtin_bswap32(size);

	return (void*)call68KFuncP(emulStateP, PceNativeTrapNo(sysTrapMemPtrNew), &stackParam, sizeof(stackParam) | kPceNativeWantA0);
}

Err MemChunkFree(void *ptr)
{
	uint32_t stackParam = __builtin_bswap32((uintptr_t)ptr);

	return call68KFuncP(emulStateP, PceNativeTrapNo(sysTrapMemChunkFree), &stackParam, sizeof(stackParam));
}

Err MemSet(void *dst, Int32 len, uint8_t val)
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

Err MemMove(void *dst, const void *src, Int32 len)
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

uint16_t MemPtrResize(void *ptr, UInt32 len)
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

UInt32 MemPtrSize(void *ptr)
{
	uint32_t stackParam = __builtin_bswap32((uintptr_t)ptr);

	return call68KFuncP(emulStateP, PceNativeTrapNo(sysTrapMemPtrSize), &stackParam, sizeof(stackParam));
}

uint16_t FrmCustomAlert(UInt16 id, const char *s1, const char *s2, const char *s3)
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

int abs(int value)
{
	return (value < 0) ? -value : value;
}


//shared startup code


#if defined(__ARM__)

	void __attribute((naked, used, section(".vector"), target("arm"))) __entry(void);
	void __attribute((naked, used, section(".vector"), target("arm"))) __entry(void)
	{
		//gcc will refuse to call a thumb function from this arm entry point no matter what we do
		//so we are forced to do it ourselves if we want to compile for thumb (we do for space)

		asm volatile(
			"1:									\n"
			"	stmfd	sp!, {r10, r11, lr}		\n"
			"	ldr		r10, =1b				\n"
			"	adr		r11, 1b					\n"
			"	ldr		r12, =ArmletMain		\n"
			"	sub		r12, r10				\n"
			"	add		r12, r11				\n"
			"	mov		lr, pc					\n"
			"	bx		r12						\n"
			"	ldmfd	sp!, {r10, r11, lr}		\n"
			"	bx		lr						\n"
		);
	}

#elif defined(__mips__)

	void __attribute((used, section(".vector"))) __entry(void);
	void __attribute((used, section(".vector"))) __entry(void)
	{
		//gcc will refuse to call a thumb function from this arm entry point no matter what we do
		//so we are forced to do it ourselves if we want to compile for thumb (we do for space)

		asm volatile(
			".set push								\n\t"
			".set noreorder							\n\t"
			"	addiu	$sp, $sp, -28				\n\t"	//mips abi expects 16 bytes it can use - give it that
			"	sw		$ra, 16($sp)				\n\t"
			"	sw		$s6, 20($sp)				\n\t"
			"	bal		ArmletMain					\n\t"
			"	sw		$s7, 24($sp)				\n\t"
			"	lw		$ra, 16($sp)				\n\t"
			"	lw		$s6, 20($sp)				\n\t"
			"	lw		$s7, 24($sp)				\n\t"
			"	jr		$ra							\n\t"
			"	addiu	$sp, $sp, 28				\n\t"
			".set pop								\n\t"
		);
	}
#elif defined(__i386__)

	void __attribute((used, section(".vector"))) __entry(void);
	void __attribute((used, section(".vector"))) __entry(void)
	{
		asm volatile(
			"	jmp ArmletMain						\n\t"
		);
	}

#else
	#error "not sure how to handle this platform"
#endif
