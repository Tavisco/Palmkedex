#include <PalmOS.h>
#include <stdarg.h>

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
