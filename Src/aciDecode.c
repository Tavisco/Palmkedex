#include <string.h>
#include <stdlib.h>
#include "imgDrawInt.h"
#include "aciDecode.h"


#define FLAGS_V1			0x01		//must be set for this v1 format, if clear, format differs
#define FLAGS_HAS_CLUT		0x02		//if no, assume greyscale of given depth and no actual CLUT
#define FLAGS_BOUNDED		0x04
#define FLAGS_RFU			0xf8		//if any of these are set, we do not know how to decode this, do not try

#define LOG(...)

struct ACIhdr {		//BE for ease
	uint16_t w;
	uint16_t h;
	uint8_t flags;
	uint8_t numColorsM2;
	//if (flags & FLAGS_BOUNDED) {
	// uint8_t top, bottom, left, right, index;
	//}
	//if (flags & FLAGS_HAS_CLUT) {
	//  uint8_t clut[];//[numColorsM2 + 1][3]
	//}
	//uint8_t startVals[numColorsM2 + 1]
	//uint8_t data
} __attribute__((packed));


int aciDecode(struct DrawState *ds, const void *data, uint32_t dataSz, ImgHdrDecodedCbkF hdrCbk)
{
	uint8_t rtop = 0, rbottom = 0, cleft = 0, cright = 0, borderColorIdx = 0, *bitsP;
	const struct ACIhdr *hdr = (const struct ACIhdr*)data;
	const uint8_t *src = (const uint8_t*)(hdr + 1);
	const uint8_t *srcEnd = ((const uint8_t*)data) + dataSz;
	uint_fast16_t i, numColors, w, h, r;
	struct PixelRange *colors;
	bool success;

	if (dataSz < sizeof(struct ACIhdr))
		return -1;

	//verify we understang the version
	if (!(hdr->flags & FLAGS_V1))
		return -1;

	//verify we do not see any features we do not understand
	if (hdr->flags & FLAGS_RFU)
		return -1;

	numColors = 2 + hdr->numColorsM2;

	//at least one byte of data is mandatory, thus LE and not LT
	if (dataSz <= sizeof(struct ACIhdr) + (2 + 1) * numColors)
		return -1;

	//ask if the size is ok, get allocated a destination buffer
#ifdef __ARM__
	h = __builtin_bswap16(hdr->h);
	w = __builtin_bswap16(hdr->w);
#else
	h = hdr->h;
	w = hdr->w;
#endif

	colors = malloc(sizeof(struct PixelRange) * numColors);
	if (!colors)
		return -1;

	//read in counds if they exist
	if (hdr->flags & FLAGS_BOUNDED) {
		rtop = *src++;
		rbottom = *src++;
		cleft = *src++;
		cright = *src++;
		borderColorIdx = *src++;
	}

	//read in the CLUT, if it exists
	if (hdr->flags & FLAGS_HAS_CLUT) {
		for (i = 0; i < numColors - 1; i++) {

			colors[i].index = i;
			colors[i].r = *src++;
			colors[i].g = *src++;
			colors[i].b = *src++;
		}
	}
	else {

		for (i = 0; i < numColors - 1; i++) {

			uint16_t bri = (i * 255 + (numColors - 2) / 2) / (numColors - 2);

			colors[i].index = i;

			colors[i].r = bri;
			colors[i].g = bri;
			colors[i].b = bri;
		}
	}

	if (!hdrCbk(ds, w, h, (struct ColortableEntry*)colors, numColors - 1, !(hdr->flags & FLAGS_HAS_CLUT))) {
		free(colors);
		return -1;
	}

	//read in ranges
	colors[0].start = 0;
	for (i = 0; i < numColors - 1; i++)
		colors[i].end = colors[i + 1].start = *src++;
	colors[i].end = 256;

	//fill image with background index
	bitsP = ds->bits;
	for (r = 0; r < h; r++, bitsP += ds->rowBytes)
		memset(bitsP, colors[borderColorIdx].index, w);

	//we are ready to process image data
	success = aciDecodeBits(ds->bits + rtop * ds->rowBytes + cleft, ds->rowBytes - (w - cleft - cright), w - cleft - cright, h - rtop - rbottom, colors, numColors, src, srcEnd);

	free(colors);

	return success ? 0 : -1;
}
