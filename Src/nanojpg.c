//This is a rejiggering of NanoJPG from the C++ mess that it was to pure C,
// with cleanup for non-32-bit-integer platforms and simplification. Done by
// DmitryGR
//  Original license follows:
// This is a cosmetic restructing and port to C++ class of 'NanoJPEG', found
// at http://keyj.s2000.ws/?p=137. It's been made somewhat thread safe in that
// all context information is pulled into an object, rather than being global
// as the original was. Other than that, the original is superior in
// configurability, comments, cleanliness, portability, etc. and should be
// preferred. The only other possible benefit this version can claim is that
// it's crammed into one header file.
//
// Scott Graham <scott.jpegdecoder@h4ck3r.net>
//
// The original license follows:
//
// NanoJPEG -- KeyJ's Tiny Baseline JPEG Decoder
// version 1.0 (2009-04-29)
// by Martin J. Fiedler <martin.fiedler@gmx.net>
//
// This software is published under the terms of KeyJ's Research License,
// version 0.2. Usage of this software is subject to the following conditions:
// 0. There's no warranty whatsoever. The author(s) of this software can not
//	be held liable for any damages that occur when using this software.
// 1. This software may be used freely for both non-commercial and commercial
//	purposes.
// 2. This software may be redistributed freely as long as no fees are charged
//	for the distribution and this license information is included.
// 3. This software may be modified freely except for this license information,
//	which must not be changed in any way.
// 4. If anything other than configuration, indentation or comments have been
//	altered in the code, the original author(s) must receive a copy of the
//	modified code.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nanojpg.h"

#define JPEG_DECODER_THROW(e) do { ctx->error = e; return; } while (0)


//DCT constants
#define W1		2841
#define W2		2676
#define W3		2408
#define W5		1609
#define W6		1108
#define W7		565

//upscale constants
#define CF4A	(-9)
#define CF4B	(111)
#define CF4C	(29)
#define CF4D	(-3)
#define CF3A	(28)
#define CF3B	(109)
#define CF3C	(-9)
#define CF3X	(104)
#define CF3Y	(27)
#define CF3Z	(-3)
#define CF2A	(139)
#define CF2B	(-11)

static uint_fast8_t _Clip(int32_t x) {
	return (x < 0) ? 0 : ((x > 0xFF) ? 0xFF : (uint8_t) x);
}

static uint_fast8_t CF(int32_t x) {
	return _Clip((x + 64) >> 7);
}


static void _RowIDCT(int32_t* blk) {
	int32_t x0, x1, x2, x3, x4, x5, x6, x7, x8;
	if (!((x1 = blk[4] << 11)
		| (x2 = blk[6])
		| (x3 = blk[2])
		| (x4 = blk[1])
		| (x5 = blk[7])
		| (x6 = blk[5])
		| (x7 = blk[3])))
	{
		blk[0] = blk[1] = blk[2] = blk[3] = blk[4] = blk[5] = blk[6] = blk[7] = blk[0] << 3;
		return;
	}
	x0 = (blk[0] << 11) + 128;
	x8 = W7 * (x4 + x5);
	x4 = x8 + (W1 - W7) * x4;
	x5 = x8 - (W1 + W7) * x5;
	x8 = W3 * (x6 + x7);
	x6 = x8 - (W3 - W5) * x6;
	x7 = x8 - (W3 + W5) * x7;
	x8 = x0 + x1;
	x0 -= x1;
	x1 = W6 * (x3 + x2);
	x2 = x1 - (W2 + W6) * x2;
	x3 = x1 + (W2 - W6) * x3;
	x1 = x4 + x6;
	x4 -= x6;
	x6 = x5 + x7;
	x5 -= x7;
	x7 = x8 + x3;
	x8 -= x3;
	x3 = x0 + x2;
	x0 -= x2;
	x2 = (181 * (x4 + x5) + 128) >> 8;
	x4 = (181 * (x4 - x5) + 128) >> 8;
	blk[0] = (x7 + x1) >> 8;
	blk[1] = (x3 + x2) >> 8;
	blk[2] = (x0 + x4) >> 8;
	blk[3] = (x8 + x6) >> 8;
	blk[4] = (x8 - x6) >> 8;
	blk[5] = (x0 - x4) >> 8;
	blk[6] = (x3 - x2) >> 8;
	blk[7] = (x7 - x1) >> 8;
}

