#ifndef _NANO_JPG_H_
#define _NANO_JPG_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

enum DecodeResult {
		JpegResultOK = 0,        // decoding successful
		JpegResultNotAJpeg,      // not a JPEG file
		JpegResultUnsupported,   // unsupported format
		JpegResultOutOfMemory,   // out of memory
		JpegResultInternalError, // internal error
		JpegResultSyntaxError,   // syntax error
		JpegResultInternal_Finished, // used internally, will never be reported
	};

struct VlcEntry {
	uint16_t start;
	uint8_t bits;
	uint8_t code;
};

struct VlcTab {
	uint32_t num;	//values past this return 0

	struct VlcEntry data[];
};

struct Component {
	uint8_t *pixels;
	uint16_t width, height, stride;
	uint8_t cid;
	uint8_t ssx, ssy;
	uint8_t qtsel;
	uint8_t actabsel, dctabsel;
	int16_t dcpred;
};

struct Context {
	const uint8_t *pos;
	int32_t size, length;

	struct VlcTab *vlctab[4];
	uint8_t qtab[4][64];

	struct Component comp[3];

	uint16_t width, height;
	uint16_t mbwidth, mbheight;
	uint16_t mbsizex, mbsizey;

	uint32_t buf;
	uint8_t bufbits;
	uint8_t ncomp;
	uint8_t qtused, qtavail;
	uint16_t rstinterval;

	enum DecodeResult error;
};

struct Context* jpgDecode(const unsigned char* data, size_t size);
void jpgFree(struct Context* ctx);

enum DecodeResult jpgGetResult(const struct Context* ctx);

// all remaining functions below are only valid if GetResult() == OK.

uint_fast16_t jpgGetWidth(const struct Context* ctx);
uint_fast16_t jpgGetHeight(const struct Context* ctx);
bool jpgIsColor(const struct Context* ctx);

const uint8_t* jpgGetImageData(const struct Context* ctx, uint_fast8_t component);		// if IsColor() then [0] is 8 bit luminance, else {R, G, B}
uint32_t jpgGetImageStride(const struct Context* ctx, uint_fast8_t component);				//data may be delivered with stride



//provided externally
void* jpgExtAlloc(size_t size);
void* jpgExtRealloc(void* ptr, size_t size);
void jpgExtFree(void* ptr);


#endif
