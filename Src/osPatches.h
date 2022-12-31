#ifndef _OS_PATCHES_H_
#define _OS_PATCHES_H_

#include <PalmOS.h>

//this module does two things:
// 1. on PalmOS 3.5 devices, PackBits compression/decompression is not
//    supported (see SDK), but PackBits is the only decent compression
//    we've got, so this module fixes that
// 2. Form bitmaps are drawn low-res since Sony Hi-Res has no ideas of
//    HiDensity bitmaps. WinDrawBitmap is patched such that form bitmas
//    are drawn in hi-res where available
//This code needs to be installed at app start and uninstalled at app end
//Install should be done only after sony hr lib has been opened

void osPatchesInstall(void);
void osPatchesRemove(void);


#endif