static void _ColIDCT(const int32_t* blk, uint8_t *out, uint16_t stride) {
	int32_t x0, x1, x2, x3, x4, x5, x6, x7, x8;
	if (!((x1 = blk[8*4] << 8)
		| (x2 = blk[8*6])
		| (x3 = blk[8*2])
		| (x4 = blk[8*1])
		| (x5 = blk[8*7])
		| (x6 = blk[8*5])
		| (x7 = blk[8*3])))
	{
		x1 = _Clip(((blk[0] + 32) >> 6) + 128);
		for (x0 = 8;  x0;  --x0) {
			*out = (uint8_t) x1;
			out += stride;
		}
		return;
	}
	x0 = (blk[0] << 8) + 8192;
	x8 = W7 * (x4 + x5) + 4;
	x4 = (x8 + (W1 - W7) * x4) >> 3;
	x5 = (x8 - (W1 + W7) * x5) >> 3;
	x8 = W3 * (x6 + x7) + 4;
	x6 = (x8 - (W3 - W5) * x6) >> 3;
	x7 = (x8 - (W3 + W5) * x7) >> 3;
	x8 = x0 + x1;
	x0 -= x1;
	x1 = W6 * (x3 + x2) + 4;
	x2 = (x1 - (W2 + W6) * x2) >> 3;
	x3 = (x1 + (W2 - W6) * x3) >> 3;
	x1 = x4 + x6;
	x4 -= x6;
	x6 = x5 + x7;
	x5 -= x7;
	x7 = x8 + x3;
	x8 -= x3;
	x3 = x0 + x2;
	x0 -= x2;
	x2 = (181 * (x4 + x5) + 128) >> 8;
	x4 = (181 * (x4 - x5) + 128) >> 8;
	*out = _Clip(((x7 + x1) >> 14) + 128);  out += stride;
	*out = _Clip(((x3 + x2) >> 14) + 128);  out += stride;
	*out = _Clip(((x0 + x4) >> 14) + 128);  out += stride;
	*out = _Clip(((x8 + x6) >> 14) + 128);  out += stride;
	*out = _Clip(((x8 - x6) >> 14) + 128);  out += stride;
	*out = _Clip(((x0 - x4) >> 14) + 128);  out += stride;
	*out = _Clip(((x3 - x2) >> 14) + 128);  out += stride;
	*out = _Clip(((x7 - x1) >> 14) + 128);
}


static uint32_t _PeekBits(struct Context* ctx, uint_fast8_t bits) {
	uint_fast8_t newbyte;

	if (!bits) return 0;
	while (ctx->bufbits < bits) {
		if (ctx->size <= 0) {
			ctx->buf = (ctx->buf << 8) | 0xFF;
			ctx->bufbits += 8;
			continue;
		}
		newbyte = *ctx->pos++;
		ctx->size--;
		ctx->bufbits += 8;
		ctx->buf = (ctx->buf << 8) | newbyte;
		if (newbyte == 0xFF) {
			if (ctx->size) {
				uint_fast8_t marker = *ctx->pos++;
				ctx->size--;
				switch (marker) {
					case 0:	break;
					case 0xD9: ctx->size = 0; break;
					default:
						if ((marker & 0xF8) != 0xD0)
							ctx->error = JpegResultSyntaxError;
						else {
							ctx->buf = (ctx->buf << 8) | marker;
							ctx->bufbits += 8;
						}
				}
			} else
				ctx->error = JpegResultSyntaxError;
		}
	}
	return (ctx->buf >> (ctx->bufbits - bits)) & ((1 << bits) - 1);
}

