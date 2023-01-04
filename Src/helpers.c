#include <PalmOS.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifndef __ARM__
	static void debug_printf(const char* fmt, ...) {
	    UInt32 ftrValue;
	    char buffer[256];
	    va_list args;

	    if (FtrGet('cldp', 0, &ftrValue) || ftrValue != 0x20150103) return;

	    va_start(args, fmt);

	    if (StrVPrintF(buffer, fmt, (_Palm_va_list)args) > 255)
	        DbgMessage("DebugLog: buffer overflowed, memory corruption ahead");
	    else
	        DbgMessage(buffer);
	}
#endif

void* malloc(size_t sz) { return MemPtrNew(sz); }

void free(void *p) { if(p) MemPtrFree(p); }

void* realloc(void *p, size_t sz) {
    
    void *newPtr;
    
    if (errNone == MemPtrResize(p, sz))
    	return p;
    
    //fail means we ARE upsizing!
    
    newPtr = MemPtrNew(sz);
    if (!newPtr)
    	return NULL;
    
    MemMove(newPtr, p, MemPtrSize(p));
    MemPtrFree(p);
    return newPtr;
}

void* calloc(size_t qtty, size_t sz) {
    MemPtr p = MemPtrNew(sz*qtty);
    MemSet(p, sz*qtty, 0);
    return p;
}

void* memset (void *p, int c, size_t l) {
    MemSet(p, l, c);
    
    return p;
}

void* memcpy (void *d, const void *s, size_t l) {
    MemMove(d, s, l);
    
    return d;
}
