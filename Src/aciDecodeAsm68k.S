//palmos allows d0-d2, a0-a1 to be clobbered
//bool aciDecodeBits(uint8_t *dst, uint16_t dstExtraStride, uint16_t w, uint16_t h, const struct PixelRange *ranges, uint16_t numColors, const uint8_t *src, const uint8_t *srcEnd);
//before push:        4(sp)            8(sp)             10(sp)     12(sp)         14(sp)                         18(sp)                 20(sp)            24(sp)
//after push:        arg(val_above)(sp)
//by compressor spec, at least one byte of input is guaranteed

#define arg(_x)		(44 + _x)

.macro get_bit				//into carry and extend flag
	subq.w		#1, %d5
	bpl.b		9f
	cmpa.l		%a0, %a1
	beq.b		8f
	move.b		(%a0)+, %d3
8:
	moveq		#7, %d5
9:
	lsr			#1, %d3
.endm

.globl aciDecodeBits
aciDecodeBits:

	movem.l		%d2-%d7/%a2-%a6,-(%sp)	//d2 stored to make space on stack

	movea.l		arg(20)(%sp), %a0		//a0 = "src"
	movea.l		arg(24)(%sp), %a1		//a1 = "srcEnd"
	movea.l		arg(14)(%sp), %a2		//a2 = "ranges"
	movea.w		arg(18)(%sp), %a3		//a3 = "numColors"
	subq.l		#1, %a3
	adda.l		%a3, %a3
	adda.l		%a3, %a3
	adda.l		%a3, %a3
	adda.l		%a2, %a3				//a3 = "&ranges[numColors - 1]"
	movea.l		arg(4)(%sp), %a5		//a5 = "dst"

	moveq		#0, %d6					//d6.w = min
	move.w		#0xffff, %d7			//d7.w = max
	moveq		#0, %d5					//d5.b = "bitsAvail"
	moveq		#0, %d4					//d4 = "val"
	moveq		#0, %d3					//d3 = bit reservoir for reading

//read initial "val"
	moveq		#15, %d0
1:
	get_bit
	addx.w		%d4, %d4
	dbra.w		%d0, 1b

	move.w		arg(12)(%sp), %a6		//a6 = "h" for loop

row_loop:

	move.w		arg(10)(%sp), %d1		//d1.w = inverse of "w" for dbra
	subq.w		#1, %d1

col_loop:

	move.l		%d4, %d2				//d2.l = "above" = val - min
	sub.w		%d6, %d2
	addq.l		#1, %d2
	lsl.l		#8, %d2
	subq.l		#1, %d2					//d2.l = (above * 256 + 255)

	move.w		%d7, %d0
	sub.w		%d6, %d0
	addq.w		#1, %d0					//d0 = width.lo16
	beq.b		1f
	divu.w		%d0, %d2
	bra			2f
1:
	swap		%d2
2:										//%d2.w = "now" = (above * 256 + 255) / width

	//we'll try linear search from end for ease of assembly

	movea.l		%a3, %a4				//a4 = &ranges[numColors - 1]

2:
	cmp.w		4(%a4), %d2				//set flags on now - a4->start
	blt.b		1f
	cmp.w		6(%a4), %d2				//set flags on now - a4->end
	blt.b		2f						//match
1:
	subq.l		#8, %a4
	cmpa.l		%a2, %a4				//set flags on a4 - a2
	bpl.b		2b
2:										//match

	cmpa.l		%a4, %a3				//d2 = "idxNow" = (center == (uint16_t)(numColors - 1)) ? prevIdx : center;
										//0(sp) = "prevIdx" = idxNo		//realy prevIdx and idxNow store: &ranges[$_SOMETHING_$]
	beq.b		1f
	move.b		(%a4), %d2
	move.b		%d2, 0(%sp)
	bra.b		2f
1:
	move.b		0(%sp), %d2