static void _SkipBits(struct Context* ctx, uint_fast8_t bits) {
	if (ctx->bufbits < bits)
		(void) _PeekBits(ctx, bits);
	ctx->bufbits -= bits;
}

static uint32_t _GetBits(struct Context* ctx, uint_fast8_t bits) {
	uint32_t res = _PeekBits(ctx, bits);
	_SkipBits(ctx, bits);
	return res;
}

static void _ByteAlign(struct Context* ctx) {
	ctx->bufbits &= 0xF8;
}

static void _Skip(struct Context* ctx, uint32_t count) {
	ctx->pos += count;
	ctx->size -= count;
	ctx->length -= count;
	if (ctx->size < 0) ctx->error = JpegResultSyntaxError;
}

static uint_fast16_t _Decode16(struct Context* ctx, const uint8_t *pos) {
	return (((uint_fast16_t)pos[0]) << 8) | pos[1];
}

static void _DecodeLength(struct Context* ctx) {
	if (ctx->size < 2) JPEG_DECODER_THROW(JpegResultSyntaxError);
	ctx->length = _Decode16(ctx, ctx->pos);
	if (ctx->length > ctx->size) JPEG_DECODER_THROW(JpegResultSyntaxError);
	_Skip(ctx, 2);
}

static void _SkipMarker(struct Context* ctx) {
	_DecodeLength(ctx);
	_Skip(ctx, ctx->length);
}

static void _DecodeSOF(struct Context* ctx) {
	static const int8_t valToShift[] = {-1, 0, 1, -1, 2, -1, -1, -1, 3};
	uint_fast8_t i, ssxmaxShift = 0, ssymaxShift = 0, mbsizexShift, mbsizeyShift;
	struct Component* c;

	_DecodeLength(ctx);
	if (ctx->length < 9) JPEG_DECODER_THROW(JpegResultSyntaxError);
	if (ctx->pos[0] != 8) JPEG_DECODER_THROW(JpegResultUnsupported);
	ctx->height = _Decode16(ctx, ctx->pos+1);
	ctx->width = _Decode16(ctx, ctx->pos+3);
	ctx->ncomp = ctx->pos[5];
	_Skip(ctx, 6);
	switch (ctx->ncomp) {
		case 1:
		case 3:
			break;
		default:
			JPEG_DECODER_THROW(JpegResultUnsupported);
	}
	if (ctx->length < (ctx->ncomp * 3)) JPEG_DECODER_THROW(JpegResultSyntaxError);
	for (i = 0, c = ctx->comp;  i < ctx->ncomp;  ++i, ++c) {

		uint_fast8_t ssx = ctx->pos[1] >> 4;	//must be a nonzero power of two
		uint_fast8_t ssy = ctx->pos[1] & 15;

		c->cid = ctx->pos[0];

		if (!(c->ssx = ctx->pos[1] >> 4)) JPEG_DECODER_THROW(JpegResultSyntaxError);
		if (c->ssx & (c->ssx - 1)) JPEG_DECODER_THROW(JpegResultUnsupported);  // non-power of two
		if (!(c->ssy = ctx->pos[1] & 15)) JPEG_DECODER_THROW(JpegResultSyntaxError);
		if (c->ssy & (c->ssy - 1)) JPEG_DECODER_THROW(JpegResultUnsupported);  // non-power of two

		c->ssx = ssx;
		c->ssy = ssy;

		//convert to shift (guaranteed to work)
		ssx = valToShift[ssx];
		ssy = valToShift[ssy];

		if ((c->qtsel = ctx->pos[2]) & 0xFC) JPEG_DECODER_THROW(JpegResultSyntaxError);
		_Skip(ctx, 3);
		ctx->qtused |= 1 << c->qtsel;
		if (ssx > ssxmaxShift) ssxmaxShift = ssx;
		if (ssy > ssymaxShift) ssymaxShift = ssy;
	}
	mbsizexShift = ssxmaxShift + 3;
	mbsizeyShift = ssymaxShift + 3;
	ctx->mbsizex = 1 << mbsizexShift;
	ctx->mbsizey = 1 << mbsizexShift;
	ctx->mbwidth = (ctx->width + ctx->mbsizex - 1) >> mbsizexShift;
	ctx->mbheight = (ctx->height + ctx->mbsizey - 1) >>mbsizeyShift;
	for (i = 0, c = ctx->comp;  i < ctx->ncomp;  ++i, ++c) {
		c->width = (ctx->width * c->ssx + (1 << ssxmaxShift) - 1) >> ssxmaxShift;
		c->stride = (c->width + 7) & 0x7FFFFFF8;
		c->height = (ctx->height * c->ssy + (1 << ssymaxShift) - 1) >> ssymaxShift;
		c->stride = (ctx->mbwidth * ctx->mbsizex * c->ssx) >> ssxmaxShift;
		if (((c->width < 3) && (c->ssx != (1 << ssxmaxShift))) || ((c->height < 3) && (c->ssy != (1 << ssymaxShift)))) JPEG_DECODER_THROW(JpegResultUnsupported);
		if (!(c->pixels = (uint8_t*)jpgExtAlloc(c->stride * ((ctx->mbheight * ctx->mbsizey * c->ssy) >> ssymaxShift)))) JPEG_DECODER_THROW(JpegResultOutOfMemory);
	}

	_Skip(ctx, ctx->length);
}

