#include <stdbool.h>
#include <endian.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define FLAGS_V1			0x01		//must be set for this v1 format, if clear, format differs
#define FLAGS_HAS_CLUT		0x02		//if no, assume greyscale of given depth and no actual CLUT
#define FLAGS_BOUNDED		0x04
#define FLAGS_RFU			0xf8		//if any of these are set, we do not know how to decode this, do not try

#define LOG(...)		//fprintf(stderr, __VA_ARGS__)

struct BmpHdr {
	uint8_t magic[2];
	uint32_t fileSz;
	uint16_t rfu[2];
	uint32_t dataOfst;
} __attribute__((packed));

struct DibHdr {
	uint32_t hdrSz;
	int32_t width;
	int32_t height;
	uint16_t numColorPlanes;	//must be 1
	uint16_t bpp;
	uint32_t compressionType;	//zero for none
	uint32_t dataSz;			//can be zero
	uint32_t ppmH;
	uint32_t ppmV;
	uint32_t numColorsInPalette;
	uint32_t numImportantColors;
} __attribute__((packed));

struct Pixel {
	uint8_t b, g, r;
} __attribute__((packed));

struct PixelHist {
	struct Pixel color;
	uint32_t counter;
};

struct PixelRange {
	struct Pixel color;
	uint32_t counter;
	uint16_t start;
	uint16_t end;
};

struct BitBufferR {
	uint8_t bitBuf;
	uint8_t numBitsHere;
};

struct BitBufferW {
	uint8_t *dst;
	uint8_t bitBuf;
	uint8_t numBitsHere;
};

static bool readIn(void *dstP, uint32_t len);

static uint_fast8_t bbRead(struct BitBufferR *bb)	//read a bit
{
	uint_fast8_t ret;

	if (!bb->numBitsHere) {

		bb->numBitsHere = 8;
		if (!readIn(&bb->bitBuf, 1)) {

			//this is ok
			bb->bitBuf = 0;
		}
	}

	bb->numBitsHere--;
	ret = bb->bitBuf & 1;
	bb->bitBuf >>= 1;

	return ret;
}

static void bbWrite(struct BitBufferW *bb, uint_fast8_t val)
{
	bb->bitBuf += val << bb->numBitsHere;
	if (++bb->numBitsHere == 8) {
		*bb->dst++ = bb->bitBuf;
		bb->bitBuf = 0;
		bb->numBitsHere = 0;
	}
}

static void bbFlush(struct BitBufferW *bb)
{
	while (bb->numBitsHere)
		bbWrite(bb, 0);
}

