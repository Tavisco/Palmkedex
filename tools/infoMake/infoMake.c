#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>



static char pkmnsNames[][12] = {
{"Bulbasaur"},
{"Ivysaur"},
{"Venusaur"},
{"Charmander"},
{"Charmeleon"},
{"Charizard"},
{"Squirtle"},
{"Wartortle"},
{"Blastoise"},
{"Caterpie"},
{"Metapod"},
{"Butterfree"},
{"Weedle"},
{"Kakuna"},
{"Beedrill"},
{"Pidgey"},
{"Pidgeotto"},
{"Pidgeot"},
{"Rattata"},
{"Raticate"},
{"Spearow"},
{"Fearow"},
{"Ekans"},
{"Arbok"},
{"Pikachu"},
{"Raichu"},
};

static const char *types[] = {
	"None",
	"Normal",
	"Fire",
	"Water",
	"Grass",
	"Electric",
	"Rock",
	"Ground",
	"Ice",
	"Flying",
	"Fighting",
	"Ghost",
	"Bug",
	"Poison",
	"Psychic",
	"Steel",
	"Dark",
	"Dragon",
	"Fairy",
	"Unknown",
	"Shadow",
};

struct PokeInfo {	//must match provided resource data
	uint8_t hp, atk, def, spAtk, spDef, speed, type[2];
} __attribute__((packed));


static const char pokeNameChars[] = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-";

struct BB {
	uint8_t *dst;
	uint16_t buffer;
	uint8_t bitsUsed;
};

static void bbEmit(struct BB *bb, uint8_t val, uint8_t bits)
{
	bb->buffer += ((uint16_t)val) << bb->bitsUsed;
	bb->bitsUsed += bits;
	while (bb->bitsUsed >= 8) {
		*(bb->dst)++ = bb->buffer;
		bb->buffer >>= 8;
		bb->bitsUsed -= 8;
	}
}

static void bbFlush(struct BB *bb)
{
	while (bb->bitsUsed)
		bbEmit(bb, 1, 1);
}