static void vlcFree(struct VlcTab *vlc)
{
	jpgExtFree(vlc);
}

static struct VlcEntry *vlcPrvFind(struct VlcTab *vlc, uint_fast16_t index)
{
	uint_fast16_t firstPossible = 0;
	uint_fast16_t lastPossible = vlc->num - 1;

	while (firstPossible <= lastPossible) {	//binary search since our items are in order

		uint_fast16_t checkIdx = (firstPossible + lastPossible) / 2;
		struct VlcEntry *e = &vlc->data[checkIdx];
		uint_fast16_t runLen = (checkIdx == vlc->num - 1 ? 65536 : vlc->data[checkIdx + 1].start) - e->start;

		if (e->start > index) {	//current is too high, check below

			lastPossible = checkIdx - 1;
			continue;
		}

		if (index - e->start < runLen) {	//this is the one

			return e;
		}

		//current is too low - check above
		firstPossible = checkIdx + 1;
	}

	return NULL;
}

static uint_fast8_t vlcGetBits(const struct VlcTab *vlc, uint_fast16_t index)
{
	struct VlcEntry *e = vlcPrvFind((struct VlcTab*)vlc, index);

	return e ? e->bits : 0;
}

static uint_fast8_t vlcGetCode(const struct VlcTab *vlc, uint_fast16_t index)
{
	struct VlcEntry *e = vlcPrvFind((struct VlcTab*)vlc, index);

	return e ? e->code : 0;
}

//assumes:
// - no double-set of same index
// - set in order with no holes
static bool vlcSetItem(struct VlcTab **vlcP, uint_fast16_t index, uint_fast8_t bits, uint_fast8_t code)
{
	struct VlcTab *vlc = *vlcP;
	struct VlcEntry *e;

	if (!bits)
		return true;

	if (index){
		//since we set in order, we are at most amending the last entry, just check that
		e = &vlc->data[vlc->num - 1];

		if (e->code == code && e->bits == bits) {
			return true;
		}
	}


	if (!vlc) {
		*vlcP = vlc = jpgExtAlloc(sizeof(struct VlcTab) + sizeof(vlc->data[0]));

		if (!vlc)
			return false;
		vlc->num = 0;
	}
	else {

		*vlcP = vlc = jpgExtRealloc(*vlcP, sizeof(struct VlcTab) + (1 + vlc->num) * sizeof(vlc->data[0]));
		if (!vlc)
			return false;
	}

	e = &vlc->data[vlc->num++];
	e->start = index;
	e->code = code;
	e->bits = bits;

	return true;
}

