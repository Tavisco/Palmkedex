#include <stdbool.h>
#include <endian.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define MIN_VALID_CHAR			0x20
#define MIN_VALID_INPUT_CHAR	0x20
#define MAX_VALID_INPUT_CHAR	0x7e
#define CHAR_POKEMON			(MAX_VALID_INPUT_CHAR + 1)
#define MAX_VALID_CHAR			CHAR_POKEMON		//for compression
#define TERMINATOR_CHAR			(MAX_VALID_CHAR + 1)
#define NUM_VALID_CHARS			(TERMINATOR_CHAR - MIN_VALID_CHAR + 1)

struct BitBufferW {
	uint8_t *dst;
	uint8_t bitBuf;
	uint8_t numBitsHere;
};

struct BitBufferR {
	uint32_t len;
	uint8_t bitBuf;
	uint8_t numBitsHere;
};

static uint_fast8_t bbRead(struct BitBufferR *bb)	//read a bit
{
	uint_fast8_t ret;

	if (!bb->numBitsHere) {

		bb->numBitsHere = 8;
		if (!bb->len)
			bb->bitBuf = 0;
		else {
			int c;

			bb->len--;
			c = getchar();

			if (c == EOF) {
				fprintf(stderr, "unexpected EOF\n");
				exit(-2);
			}
			bb->bitBuf = c;
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
		*(bb->dst)++ = bb->bitBuf;
		bb->bitBuf = 0;
		bb->numBitsHere = 0;
	}
}

static void bbFlush(struct BitBufferW *bb)
{
	while (bb->numBitsHere)
		bbWrite(bb, 0);
}

static void putU16(uint16_t val)
{
	putchar(val >> 8);
	putchar(val);
}

static uint16_t getU16(void)
{
	int first = getchar(), second = getchar();

	if (first == EOF || second == EOF) {

		fprintf(stderr, "EOF too soon\n");
		exit(-1);
	}
	return first * 256 + second;
}

static unsigned compressString(uint8_t *dst, const uint8_t *src, unsigned inLen, const uint16_t *start, const uint16_t *end)
{
	uint16_t min = 0x00000000, max = 0x0000ffff, numTruncated = 0, i;
	struct BitBufferW bb = {.dst = dst, };

	for (i = 0; i < inLen + 1; i++) {

		uint8_t idx = ((i == inLen) ? TERMINATOR_CHAR : src[i]) - MIN_VALID_CHAR;
		uint32_t width = (uint32_t)max - min + 1;

		fprintf(stderr, "[%u / %u] range %08xh..%08xh char %c(%02xh) -> {%04xh..%05xh}\n", i, inLen,
			min, max, idx + MIN_VALID_CHAR, idx + MIN_VALID_CHAR, start[idx], end[idx]);

		//calc new range
		max = min + width * end[idx] / 0x4000 - 1;
		min = min + width * start[idx] / 0x4000;

		fprintf(stderr, " 1 range now %08xh..%08xh\n", min, max);

		while ((min >> 15) == (max >> 15)) {

			uint_fast8_t bit = min >> 15;

			bbWrite(&bb, bit);
			bit = 1 - bit;
			while (numTruncated) {
				bbWrite(&bb, bit);
				numTruncated--;
			}
			min = min * 2;
			max = max * 2 + 1;
		}

		while ((min >> 14) == 1 && (max >> 14) == 2) {

			numTruncated++;
			min = (min << 1) ^ 0x8000;
			max = (max << 1) ^ 0x8001;
		}
		fprintf(stderr, " 2 range now %08xh..%08xh\n", min, max);

	}
	bbWrite(&bb, 1);
	bbFlush(&bb);

	return bb.dst - dst;
}

static unsigned extractString(unsigned len, const uint16_t *start, const uint16_t *end)	//return num chars used
{
	uint16_t min = 0x00000000, max = 0x0000ffff, val = 0;
	struct BitBufferR bb = {.len = len};
	char lastchar = 0;
	unsigned i;

	//read initial value
	for (i = 0; i < 16; i++)
		val = val * 2 + bbRead(&bb);

	while (1) {

		uint32_t width = (uint32_t)max - min + 1;
		uint32_t above = val - min;
		uint32_t now = ((above + 1) * 0x4000 - 1) / width;
		uint_fast8_t idxNow;

		//could be faster ... later
		for (idxNow = MIN_VALID_CHAR; idxNow <= TERMINATOR_CHAR; idxNow++) {

			if (now >= start[idxNow - MIN_VALID_CHAR] && now < end[idxNow - MIN_VALID_CHAR])
				break;
		}

		fprintf(stderr, " %04xh..%04xh..%04xh -> %04x -> %u ('%c' %02xh) with range %04x..%04x\n",
			min, val, max, now,
			idxNow, idxNow, idxNow,
			start[idxNow - MIN_VALID_CHAR], end[idxNow - MIN_VALID_CHAR]);

		//emit byte (or handle the terminator)
		if (idxNow == TERMINATOR_CHAR)
			break;
		if (idxNow == CHAR_POKEMON) {
			printf("POKEMON");
			lastchar = 'N';
		}
		else {
			
			putchar(idxNow);
			lastchar = idxNow;
		}
		
		//calc new range
		max = min + width * end[idxNow - MIN_VALID_CHAR] / 0x4000 - 1;
		min = min + width * start[idxNow - MIN_VALID_CHAR] / 0x4000;

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
	}
	
	if (lastchar != '!')
		putchar('.');
	
	return len - bb.len;
}

int main(int argc, char **argv)
{
	unsigned i, j, numStrings = 0, *compressedLengths, curOfst = 0;
	uint16_t start[NUM_VALID_CHARS], end[NUM_VALID_CHARS];
	char **sourceStrings = NULL;
	uint8_t **compressedStrings;
	char descr[512];	//so sue me

	if (argc != 2 || strcmp(argv[1], "d")) {

		uint32_t count[NUM_VALID_CHARS] = {}, totalCount = 0;

		while (fgets(descr, sizeof(descr), stdin)) {

			while (strlen(descr) && (descr[strlen(descr) - 1] == '\n' || descr[strlen(descr) - 1] == '\r'))
				descr[strlen(descr) - 1] = 0;

			if (!strlen(descr))
				break;

			//replace "POKEMON" with CHAR_POKEMON
			for (j = 0, i = 0; descr[i]; i++) {
				
				if (descr[i] < MIN_VALID_INPUT_CHAR || descr[i] > MAX_VALID_INPUT_CHAR) {
					fprintf(stderr, "unexpected char '%c'(%02x)\n", descr[i], descr[i]);
					exit(-1);
				}
				
				if (strncmp(descr + i, "POKEMON", 7))
					descr[j++] = descr[i];
				else {
					i += 6;
					descr[j++] = CHAR_POKEMON;
				}
			}
			descr[j] = 0;
			
			//remove dot at end
			if (j && descr[j - 1] == '.')
				descr[j - 1] = 0;
			
			sourceStrings = realloc(sourceStrings, (numStrings + 1) * sizeof(char*));
			sourceStrings[numStrings++] = strdup(descr);

			for (i = 0; i < strlen(descr); i++) {
				uint8_t ch = descr[i];

				if (ch < MIN_VALID_CHAR || ch > MAX_VALID_CHAR) {
					fprintf(stderr, "unexpected char '%c'(%02x)\n", ch, ch);
					exit(-1);
				}
				count[ch - MIN_VALID_CHAR]++;
				totalCount++;
			}
		}
		compressedStrings = calloc(sizeof(char*), numStrings);
		compressedLengths = calloc(sizeof(unsigned), numStrings);

		count[NUM_VALID_CHARS - 1] = numStrings;	//the number of terminators matches the number of strings...

		//allocate weights from end to start (so that space gets more range)
		end[TERMINATOR_CHAR - MIN_VALID_CHAR] = 0x4000;
		for (i = TERMINATOR_CHAR; i > MIN_VALID_CHAR; i--) {

			uint32_t len = 0x4000ull * count[i - MIN_VALID_CHAR] / totalCount;

			if (count[i - MIN_VALID_CHAR] && !len)
				len++;

			end[i - MIN_VALID_CHAR - 1] = start[i - MIN_VALID_CHAR] = end[i - MIN_VALID_CHAR] - len;
		}
		start[0] = 0;

		for (i = MIN_VALID_CHAR; i <= TERMINATOR_CHAR; i++)
			fprintf(stderr, "{'%c'(%02x)} x %u allocated range %04xh..%04xh\n", i, i, count[i - MIN_VALID_CHAR], start[i - MIN_VALID_CHAR], end[i - MIN_VALID_CHAR]);

		//compress
		for (i = 0; i < numStrings; i++) {

			const char *source = sourceStrings[i];
			unsigned inLen = strlen(source);

			fprintf(stderr, "inStr = '%s'\n", source);
			compressedLengths[i] = compressString(compressedStrings[i] = malloc(inLen * 128), (const uint8_t*)source, inLen, start, end);
			fprintf(stderr, "[%3u] compressed %u -> %u\n", i, inLen, compressedLengths[i]);
			free(sourceStrings[i]);
		}
		free(sourceStrings);

		//put num strings
		putU16(numStrings);
		curOfst += 2;

		//put ranges
		for (i = 0; i < NUM_VALID_CHARS - 1; i++) {

			putU16(end[i] - start[i]);
			curOfst += 2;
		}

		//put offsets
		curOfst += (numStrings + 1) * 2;
		for (i = 0; i < numStrings; i++) {

			putU16(curOfst);
			curOfst += compressedLengths[i];
		}
		putU16(curOfst);

		//put data
		for (i = 0; i < numStrings; i++) {

			for (j = 0; j < compressedLengths[i]; j++)
				putchar(compressedStrings[i][j]);

			free(compressedStrings[i]);
		}
		free(compressedStrings);
		free(compressedLengths);

		return 0;
	}
	else {	//decompress

		uint16_t *offsets;

		//read num strings
		numStrings = getU16();
		curOfst += 2;

		//read ranges
		start[0] = 0;
		for (i = 0; i < NUM_VALID_CHARS - 1; i++) {

			end[i] = start[i + 1] = start[i] + getU16();
			curOfst += 2;
		}
		end[i] = 0x4000;

		//read offsets
		offsets = calloc(sizeof(uint16_t), numStrings + 1);
		for (i = 0; i <= numStrings; i++) {

			offsets[i] = getU16();
			curOfst += 2;
		}

		//extract each string
		for (i = 0; i < numStrings; i++) {

			//sanity check
			if (offsets[i] != curOfst)
				fprintf(stderr, "at offset %u, expected %u\n", curOfst, offsets[0]);

			fprintf(stderr, "[%03u]:\n", i);
			curOfst += extractString(offsets[i + 1] - offsets[i], start, end);
			printf("\n");
		}
	}
}