2:

	move.b		%d2, (%a5)+				//*dst++ = ranges[idxNow].color

	tst.w		%d0
	beq.b		1f
	move.w		6(%a4), %d7				//max = min + mul16x16_32(width, ranges[center].end) / 256 - 1;
	mulu.w		%d0, %d7
	lsr.l		#8, %d7					//guaranteed to produce max 16 bits
	subq.w		#1, %d7
	add.w		%d6, %d7
	move.w		4(%a4), %d2				//min += mul16x16_32(width, ranges[center].start) / 256;
	mulu.w		%d0, %d2
	lsr.l		#8, %d2					//guaranteed to produce max 16 bits
	add.w		%d2, %d6

	bra.b		2f
1:
	move.b		7(%a4), %d7				//same as: "move.w 6(%a4), %d7 ; lsl.w #8, %d7"
	lsl.w		#8, %d7					//max = min + ranges[center].end * 256 - 1;
	subq.w		#1, %d7
	add.w		%d6, %d7

	move.b		5(%a4), %d2				//same as: "move.w 4(%a4), %d2 ; lsl.w #8, %d2"
	lsl.w		#8, %d2					//min = min + ranges[center].start * 256;
	add.w		%d2, %d6
2:

handle_equal_top_bits:
	move.w		%d6, %d2				//while ((min >> 15) == (max >> 15)) {
	eor.w		%d7, %d2
	bmi.b		equal_top_bits_handled
//top bits match

	add.w		%d6, %d6				//	min = min * 2;
	add.w		%d7, %d7				//	max = max * 2 + 1;
	addq.w		#1, %d7

	get_bit
	addx.w		%d4, %d4				//	val = val * 2 + bbRead(&bb);

	bra.b		handle_equal_top_bits	//}
equal_top_bits_handled:

handle_indeterminate:
	//we need to check if top bits of "min" are 01 and of "max "are 10
	//we know their current top bits DIFFER, adn we know min < max
	//thus we know the top bits ARE correct, just check bits #14
	btst		#14, %d6				//while ((min >> 14) == 1 && (max >> 14) == 2) {
	beq.b		indeterminate_handled
	btst		#14, %d7
	bne.b		indeterminate_handled
//top bits as expected

	move.w		#0x8000, %d2

	add.w		%d6, %d6				//	min = (min << 1) ^ 0x8000;
	eor.w		%d2, %d6
	add.w		%d7, %d7				//	max = (max << 1) ^ 0x8001;
	eori.w		#0x8001, %d7

	and.w		%d4, %d2				//	val = (val & 0x8000) + (val & 0x3fff) * 2 + bbRead(&bb);
	andi.w		#0x3fff, %d4
	get_bit
	addx.w		%d4, %d4
	add.w		%d2, %d4

	bra.b		handle_indeterminate	//}
indeterminate_handled:

	dbra		%d1, col_loop

	adda.w		arg(8)(%sp), %a5		//dst += dstExtraStride

	subq.l		#1, %a6
	move.l		%a6, %d2				//just to set flags since "subq" should but doesnt
	bne			row_loop

	moveq.l		#1, %d0
	movem.l		(%sp)+, %d2-%d7/%a2-%a6
	rts