static void _DecodeDHT(struct Context* ctx) {

	uint_fast16_t j, remain, spread, curpos;
	uint_fast8_t i, currcnt, codelen;
	uint8_t counts[16];
	struct VlcTab **vlcP;

	_DecodeLength(ctx);
	while (ctx->length >= 17) {
		i = ctx->pos[0];
		if (i & 0xEC) JPEG_DECODER_THROW(JpegResultSyntaxError);
		if (i & 0x02) JPEG_DECODER_THROW(JpegResultUnsupported);
		i = (i | (i >> 3)) & 3;  // combined DC/AC + tableid value
		for (codelen = 0;  codelen < 16;  ++codelen)
			counts[codelen] = ctx->pos[codelen + 1];
		_Skip(ctx, 17);
		vlcP = &ctx->vlctab[i];
		curpos = 0;
		remain = spread = 65536;
		for (codelen = 0;  codelen < 16;  ++codelen) {

			spread >>= 1;
			currcnt = counts[codelen];
			if (!currcnt) continue;
			if (ctx->length < (int32_t)(uint32_t)currcnt) JPEG_DECODER_THROW(JpegResultSyntaxError);
			if (remain < (uint32_t)(currcnt << (15 - codelen))) JPEG_DECODER_THROW(JpegResultSyntaxError);
			remain -= currcnt << (15 - codelen);

			for (i = 0;  i < currcnt;  ++i) {
				uint_fast8_t code = ctx->pos[i];

				for (j = spread;  j;  --j) {
					if (!vlcSetItem(vlcP, curpos, codelen + 1, code))
						JPEG_DECODER_THROW(JpegResultOutOfMemory);
					curpos++;
				}
			}
			_Skip(ctx, currcnt);
		}

		//if curpos != 65536, we must set the lst item with bits = 0 to make sure that for all out of bounds values we return that
		if (curpos != 65536)
			vlcSetItem(vlcP, curpos, 0, 0);
	}
	if (ctx->length) JPEG_DECODER_THROW(JpegResultSyntaxError);
}

static void _DecodeDQT(struct Context* ctx) {
	uint_fast8_t i;
	uint8_t *t;

	_DecodeLength(ctx);
	while (ctx->length >= 65) {
		i = ctx->pos[0];
		if (i & 0xFC) JPEG_DECODER_THROW(JpegResultSyntaxError);
		ctx->qtavail |= 1 << i;
		t = &ctx->qtab[i][0];
		for (i = 0;  i < 64;  ++i)
			t[i] = ctx->pos[i + 1];
		_Skip(ctx, 65);
	}
	if (ctx->length) JPEG_DECODER_THROW(JpegResultSyntaxError);
}

static void _DecodeDRI(struct Context* ctx) {
	_DecodeLength(ctx);
	if (ctx->length < 2) JPEG_DECODER_THROW(JpegResultSyntaxError);
	ctx->rstinterval = _Decode16(ctx, ctx->pos);
	_Skip(ctx, ctx->length);
}

static uint_fast16_t _GetVLC(struct Context* ctx, const struct VlcTab* vlc, uint_fast8_t* codeP) {
	uint_fast16_t value = _PeekBits(ctx, 16);
	uint_fast8_t bits = vlcGetBits(vlc, value), code;
	if (!bits) { ctx->error = JpegResultSyntaxError; return 0; }
	_SkipBits(ctx, bits);
	code = vlcGetCode(vlc, value);
	if (codeP) *codeP = code;
	bits = code & 15;
	if (!bits) return 0;
	value = _GetBits(ctx, bits);
	if (value < (1u << (bits - 1)))
		value += ((uint32_t)(-1) << bits) + 1;
	return value;
}

