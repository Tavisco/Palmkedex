#include "aciDecode.h"


#define LOG(...)

struct BitBuffer {
	uint8_t bitBuf;
	uint8_t numBitsHere;
	const uint8_t *src;
	const uint8_t *srcEnd;
};

static void bbInit(struct BitBuffer *bb, const uint8_t *src, const uint8_t *srcEnd)
{
	bb->numBitsHere = 0;
	bb->bitBuf = 0;
	bb->src = src;
	bb->srcEnd = srcEnd;
}

static uint_fast8_t __attribute__((always_inline)) bbRead(struct BitBuffer *bb)	//read a bit
{
	uint_fast8_t ret;

	if (!bb->numBitsHere) {

		bb->numBitsHere = 8;
		if (bb->src < bb->srcEnd)
			bb->bitBuf = *bb->src++;
		//will already be zero so no need to set it in the "else" case
	}

	bb->numBitsHere--;
	ret = bb->bitBuf & 1;
	bb->bitBuf >>= 1;

	return ret;
}

bool aciDecodeBits(uint8_t *dst, uint16_t dstExtraStride, uint16_t w, uint16_t h, const struct PixelRange *ranges, uint16_t numColors, const uint8_t *src, const uint8_t *srcEnd)
{
	uint_fast16_t prevIdx = numColors - 1, i, r, c;
	uint16_t min = 0, max = 0xffff, val = 0;
	struct BitBuffer bb;

	bbInit(&bb, src, srcEnd);

	//init state
	for (i = 0; i < 16; i++)
		val = val * 2 + bbRead(&bb);

	for (r = 0; r < h; r++, dst += dstExtraStride) {

		for (c = 0; c < w; c++) {

			uint32_t width = (uint32_t)max - min + 1, above = val - min, now = (above * 256 + 255) / width;
			uint_fast8_t idxNow;

			//find matching symbol for "now". binary search
			uint_fast16_t end = numColors, start = 0, center = 0;

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

			idxNow = (center == (unsigned)(numColors - 1)) ? prevIdx : center;
			prevIdx = idxNow;

			//emit the pixel
			*dst++ = ranges[idxNow].index;

			//adjust state
			LOG("[%4u/%4u] idx %2u applying range %02xh...%02xh to %08xh...%08xh", (unsigned)(r * w + c), (unsigned)(w * h), center,
				ranges[center].start, ranges[center].end, (unsigned)(uint32_t)min, (unsigned)(uint32_t)max);

			//calc new range (once again, gcc sucks)
			//max = min + width * ranges[center].end / 256 - 1;
			//min = min + width * ranges[center].start / 256;
			//width can only take one 17-bit value - 0x10000, and it cannot take on the value of 0x0000
			//this test doe si tbetter than gcc could
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

				//range crossed
				return false;
			}
			if (val < min || val > max) {

				//val is oob
				return false;
			}
		}
	}

	return true;
}
