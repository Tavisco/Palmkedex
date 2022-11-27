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

void* memset (void *p, int c, UInt32 l) {
    return MemSet(p, l, c);
}

void* memcpy (void *d, const void *s, UInt32 l) {
    return MemMove(d, s, l);
}