/*
{
	uint16_t i, r, c;
	uint16_t min = 0, max = 0xffff, val = 0;
	uint_fast16_t prevIdx = numColors - 1;
	struct BitBuffer bb;

	bbInit(&bb, src, srcEnd);

	//init state
	for (i = 0; i < sizeof(val) * 8; i++)
		val = val * 2 + bbRead(&bb);

	for (r = 0; r < h; r++, dst += dstExtraStride) {

		for (c = 0; c < w; c++) {

			uint32_t width = (uint32_t)max - min + 1;
			uint16_t above = val - min;
			uint16_t now;
			uint_fast8_t idxNow;

			//this is faster than long multiplication and long division
			//now = (above * 256 + 255) / width
			//result will always be a byte, but gcc sucks at compiling this
			//width will never be zero, so comparnig with zero is as good as comparing with 0x10000
			now = ((uint32_t)above * 256 + 255) / width;

			//find matching symbol for "now". binary search
			uint_fast16_t end = numColors, start = 0, center;

			while (end > start) {

				center = (end + start) / 2;

				if (ranges[center].end <= now)
					start = center;
				else if (ranges[center].start > now)
					end = center;
				else
					break;
			}

			idxNow = (center == (uint16_t)(numColors - 1)) ? prevIdx : center;
			prevIdx = idxNow;

			//emit the pixel
			*dst++ = ranges[idxNow].color;

			//calc new range (once again, gcc sucks)
			max = min + width * ranges[center].end / 256 - 1;
			min = min + width * ranges[center].start / 256;

			while ((min >> 15) == (max >> 15)) {

				min = min * 2;
				max = max * 2 + 1;

				val = val * 2 + bbRead(&bb);
			}

			while ((min >> 14) == 1 && (max >> 14) == 2) {

				min = (min << 1) ^ 0x8000;
				max = (max << 1) ^ 0x8001;

				val = (val & 0x8000) + (val & 0x3fff) * 2 + bbRead(&bb);
			}

			if (min > max) {

				ErrAlertCustom(0, "range crossed", 0, 0);
				return false;
			}
			if (val < min || val > max) {

				ErrAlertCustom(0, "val OOB", 0, 0);
				return false;
			}
		}
	}

	return true;
}

*/



//void aciRepack(UInt8* buffer, UInt32 npixels, UInt8 depth)
//                      4(sp)     8(sp)           12(sp)
//we are guaranteed thta we'll produce an integer number of 16-bit words as output!
//do not pass-in zero width or height
#undef arg
#define arg(_x)		(0 + _x)
.globl aciRepack
aciRepack:
	move.l		arg(4)(%sp), %a0			//a0 = dst
	move.l		%a0, %a1					//a1 = src
	move.l		arg(8)(%sp), %d0
	cmpi.b  	#2, arg(12)(%sp)
	beq.b		aciRepack_2bpp
	blt.b		aciRepack_1bpp

aciRepack_4bpp:
	move.l		(%a0)+, %d2
	move.l		%d2, %d1
	lsr.l		#4, %d1
	add.l		%d1, %d2
	swap		%d2
	move.b		%d2, (%a1)+
	swap		%d2
	move.b		%d2, (%a1)+
	subq.w		#4, %d0
	bne.b		aciRepack_4bpp
	rts

aciRepack_2bpp:
//we know DESTINATION has integer multiples of words (thus integer multiple of 8 pixels) so we can untoll this loop 2x
.rept 2
	move.l		(%a0)+, %d2
	move.l		%d2, %d1
	lsr.l		#6, %d1
	add.l		%d1, %d2
	move.l		%d2, %d1
	swap		%d1
	lsl.w		#4, %d1
	add.w		%d1, %d2
	move.b		%d2, (%a1)+
.endr
	subq.w		#8, %d0
	bne.b		aciRepack_2bpp
	rts

aciRepack_1bpp:
//we know DESTINATION has integer multiples of words (thus integer multiple of 16 pixels) so we can untoll this loop 4x, but we only do 2x, as a compromise of code size
.rept 2
	move.l		(%a0)+, %d2
	move.l		%d2, %d1
	lsr.l		#7, %d1
	add.l		%d1, %d2
	move.l		%d2, %d1
	swap		%d1
	lsl.w		#2, %d1
	add.w		%d1, %d2
	lsl.w		#4, %d2
	move.b		%d2, (%a1)
	move.l		(%a0)+, %d2
	move.l		%d2, %d1
	lsr.l		#7, %d1
	add.l		%d1, %d2
	move.l		%d2, %d1
	swap		%d1
	lsl.w		#2, %d1
	add.w		%d1, %d2
	add.b		%d2, (%a1)+
.endr
	subi.w		#16, %d0
	bne.b		aciRepack_1bpp
	rts