static void _DecodeBlock(struct Context* ctx, struct Component* c, uint8_t* out) {
	static const char ZZ[64] = {
		0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18,
		11, 4, 5, 12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28, 35,
		42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45,
		38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
	};

	uint8_t coef = 0;
	int32_t block[64] = {0,};

	c->dcpred += _GetVLC(ctx, ctx->vlctab[c->dctabsel], NULL);
	block[0] = (c->dcpred) * ctx->qtab[c->qtsel][0];
	do {
		uint_fast8_t code;
		uint_fast16_t value = _GetVLC(ctx, ctx->vlctab[c->actabsel], &code);

		if (!code) break;  // EOB
		if (!(code & 0x0F) && (code != 0xF0)) JPEG_DECODER_THROW(JpegResultSyntaxError);
		coef += (code >> 4) + 1;
		if (coef > 63) JPEG_DECODER_THROW(JpegResultSyntaxError);
		block[(int) ZZ[coef]] = value * ctx->qtab[c->qtsel][coef];
	} while (coef < 63);
	for (coef = 0;  coef < 64;  coef += 8)
		_RowIDCT(&block[coef]);
	for (coef = 0;  coef < 8;  ++coef)
		_ColIDCT(&block[coef], &out[coef], c->stride);
}

static void _DecodeScan(struct Context* ctx) {
	uint_fast8_t i, sbx, sby, nextrst = 0;
	uint_fast16_t mbx, mby, rstcount = ctx->rstinterval;
	struct Component* c;

	_DecodeLength(ctx);
	if (ctx->length < (4 + 2 * ctx->ncomp)) JPEG_DECODER_THROW(JpegResultSyntaxError);
	if (ctx->pos[0] != ctx->ncomp) JPEG_DECODER_THROW(JpegResultUnsupported);
	_Skip(ctx, 1);
	for (i = 0, c = ctx->comp;  i < ctx->ncomp;  ++i, ++c) {
		if (ctx->pos[0] != c->cid) JPEG_DECODER_THROW(JpegResultSyntaxError);
		if (ctx->pos[1] & 0xEE) JPEG_DECODER_THROW(JpegResultSyntaxError);
		c->dctabsel = ctx->pos[1] >> 4;
		c->actabsel = (ctx->pos[1] & 1) | 2;
		_Skip(ctx, 2);
	}
	if (ctx->pos[0] || (ctx->pos[1] != 63) || ctx->pos[2]) JPEG_DECODER_THROW(JpegResultUnsupported);
	_Skip(ctx, ctx->length);
	for (mby = 0;  mby < ctx->mbheight;  ++mby)
		for (mbx = 0;  mbx < ctx->mbwidth;  ++mbx) {
			for (i = 0, c = ctx->comp;  i < ctx->ncomp;  ++i, ++c)
				for (sby = 0;  sby < c->ssy;  ++sby)
					for (sbx = 0;  sbx < c->ssx;  ++sbx) {
						_DecodeBlock(ctx, c, &c->pixels[((mby * c->ssy + sby) * c->stride + mbx * c->ssx + sbx) << 3]);
						if (ctx->error)
						return;
					}
			if (ctx->rstinterval && !(--rstcount)) {
				_ByteAlign(ctx);
				i = _GetBits(ctx, 16);
				if (((i & 0xFFF8) != 0xFFD0) || ((i & 7) != nextrst)) JPEG_DECODER_THROW(JpegResultSyntaxError);
				nextrst = (nextrst + 1) & 7;
				rstcount = ctx->rstinterval;
				for (i = 0;  i < 3;  ++i)
					ctx->comp[i].dcpred = 0;
			}
		}
	ctx->error = JpegResultInternal_Finished;
}

