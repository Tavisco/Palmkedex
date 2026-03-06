#ifndef _ARM_CALLS_H_
#define _ARM_CALLS_H_



void armCallsInit(void *emulStateP, void *call68KFuncP);

unsigned long armCallDo(unsigned long m68kFunc, const void *stackParams, unsigned long paramLen);

#if defined(__ARM__)
	static void* __get_ptr(const void *ptr)
	{
		return (void*)ptr;
	}
#elif defined(__mips__)
	static void* __get_ptr(const void *ptr)
	{
		void* ret;

		asm volatile(
			".set push					\n\t"
			".set noreorder				\n\t"
			"	bgezal	$zero, 1f		\n\t"
			"	lw		%0, 0($ra)		\n\t"
			"	.word	.				\n\t"
			"1:							\n\t"
			"	subu	%0, %0, $ra		\n\t"
			"	subu	%0, %1, %0		\n\t"
			".set pop					\n\t"
			:"=&r"(ret)
			:"r"(ptr)
			:"ra"
		);

		return ret;
	}
#elif defined(__i386__)
	static void* __get_ptr(const void *ptr)
	{
		unsigned long base;

		asm(
			"	call 1f		\n\t"
			"1:				\n\t"
			"	pop %0		\n\t"
			:"=r"(base)
		);
		return (void*)(((unsigned long)ptr) - 0x10000000 + (base & 0xffff0000));
	}
#else
	#error "not sure how to handle this platform"
#endif

#endif