//return num bytes produced
static uint32_t compressImage(uint8_t *dst, const struct Pixel *pixels, uint32_t w, uint32_t h, uint_fast8_t numGreyBits, bool allowBorder)
{
	uint32_t numColors = 1, i, j, k, r, c, rtop, rbottom, cleft, cright, numTruncated = 0, numEncodedPixels = 0, borderColorIdx, framePix;
	struct PixelHist *hist = calloc(sizeof(struct PixelHist), w * h);
	struct PixelRange *ranges;
	struct BitBufferW bb = {.dst = dst, };
	struct Pixel prevPixel;
	uint16_t min, max;

	//calculate bounding box of the same color
	rtop = 0;
	if (!memcmp(pixels, pixels + 1, sizeof(struct Pixel) * (w - 1))) {
		while (!memcmp(pixels, pixels + rtop * w, sizeof(struct Pixel) * w))
			rtop++;
	}
	rbottom = 0;
	if (!memcmp(pixels + (h - 1) * w, pixels + (h - 1) * w + 1, sizeof(struct Pixel) * (w - 1))) {
		while (!memcmp(pixels + (h - 1) * w, pixels + (h - 1 - rbottom) * w, sizeof(struct Pixel) * w))
			rbottom++;
	}
	cleft = 0;
	while(1) {
		for (i = 0; i < h && !memcmp(pixels, pixels + i * w + cleft, sizeof(struct Pixel)); i++);
		if (i != h)
			break;
		cleft++;
	}
	cright = 0;
	while(1) {
		for (i = 0; i < h && !memcmp(pixels + w - 1, pixels + i * w + w - 1 - cright, sizeof(struct Pixel)); i++);
		if (i != h)
			break;
		cright++;
	}

	//they must all be representable as a byte
	if (rtop > 255)
		rtop = 255;
	if (rbottom > 255)
		rbottom = 255;
	if (cleft > 255)
		cleft = 255;
	if (cright > 255)
		cright = 255;
	
	//if we have disjoint areas, verify they are the same color, and if not, pick the larger to stick with
	if (rtop && rbottom && !cleft && !cright) {									//separate top and bottom
		
		if (memcmp(pixels, pixels + (h - 1) * w, sizeof(struct Pixel))) {		//colors differ
			
			if (rtop > rbottom)
				rbottom = 0;
			else
				rtop = 0;
		}
	}
	else if (cleft && cright && !rbottom && !rtop) {							//separate right and left
		
		if (memcmp(pixels, pixels + w - 1, sizeof(struct Pixel))) {		//colors differ
			
			if (cleft > cright)
				cright = 0;
			else
				cleft = 0;
		}
	}
	
	//it may not be worth it. we'll try both ways
	framePix = (rtop + rbottom) * w + (cleft + cright) * h - (rtop + rbottom) * (cleft + cright);
	if (allowBorder && (rtop || rbottom || cleft || cright)) {
		
		LOG("Border with %u pixels found and will be used: top: %u bottom %u, left %u right %u\n", framePix, rtop, rbottom, cleft, cright);
	}
	else {
		cleft = cright = rtop = rbottom = 0;
	}
	//here, 0th color means "same as last" (pseudo-RLE)

	//collect histogram
	if (numGreyBits) {	//init histogram to make sure we have order

		for (i = 1; i <= (1u << numGreyBits); i++)
			hist[i].color.r = hist[i].color.g = hist[i].color.b = i - 1;

		numColors = i;
	}

	//if we have a border, make sure its index is added to the colortable with weight 1
	if (rtop) {

		for (j = 1; j < numColors && memcmp(pixels, &hist[j].color, sizeof(struct Pixel)); j++);
		if (j == numColors) {		//new color

			if (numGreyBits) {

				fprintf(stderr, "did not expect a new color for a grey bitmap: (%u,%u,%u)!\n", pixels[i].r, pixels[i].g, pixels[i].b);
				abort();
			}

			hist[j].color = pixels[0];
			numColors++;
		}
		hist[j].counter++;
	}

	for (r = rtop; r < h - rbottom; r++) {
		for (c = cleft; c < w - cright; c++) {

			const struct Pixel *thisPixel = pixels + r * w + c;

			for (j = 1; j < numColors && memcmp(thisPixel, &hist[j].color, sizeof(struct Pixel)); j++);
			if (j == numColors) {		//new color

				if (numGreyBits) {

					fprintf(stderr, "did not expect a new color for a grey bitmap: (%u,%u,%u)!\n", pixels[i].r, pixels[i].g, pixels[i].b);
					abort();
				}

				hist[j].color = *thisPixel;
				numColors++;
			}
			else if (i && !memcmp(&prevPixel, thisPixel, sizeof(struct Pixel)))
				hist[0].counter++;
			else
				hist[j].counter++;

			prevPixel = *thisPixel;
			numEncodedPixels++;
		}
	}

	LOG("Bitmap is %ux%u and has %u colors\n", w, h, numColors);
	if (numColors >= 256)
		exit(-1);

	if (!numGreyBits) {
		//sort colors from least used to most for color modes so that the more used colors can get into the palette
		for (i = 1; i < numColors; i++) {

			struct PixelHist tmp;

			for (k = i, j = i + 1; j < numColors; j++) {

				if (hist[k].counter > hist[j].counter)
					k = j;
			}

			tmp = hist[k];
			hist[k] = hist[i];
			hist[i] = tmp;
		}
	}

	//make our range list, make RLE entry be at the end, now [numColors - 1] means "RLE color"
	//most used color is first, this will allow, on decode, to pick which color(s) to favour...
	ranges = calloc(sizeof(struct PixelRange), numColors);
	for (i = 0; i < numColors; i++) {
		ranges[numColors - i - 1].color = hist[i].color;
		ranges[numColors - i - 1].counter = hist[i].counter;
	}
	free(hist);
	hist = NULL;

	//allocate bit ranges
	k = 0;
	for (i = 0; i < numColors - 1; i++) {

		uint32_t len;

		ranges[i].start = k;
		len = ranges[i].counter * 256 / numEncodedPixels;
		if (!len)
			len++;
		ranges[i].end = ranges[i].start + len;
		k += len;
	}
	ranges[i].start = k;
	ranges[i].end = 256;

	for (i = 0; i < numColors - 1; i++) {

		LOG("color [%3u] (%3u, %3u, %3u) is used by %4u pixels and gets range %02xh...%02xh\n", i,
			ranges[i].color.r, ranges[i].color.g, ranges[i].color.b, ranges[i].counter, ranges[i].start, ranges[i].end);
	}
	LOG("RLE entry is used by %4u pixels and gets range %02xh...%02xh\n",
			ranges[i].counter, ranges[i].start, ranges[i].end);

	//emit W & H
	*bb.dst++ = w >> 8;
	*bb.dst++ = w;
	*bb.dst++ = h >> 8;
	*bb.dst++ = h;

	//flags/status byte
	*bb.dst++ = FLAGS_V1 | (numGreyBits ? 0 : FLAGS_HAS_CLUT) | (rtop ? FLAGS_BOUNDED : 0);

	//emit num colors
	*bb.dst++ = numColors - 2;	//do not count RLE color, and offset by one to allow for 256 colors

	if (rtop) {
		*bb.dst++ = rtop;
		*bb.dst++ = rbottom;
		*bb.dst++ = cleft;
		*bb.dst++ = cright;

		//find index of border
		for (j = 0; j < numColors - 1 && memcmp(pixels, &ranges[j].color, sizeof(struct Pixel)); j++);
		*bb.dst++ = j;
	}

	if (!numGreyBits) {
		//emit clut
		for (i = 0; i < numColors - 1; i++) {

			*bb.dst++ = ranges[i].color.r;
			*bb.dst++ = ranges[i].color.g;
			*bb.dst++ = ranges[i].color.b;
		}
	}

	//emit "start" of each value (except first for which we know it)
	for (i = 1; i < numColors; i++)
		*bb.dst++ = ranges[i].start;

	//emit pixels
	min = 0;
	max = 0xffff;
	k = numColors - 1;	//previndex, this is guaranteed to not match 0th pixel


	for (r = rtop; r < h - rbottom; r++) {
		for (c = cleft; c < w - cright; c++) {

			const struct Pixel *thisPixel = pixels + r * w + c;
			uint32_t width = (uint32_t)max - min + 1;

			//find the index
			for (j = 0; j < numColors - 1 && memcmp(thisPixel, &ranges[j].color, sizeof(struct Pixel)); j++);

			//see if it matches prev and if so, apply that special marker
			if (j == k)
				j = numColors - 1;
			else
				k = j;

			LOG("[%4u/%4u] idx %2u applying range %02xh...%02xh to %08xh...%08xh", i, totalPixels, j,
				ranges[j].start, ranges[j].end, (unsigned)(uint32_t)min, (unsigned)(uint32_t)max);

			//calc new range
			max = min + width * ranges[j].end / 256 - 1;
			min = min + width * ranges[j].start / 256;

			LOG(" produces a range of %08xh...%08xh\n", (unsigned)(uint32_t)min, (unsigned)(uint32_t)max);

			while ((min >> 15) == (max >> 15)) {

				uint_fast8_t bit = min >> 15;

				bbWrite(&bb, bit);
				LOG(" emitting %u\n", bit);
				bit = 1 - bit;
				while (numTruncated) {
					bbWrite(&bb, bit);
					LOG(" emitting %u (from truncation)\n", bit);
					numTruncated--;
				}


				min = min * 2;
				max = max * 2 + 1;
			}

			while ((min >> 14) == 1 && (max >> 14) == 2) {

				LOG(" truncating\n");
				numTruncated++;
				min = (min << 1) ^ 0x8000;
				max = (max << 1) ^ 0x8001;
			}
		}
	}

	//the end
	bbWrite(&bb, 1);
	LOG(" emitting %u\n", 1);

	//emit all the remaining bits we need to encode the range
	bbFlush(&bb);

	free(ranges);
	
	return bb.dst - dst;
}