int main(int argc, char** argv)
{
	unsigned i, ofst, numPokes = sizeof(pkmnsNames) / sizeof(*pkmnsNames), prevVal;
	uint8_t compressedData[numPokes][16];
	uint8_t compressedLengths[numPokes];
	struct PokeInfo infos[numPokes];
	
	
	for (i = 0; i < numPokes; i++) {
		
		char fname[32];
		FILE *f;
		
		sprintf(fname, "data/%04u.bin", i + 1);
		f = fopen(fname, "rb");
		if (sizeof(struct PokeInfo) != fread(infos + i, 1, sizeof(struct PokeInfo), f))
			abort();
		fclose(f);
		
		if (infos[i].type[1] == 21)
			infos[i].type[1] = 0;
	}
	
	puts("//pokemon info:");
	puts("// bytes of name (4 bit len, then 6 bits per char: 0-9A-Za-z-)");
	puts("// type (5 bits)");
	puts("// hasSecondType? (1 bit), if so, another 5 bits of type2");
	puts("// stats, one byte each (hp, atk, def, spAtk, spDef, speed)");
	puts("HEX \"INFO\" ID 0");
	
	ofst = 2 * (1 + numPokes);
	
	//compress names and types
	for (i = 0; i < numPokes; i++) {
		
		struct BB bb = {.dst = compressedData[i], };
		unsigned j = 0;
		char name[12];
		
		strcpy(name, pkmnsNames[i]);
		
		while (strlen(name) < 4)	//max len is 11, so with a 3 bit len range we can encode 4..11, extend shorted ones with spaces. this is rare enough to make space savings worth it
			strcat(name, " ");
		
		bbEmit(&bb, strlen(name) - 4, 3);
		char ch;
		
		while ((ch = name[j++]) != 0) {
			
			char *at = strchr(pokeNameChars, ch);
			unsigned encoded;
			
			if (!at) {
				
				fprintf(stderr, "Saw uncompressible char in poke name '%s'. Char was '%c'(%02xh)\n", pkmnsNames[i], ch, ch);
				abort();
			}
			bbEmit(&bb, at - pokeNameChars, 6);
		}
		
		//types
		bbEmit(&bb, infos[i].type[0], 5);
		if (infos[i].type[1]) {
			
			bbEmit(&bb, 1, 1);
			bbEmit(&bb, infos[i].type[1], 5);
		}
		else {
			
			bbEmit(&bb, 0, 1);
		}
		
		bbFlush(&bb);
		compressedLengths[i] = bb.dst - compressedData[i];
	}
	printf("0x%02x 0x%02x //NUM POKEMON\n", (uint8_t)(numPokes >> 8), (uint8_t)numPokes);
	printf("//Now: offsets into this resource for each poke's data. Each poke occupies a minimum of 10 bytes (left as an exercise to you to prove this)\n");
	printf("//each offset is stores as 12 bits. A and B are stored as A.lo (A.hi + 16 * B.hi) B.lo\n");
	printf("//offsets are indexed from the FIRST full byte that follows ALL the offsets\n");
	printf("//offset for poke N is stored as (actual_offset - 10 * N)\n");
	
	ofst = 0;
	prevVal = 0;
	
	for (i = 0; i < numPokes; i++) {
		
		uint32_t effectiveOffset = ofst - 10 * i;
		
		if (effectiveOffset > 0x1000) {
			fprintf(stderr, "offset not encodeable\n");
			abort();
		}
		
		if (i & 1) {	//second value - emit

			printf(" 0x%02x 0x%02x 0x%02x // offsets for pokes %u and %u (%u and %u encoded)\n", 
					(uint8_t)prevVal, (uint8_t)(prevVal >> 8) + (uint8_t)((effectiveOffset >> 8) << 4), (uint8_t)effectiveOffset, i, i + 1, prevVal, effectiveOffset);
		}
		else {			//first value - record
			
			prevVal = effectiveOffset;
		}
		
		ofst += 6 + compressedLengths[i];
	}
	//emit last offset, if any, do not emit 3 bytes when two will do
	if (numPokes & 1) {
		printf(" 0x%02x 0x%02x // offset for pokes %u\n", 
					(uint8_t)prevVal, (uint8_t)(prevVal >> 8), i);
	}
	
	for (i = 0; i < numPokes; i++) {
		
		unsigned j;
		
		
		printf(	"\t//#%u (%s: %-8s %8s):\n"
				"\t\t%-3u %-3u %-3u %-3u %-3u %-3u  //stats\n\t\t",
				i + 1, pkmnsNames[i], types[infos[i].type[0]], types[infos[i].type[1]],
				infos[i].hp, infos[i].atk, infos[i].def, infos[i].spAtk, infos[i].spDef, infos[i].speed);
		
		for (j = 0; j < compressedLengths[i]; j++)
			printf("0x%02x ", compressedData[i][j]);
		printf(" //name and type(s)\n");
	}
	
	puts("\n");
	puts("//pokemon index");
	puts("//first: BE word offsets to start of each of the 26 chains");
	puts("//then: 26 chains.");
	puts("// each chain is a list of BE words, 0-terminated of pokemon starting with that letter in increasing pokemon order");

	
	printf("HEX \"INDX\" ID 0\n");
	uint16_t perLetter[26][1024], pos[26] = {};
	uint32_t totalOfst = 26;
	
	//list pokes by letter
	for (i = 0; i < sizeof(pkmnsNames) / sizeof(*pkmnsNames); i++) {
		
		uint16_t pokeId = i + 1;
		char firstLetter = pkmnsNames[i][0];
		
		if (firstLetter >= 'a' && firstLetter <= 'z')
			firstLetter += 'A' - 'a';
		
		perLetter[firstLetter - 'A'][pos[firstLetter - 'A']++] = pokeId;
	}
	
	//account for the terminators in lengths
	for (i = 0; i < 26; i++)
		perLetter[i][pos[i]++] = 0;
	
	//first the indices into the chain starts
	printf("\t//word offsets of the start of each chain:\n");
	for (i = 0; i < 26; i++) {
		printf(" 0x%02x 0x%02x\n", (uint8_t)(totalOfst >> 8), (uint8_t)totalOfst);
		totalOfst += pos[i];
	}
	printf("\n");
	//now the chains
	for (i = 0; i < 26; i++) {
		unsigned j;
		
		printf("\t//List of pokemon for the letter '%c':\n\t\t", 'A' + i);
		for (j = 0; j < pos[i]; j++) {
			printf("  0x%02x 0x%02x", (uint8_t)(perLetter[i][j] >> 8), (uint8_t)perLetter[i][j]);
		}
		printf("\n");
	}
}