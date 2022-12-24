#include "Src/pngle.h"
#include "pngDrawInt.h"
#include "armcalls.h"



void on_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4], struct DrawState *ds)
{
	uint_fast16_t r = rgba[0] & 0xf8;
	uint_fast16_t g = rgba[1] & 0xfc;
	uint_fast16_t b = rgba[2] & 0xf8;
	uint_fast16_t color = (r << 8) + (g << 3) + (b >> 3);

	uint16_t *dst = ds->bits + y * ds->rowHalfwords + x;

	*dst = __builtin_bswap16(color);
}

static uint32_t read32(const void *fromP)
{
	const uint8_t *from = fromP;
	uint32_t ret = 0;

	ret = (ret << 8) + from[0];
	ret = (ret << 8) + from[1];
	ret = (ret << 8) + from[2];
	ret = (ret << 8) + from[3];

	return ret;
}

int __attribute__((used)) ArmletMain(void *emulStateP, struct ArmParams *pp, void *call68KFuncP)
{
	struct DrawState *ds68k;
	struct DrawState dsArm;
	pngle_t *pngle;
	int ret;

	armCallsInit(emulStateP, call68KFuncP);


	ds68k = (struct DrawState*)read32(&pp->ds);
	dsArm.b = (struct BitmapType*)read32(&ds68k->b);
	dsArm.bits = (uint16_t*)read32(&ds68k->bits);
	dsArm.rowHalfwords = __builtin_bswap16(ds68k->rowHalfwords);

	pngle = pngle_new();
	if (!pngle)
		return -1;

	pngle_set_draw_callback(pngle, &dsArm);


	ret = pngle_feed(pngle, (const void*)read32(&pp->data), read32(&pp->dataSz));

	pngle_destroy(pngle);

	return ret;
}

void __attribute((naked, used, section(".vector"), target("arm"))) __entry(void)
{
	//gcc will refuse to call a thumb functionj from this arm entry point no matter what we do
	//so we are forced to do it ourselves if we want to compile for thumb (we do for space)

	asm volatile(
		"1:									\n"
		"	stmfd	sp!, {r10, r11, lr}		\n"
		"	ldr		r10, =1b				\n"
		"	adr		r11, 1b					\n"
		"	ldr		r12, =ArmletMain		\n"
		"	sub		r12, r10				\n"
		"	add		r12, r11				\n"
		"	mov		lr, pc					\n"
		"	bx		r12						\n"
		"	ldmfd	sp!, {r10, r11, lr}		\n"
		"	bx		lr						\n"
	);
}