static bool readIn(void *dstP, uint32_t len)
{
	uint8_t *dst = dstP;
	int c;

	while (len--) {
		if ((c = getchar()) == EOF)
			return false;

		if (dst)
			*dst++ = c;
	}

	return true;
}

static void emitColor(const struct PixelRange *rangeEntry)
{
	putchar(rangeEntry->color.b);
	putchar(rangeEntry->color.g);
	putchar(rangeEntry->color.r);
}

static void emitFullRow(const struct PixelRange *rangeEntry, uint32_t width, uint32_t rowExtraBytes)
{
	uint32_t c;

	for (c = 0; c < width; c++)
		emitColor(rangeEntry);
	while (rowExtraBytes--)
		putchar(0);
}

static void decompressImage(uint_fast16_t w, uint_fast16_t h, uint_fast16_t rowExtraBytes, const struct PixelRange *ranges, uint_fast16_t numColors, uint8_t rtop, uint8_t rbottom, uint8_t cleft, uint8_t cright, uint8_t borderColorIdx)
{
	uint32_t i, r, c;
	uint16_t min = 0, max = 0xffff, val = 0;
	uint_fast16_t prevIdx = numColors - 1;
	struct BitBufferR bb = {};

	//init state
	for (i = 0; i < sizeof(val) * 8; i++)
		val = val * 2 + bbRead(&bb);


	for (r = 0; r < rtop; r++)
		emitFullRow(ranges + borderColorIdx, w, rowExtraBytes);

	for (; r < h - rbottom; r++) {

		for (c = 0; c < cleft; c++)
			emitColor(ranges + borderColorIdx);

		for (; c < w - cright; c++) {

			uint32_t width = (uint32_t)max - min + 1;
			uint32_t above = val - min;
			uint32_t now = ((above + 1) * 256 - 1) / width;
			uint_fast8_t idxNow;

			//find matching symbol for "now". binary search
			uint_fast16_t end = numColors, start = 0, center;

			LOG(">> val 0x%08xh, above 0x%08xh, looking for %08xh\n", val, above, now);

			while (end > start) {

				center = (end + start) / 2;

				if (ranges[center].end <= now)
					start = center;
				else if (ranges[center].start > now)
					end = center;
				else
					break;
			}

			idxNow = (center == numColors - 1) ? prevIdx : center;
			prevIdx = idxNow;

			//emit the pixel
			emitColor(ranges + idxNow);

			//adjust state
			LOG("[%4u/%4u] idx %2u applying range %02xh...%02xh to %08xh...%08xh", (unsigned)(r * w + c), (unsigned)(w * h), center,
				ranges[center].start, ranges[center].end, (unsigned)(uint32_t)min, (unsigned)(uint32_t)max);

			//calc new range
			max = min + width * ranges[center].end / 256 - 1;
			min = min + width * ranges[center].start / 256;

			LOG(" produces a range of %08xh...%08xh\n", (unsigned)(uint32_t)min, (unsigned)(uint32_t)max);

			while ((min >> 15) == (max >> 15)) {

				min = min * 2;
				max = max * 2 + 1;

				val = val * 2 + bbRead(&bb);
				LOG(" reading %u -> %08xh\n", val & 1, (unsigned)val);
				LOG("  %08xh...%08xh\n", (unsigned)(uint32_t)min, (unsigned)(uint32_t)max);
			}

			while ((min >> 14) == 1 && (max >> 14) == 2) {

				min = (min << 1) ^ 0x8000;
				max = (max << 1) ^ 0x8001;

				val = (val & 0x8000) + (val & 0x3fff) * 2 + bbRead(&bb);
				LOG(" truncate-reading %u-> %08xh\n", val & 1, (unsigned)val);
				LOG("  %08xh...%08xh\n", (unsigned)(uint32_t)min, (unsigned)(uint32_t)max);
			}

			if (min > max) {

				fprintf(stderr, "range crossed\n");
				abort();
			}
			if (val < min || val > max) {

				fprintf(stderr, "val OOB: %08xh\n", val);
				abort();
			}
		}
		for (c = 0; c < cright; c++)
			emitColor(ranges + borderColorIdx);

		for (c = 0; c < rowExtraBytes; c++)
			putchar(0);
	}
	for (r = 0; r < rbottom; r++)
		emitFullRow(ranges + borderColorIdx, w, rowExtraBytes);
}

