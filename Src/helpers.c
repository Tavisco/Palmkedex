#include <PalmOS.h>

void* malloc(UInt32 sz) { return MemPtrNew(sz); }

void free(void *p) { if(p) MemPtrFree(p); }

void* realloc(void *p, UInt32 sz) { 
    MemPtrResize(p, sz); 
    return p;
}

void* calloc(UInt32 qtty, UInt32 sz) { 
    MemPtr p = MemPtrNew(sz*qtty);
    MemSet(p, sz*qtty, 0);
    return p;
}
