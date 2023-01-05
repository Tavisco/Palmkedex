#ifndef _MY_TRG_H_
#define _MY_TRG_H_


#define _TRG_H_	//to make sure stock isnt included



#include "PalmTypes.h"

#define TRGSysFtrID             'TRG '

#ifdef __GNUC__

#define _TRG_CALL_WITH_16BIT_SELECTOR(table, vector, selector)	__attribute__((__raw_inline__(0x343C, selector, 0x4E40 + table, vector)));

#elif defined (__MWERKS__)	/* The equivalent in CodeWarrior syntax */

#define _TRG_CALL_WITH_16BIT_SELECTOR(table, vector, selector) \
    = { 0x343C, selector, 0x4E40 + table, vector }

#endif

#define TRG_TRAP(sel) \
        _TRG_CALL_WITH_16BIT_SELECTOR(_SYSTEM_TABLE, sysTrapOEMDispatch, sel)

#ifdef BUILDING_EXTENSION
    #define EXT_TRAP(x)
#else
    #define EXT_TRAP(x) TRG_TRAP(x)
#endif



#include "Silk.h"
#include "Vga.h"



#endif