static void usage(const char *self)
{
	fprintf(stderr,
		"USAGE:\n"
		"\tcompress: %s c[n] < in.bmp > out.bin\n"
		"\t\tBitmap must be a 24 bits per pixel uncompressed bitmap under 65535 x 65535.\n"
		"\t\tn, if present, must be 4, 2, or 1 to signify bits of grey to encode\n"
		"\tdecompress: %s d < in.bin > out.bmp\n",
		self, self);

	exit(-1);
}

int main(int argc, char **argv)
{
	uint32_t i, r, c, rowBytes, rowExtraBytes;
	struct BmpHdr hdrBmp = {};
	struct DibHdr hdrDib = {};

	if (argc != 2 || (argv[1][0] != 'c' && argv[1][0] != 'd'))
		usage(argv[0]);

	if (argv[1][0] == 'c') {	//compress

		uint32_t compressedSzF, compressedSzNF, compressedSzBest;
		uint_fast8_t numGreyBits = 0;
		bool rightSideUp = false;
		struct Pixel *pixels;
		int32_t w, h;
		uint8_t *dst;
		
		switch (argv[1][1]) {
			case 0:
				break;

			case '1':
			case '2':
			case '4':
				numGreyBits = argv[1][1] - '0';
				if (!(argv[1][2]))
					break;
				//fallthrough

			default:
				usage(argv[0]);
				break;
		}

		if (!readIn(&hdrBmp, sizeof(hdrBmp)) ||
				!readIn(&hdrDib, sizeof(hdrDib)) ||
				hdrBmp.magic[0] != 'B' ||
				hdrBmp.magic[1] != 'M' ||
				le32toh(hdrBmp.dataOfst) < sizeof(struct BmpHdr) + sizeof(struct DibHdr) ||
				le32toh(hdrDib.hdrSz) < sizeof(struct DibHdr) ||
				le16toh(hdrDib.numColorPlanes) != 1 ||
				le16toh(hdrDib.bpp) != 24 ||
				le32toh(hdrDib.compressionType) ||
				!(w = le32toh(hdrDib.width)) ||
				!(h = le32toh(hdrDib.height)) ||
				abs(w) >> 16 ||
				abs(h) >> 16 ||
				!readIn(NULL, le32toh(hdrBmp.dataOfst) - sizeof(struct BmpHdr) - sizeof(struct DibHdr)))
			usage(argv[0]);

		if (h < 0) {
			h = -h;
			rightSideUp = true;
		}

		rowBytes = w * 3 + 3 / 4 * 4;
		rowExtraBytes = rowBytes - w * 3;
	
		dst = malloc(w * h * 8);//big enough
		
		pixels = malloc(sizeof(struct Pixel) * w * h);
		for (r = 0; r < (uint32_t)h; r++) {

			for (c = 0; c < (uint32_t)w; c++) {

				struct Pixel *dstPixel = &pixels[c + w * (rightSideUp ? r : h - r - 1)];

				if (!readIn(dstPixel, sizeof(struct Pixel))) {		//we always encode right side up

					fprintf(stderr, "cannot read pixel data for row %u col %u\n", r, c);
					return -1;
				}

				if (numGreyBits) {

					static const uint32_t divBy[] = {[1] = 0xff0000, [2] = 0x550000, [4] = 0x110000};
					uint32_t r = dstPixel->r;
					uint32_t g = dstPixel->g;
					uint32_t b = dstPixel->b;
					uint32_t greyLevel;

					r *= 19595;
					g *= 38470;
					b *= 7471;

					greyLevel = r + g + b; //0..FF0000

					greyLevel = (greyLevel + divBy[numGreyBits] / 2) / divBy[numGreyBits];		//this is black-to-white, we need to reverse it
					greyLevel = (1 << numGreyBits) - 1 - greyLevel;

					dstPixel->r = greyLevel;
					dstPixel->g = greyLevel;
					dstPixel->b = greyLevel;
				}
			}
			if (!readIn(NULL, rowExtraBytes)) {

				fprintf(stderr, "cannot read dummy data for row  %u\n", r);
				return -1;
			}
		}
		
		//try with and without border for smaller size
		compressedSzNF = compressImage(dst, pixels, w, h, numGreyBits, false);
		LOG(" NF: compressed to %u bytes\n", compressedSzNF);
		compressedSzF = compressImage(dst, pixels, w, h, numGreyBits, true);
		LOG(" F:  compressed to %u bytes\n", compressedSzF);
		if (compressedSzNF < compressedSzF) {
			
			LOG(" NF wins\n");
			compressedSzBest = compressImage(dst, pixels, w, h, numGreyBits, false);
		}
		else {
			
			LOG(" F wins\n");
			compressedSzBest = compressedSzF;
		}
		if (compressedSzBest != fwrite(dst, 1, compressedSzBest, stdout)) {
			
			fprintf(stderr, "write failure\n");
			abort();
		}
		free(dst);
		free(pixels);
	}
	else if (argv[1][1])
		usage(argv[0]);
	else {	//decompress

		struct PixelRange *ranges;
		uint16_t w, h, numColors;
		uint8_t numcolorsM2, flags, rtop = 0, rbottom = 0, cleft = 0, cright = 0, borderColorIdx = 0;

		//the basics
		if (!readIn(&w, sizeof(w)) || !readIn(&h, sizeof(h)) || !readIn(&flags, sizeof(flags)) || !readIn(&numcolorsM2, sizeof(numcolorsM2)))
			usage(argv[1]);
		w = be16toh(w);
		h = be16toh(h);
		numColors = (uint16_t)numcolorsM2 + 2;
		if (!(flags & FLAGS_V1)) {
			fprintf(stderr, "unknown version\n");
			exit(-2);
		}
		if (flags & FLAGS_RFU) {
			fprintf(stderr, "unknown features\n");
			exit(-2);
		}
		
		if (flags & FLAGS_BOUNDED) {
	
			if (!readIn(&rtop, sizeof(rtop)) || !readIn(&rbottom, sizeof(rbottom)) || !readIn(&cleft, sizeof(cleft)) || !readIn(&cright, sizeof(cright)) || !readIn(&borderColorIdx, sizeof(borderColorIdx)))
				usage(argv[1]);
		}

		ranges = calloc(sizeof(struct PixelRange), numColors);

		//read & decode the clut, if it exists (last entry is the RLE entry and has no color)
		if (flags & FLAGS_HAS_CLUT) {
			for (i = 0; i < (uint16_t)(numColors - 1); i++) {

				if (!readIn(&ranges[i].color.r, 1) || !readIn(&ranges[i].color.g, 1) || !readIn(&ranges[i].color.b, 1))
					usage(argv[1]);
			}
		}
		else {

			//create a white-to-black clut
			for (i = 0; i < (uint16_t)(numColors - 1); i++) {

				ranges[i].color.r = ranges[i].color.g = ranges[i].color.b = 255 - (255 * i + (numColors - 2) / 2) / (numColors - 2);
			}
		}

		//read and calculate the ranges
		for (i = 1; i < numColors; i++) {

			uint8_t tmp;

			if (!readIn(&tmp, 1))
				usage(argv[1]);
			ranges[i].start = tmp;
			ranges[i - 1].end = ranges[i].start;
		}
		ranges[numColors - 1].end = 256;

		for (i = 0; i < (uint16_t)(numColors - 1); i++) {

			LOG("color (%3u, %3u, %3u) gets range %02xh...%02xh\n",
				ranges[i].color.r, ranges[i].color.g, ranges[i].color.b, ranges[i].start, ranges[i].end);
		}
		LOG("RLE entry gets range %02xh...%02xh\n", ranges[i].start, ranges[i].end);

		//emit bitmap and DIB headers
		rowBytes = w * 3 + 3 / 4 * 4;
		rowExtraBytes = rowBytes - w * 3;

		hdrBmp.magic[0] = 'B';
		hdrBmp.magic[1] = 'M';
		hdrBmp.fileSz = htole32(sizeof(struct BmpHdr) + sizeof(struct DibHdr) + rowBytes * h);
		hdrBmp.dataOfst = htole32(sizeof(struct BmpHdr) + sizeof(struct DibHdr));
		if (fwrite(&hdrBmp, 1, sizeof(struct BmpHdr), stdout) != sizeof(struct BmpHdr))
			abort();

		hdrDib.hdrSz = htole32(sizeof(struct DibHdr));
		hdrDib.width = htole32(w);
		hdrDib.height = htole32(-h);
		hdrDib.numColorPlanes = htole16(1);
		hdrDib.bpp = htole16(24);
		hdrDib.compressionType = htole32(0);
		hdrDib.dataSz = htole32(rowBytes * h);
		hdrDib.ppmH = htole32(2834);
		hdrDib.ppmV = htole32(2834);
		hdrDib.numColorsInPalette = htole32(0);
		hdrDib.numImportantColors = htole32(0);
		if (fwrite(&hdrDib, 1, sizeof(struct DibHdr), stdout) != sizeof(struct DibHdr))
			abort();

		decompressImage(w, h, rowExtraBytes, ranges, numColors, rtop, rbottom, cleft, cright, borderColorIdx);

		free(ranges);
	}


	return 0;
}
