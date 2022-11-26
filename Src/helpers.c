#include <PalmOS.h>

void* malloc(UInt32 sz) { return MemPtrNew(sz); }
void free(void*p) { if(p) MemPtrFree(p); }
void realloc(void*p, UInt32 sz) { MemPtrResize(p, sz); }
void calloc(UInt32 qtty, UInt32 sz) { return MemPtrNew(sz); }