static void _UpsampleH(struct Context* ctx, struct Component* c) {
	uint_fast16_t x, y, xmax = c->width - 3;
	uint8_t *out, *lin, *lout;

	//XXX: untested
	out = (uint8_t*)jpgExtAlloc((c->width * c->height) << 1);
	if (!out) JPEG_DECODER_THROW(JpegResultOutOfMemory);
	lin = c->pixels;
	lout = out;
	for (y = c->height;  y;  --y) {
		lout[0] = CF(CF2A * lin[0] + CF2B * lin[1]);
		lout[1] = CF(CF3X * lin[0] + CF3Y * lin[1] + CF3Z * lin[2]);
		lout[2] = CF(CF3A * lin[0] + CF3B * lin[1] + CF3C * lin[2]);
		for (x = 0;  x < xmax;  ++x) {
			lout[(x << 1) + 3] = CF(CF4A * lin[x] + CF4B * lin[x + 1] + CF4C * lin[x + 2] + CF4D * lin[x + 3]);
			lout[(x << 1) + 4] = CF(CF4D * lin[x] + CF4C * lin[x + 1] + CF4B * lin[x + 2] + CF4A * lin[x + 3]);
		}
		lin += c->stride;
		lout += c->width << 1;
		lout[-3] = CF(CF3A * lin[-1] + CF3B * lin[-2] + CF3C * lin[-3]);
		lout[-2] = CF(CF3X * lin[-1] + CF3Y * lin[-2] + CF3Z * lin[-3]);
		lout[-1] = CF(CF2A * lin[-1] + CF2B * lin[-2]);
	}
	c->width <<= 1;
	c->stride = c->width;
	jpgExtFree(c->pixels);
	c->pixels = out;
}

static void _UpsampleV(struct Context* ctx, struct Component* c) {
	uint16_t w = c->width, s1 = c->stride, s2 = s1 + s1, x, y;
	uint8_t *out, *cin, *cout;

	//XXX: untested
	out = (uint8_t*)jpgExtAlloc((c->width * c->height) << 1);
	if (!out) JPEG_DECODER_THROW(JpegResultOutOfMemory);
	for (x = 0;  x < w;  ++x) {
		cin = &c->pixels[x];
		cout = &out[x];
		*cout = CF(CF2A * cin[0] + CF2B * cin[s1]);  cout += w;
		*cout = CF(CF3X * cin[0] + CF3Y * cin[s1] + CF3Z * cin[s2]);  cout += w;
		*cout = CF(CF3A * cin[0] + CF3B * cin[s1] + CF3C * cin[s2]);  cout += w;
		cin += s1;
		for (y = c->height - 3;  y;  --y) {
			*cout = CF(CF4A * cin[-s1] + CF4B * cin[0] + CF4C * cin[s1] + CF4D * cin[s2]);  cout += w;
			*cout = CF(CF4D * cin[-s1] + CF4C * cin[0] + CF4B * cin[s1] + CF4A * cin[s2]);  cout += w;
			cin += s1;
		}
		cin += s1;
		*cout = CF(CF3A * cin[0] + CF3B * cin[-s1] + CF3C * cin[-s2]);  cout += w;
		*cout = CF(CF3X * cin[0] + CF3Y * cin[-s1] + CF3Z * cin[-s2]);  cout += w;
		*cout = CF(CF2A * cin[0] + CF2B * cin[-s1]);
	}
	c->height <<= 1;
	c->stride = c->width;
	jpgExtFree(c->pixels);
	c->pixels = out;
}

