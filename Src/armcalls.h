#ifndef _ARM_CALLS_H_
#define _ARM_CALLS_H_



void armCallsInit(void *emulStateP, void *call68KFuncP);

unsigned long armCallDo(unsigned long m68kFunc, const void *stackParams, unsigned long paramLen);

#endif
