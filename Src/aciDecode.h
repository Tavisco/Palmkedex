#ifndef _ACI_DECODE_H_
#define _ACI_DECODE_H_

#include <stdbool.h>
#include <stdint.h>


struct PixelRange {					//assembly assumes this is 8 bytes big
	uint8_t index, r, g, b;			//assembly assumes "index" is at offset 0
	uint16_t start;					//assembly assumes this is at offset 4
	uint16_t end;					//assembly assumes this is at offset 6
};	//iface code assumes this struct is same size and coincides with "struct ColortableEntry"

//low-level bits decoder
bool aciDecodeBits(uint8_t *dst, uint16_t dstExtraStride, uint16_t w, uint16_t h, const struct PixelRange *ranges, uint16_t numColors, const uint8_t *src, const uint8_t *srcEnd);




#endif