static void _Convert(struct Context* ctx) {
	uint_fast8_t i;

	struct Component* c;
	for (i = 0, c = ctx->comp;  i < ctx->ncomp;  ++i, ++c) {
		while ((c->width < ctx->width) || (c->height < ctx->height)) {
			if (c->width < ctx->width) _UpsampleH(ctx, c);
			if (ctx->error) return;
			if (c->height < ctx->height) _UpsampleV(ctx, c);
			if (ctx->error) return;
		}
		if ((c->width < ctx->width) || (c->height < ctx->height)) JPEG_DECODER_THROW(JpegResultInternalError);
	}
	if (ctx->ncomp == 3) {
		// convert to RGB
		uint8_t *py  = ctx->comp[0].pixels;
		uint8_t *pcb = ctx->comp[1].pixels;
		uint8_t *pcr = ctx->comp[2].pixels;
		uint_fast16_t x, y;

		for (y = ctx->height;  y;  --y) {
			for (x = 0;  x < ctx->width;  ++x) {
				int_fast16_t y = py[x] << 8;
				int_fast16_t cb = pcb[x] - 128;
				int_fast16_t cr = pcr[x] - 128;
				uint_fast8_t R = _Clip((y            + 359 * cr + 128) >> 8);
				uint_fast8_t G = _Clip((y -  88 * cb - 183 * cr + 128) >> 8);
				uint_fast8_t B = _Clip((y + 454 * cb            + 128) >> 8);

				py[x] = R;
				pcb[x] = G;
				pcr[x] = B;
			}
			py += ctx->comp[0].stride;
			pcb += ctx->comp[1].stride;
			pcr += ctx->comp[2].stride;
		}
	}
}

static enum DecodeResult _Decode(struct Context* ctx, const uint8_t* jpeg, const uint32_t size) {
	uint_fast8_t i;

	ctx->pos = (const uint8_t*) jpeg;
	ctx->size = size & 0x7FFFFFFF;
	if (ctx->size < 2) return JpegResultNotAJpeg;
	if ((ctx->pos[0] ^ 0xFF) | (ctx->pos[1] ^ 0xD8)) return JpegResultNotAJpeg;
	_Skip(ctx, 2);
	while (!ctx->error) {
		if ((ctx->size < 2) || (ctx->pos[0] != 0xFF)) return JpegResultSyntaxError;
		_Skip(ctx, 2);
		switch (ctx->pos[-1]) {
			case 0xC0: _DecodeSOF(ctx);  break;
			case 0xC4: _DecodeDHT(ctx);  break;
			case 0xDB: _DecodeDQT(ctx);  break;
			case 0xDD: _DecodeDRI(ctx);  break;
			case 0xDA: _DecodeScan(ctx); break;
			case 0xFE: _SkipMarker(ctx); break;
			default:
				if ((ctx->pos[-1] & 0xF0) == 0xE0)
					_SkipMarker(ctx);
				else
					return JpegResultUnsupported;
		}
	}

	//free VLCs now so that if further code needs mem, ir is reused
	for (i = 0;  i < 4;  ++i) {

		vlcFree(ctx->vlctab[i]);
		ctx->vlctab[i] = NULL;
	}

	if (ctx->error != JpegResultInternal_Finished) return ctx->error;
	ctx->error = JpegResultOK;
	_Convert(ctx);
	return ctx->error;
}

struct Context* jpgDecode(const uint8_t* data, size_t size)
{
	struct Context *ctx = jpgExtAlloc(sizeof(*ctx));

	memset(ctx, 0, sizeof(*ctx));

	if (ctx)
		_Decode(ctx, data, size);
	return ctx;
}

void jpgFree(struct Context* ctx)
{
	uint_fast8_t i;

	for (i = 0;  i < 3;  ++i)
		jpgExtFree(ctx->comp[i].pixels);
	jpgExtFree(ctx);
}


enum DecodeResult jpgGetResult(const struct Context* ctx)
{
	return ctx->error;
}

uint_fast16_t jpgGetWidth(const struct Context* ctx)
{
	return ctx->width;
}

uint_fast16_t jpgGetHeight(const struct Context* ctx)
{
	return ctx->height;
}

bool jpgIsColor(const struct Context* ctx)
{
	return ctx->ncomp != 1;
}

const uint8_t* jpgGetImageData(const struct Context* ctx, uint_fast8_t component)
{
	return component < ctx->ncomp ? ctx->comp[component].pixels : 0;
}

uint32_t jpgGetImageStride(const struct Context* ctx, uint_fast8_t component)
{
	return component < ctx->ncomp ? ctx->comp[component].stride : 0;
}



#undef JPEG_DECODER_THROW
