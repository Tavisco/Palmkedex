#include <PalmOS.h>

#include "Palmkedex.h"
#include "Rsc/Palmkedex_Rsc.h"
#include "Src/pokeInfo.h"
#include "Src/imgDraw.h"
#include "osExtra.h"

#define POKE_IMAGE_AT_X		1
#define POKE_IMAGE_AT_Y		16


static const char emptyString[1] = {0};	//needed for PalmOS under 4.0 as we cannot pass NULL to FldSetTextPtr

static void DrawTypes(const struct PokeInfo *info);


static void DrawPkmnPlaceholder(void)
{
	MemHandle h;
	BitmapPtr bitmapP;
	h = DmGetResource(bitmapRsc, BmpMissingPokemon);

	bitmapP = (BitmapPtr)MemHandleLock(h);
	ErrFatalDisplayIf(!bitmapP, "Failed to lock placeholder bmp");

	WinDrawBitmap(bitmapP, POKE_IMAGE_AT_X, POKE_IMAGE_AT_Y);
	MemPtrUnlock(bitmapP);
	DmReleaseResource(h);
}

static void DrawPkmnSprite(UInt16 selectedPkmnId)
{
	MemHandle imgMemHandle;
	MemPtr pngData;
	UInt32 size;
	int ret;
	BitmapType *bmpP;
	WinHandle win;
	Err error;
	struct DrawState *ds = NULL;

	// Check if the PNG for the current pkmn
	// is already decoded in memory
	error = FtrGet(appFileCreator, ftrPokeImage, (UInt32*)&ds);
	if (error == errNone && ds)
	{
		// If it is, draw it and return
		imgDrawRedraw(ds, POKE_IMAGE_AT_X, POKE_IMAGE_AT_Y);
		return;
	}

	// Check if there is any image for current pkmn
	imgMemHandle = pokeImageGet(selectedPkmnId);
	if (imgMemHandle) {

		if (imgDecode(&ds, MemHandleLock(imgMemHandle), MemHandleSize(imgMemHandle), 64, 64, 0))
			imgDrawRedraw(ds, POKE_IMAGE_AT_X, POKE_IMAGE_AT_Y);
		else
			ds = NULL;
		MemHandleUnlock(imgMemHandle);
		pokeImageRelease(imgMemHandle);
	}
	// And store its pointer to quickly redraw it
	FtrSet(appFileCreator, ftrPokeImage, (UInt32)ds);

	if (!ds)
		DrawPkmnPlaceholder();
}

void LoadPkmnStats(void)
{
	SharedVariables *sharedVars;
	struct PokeInfo info;
	FormType *frm;
	ListType *list;
	Err err;

	err = FtrGet(appFileCreator, ftrShrdVarsNum, (UInt32*)&sharedVars);
	ErrFatalDisplayIf(err != errNone, "Failed to load feature memory");

	pokeInfoGet(&info, sharedVars->selectedPkmnId);

	frm = FrmGetActiveForm();

	SetLabelInfo(PkmnMainHPValueLabel, info.hp, frm);
	SetLabelInfo(PkmnMainAtkValueLabel, info.atk, frm);
	SetLabelInfo(PkmnMainDefValueLabel, info.def, frm);
	SetLabelInfo(PkmnMainSPAtkValueLabel, info.spAtk, frm);
	SetLabelInfo(PkmnMainSPDefValueLabel, info.spDef, frm);
	SetLabelInfo(PkmnMainSpeedValueLabel, info.speed, frm);
	DrawTypes(&info);

	list = GetObjectPtr(PkmnMainPopUpList);
	LstSetSelection(list, 0);

	SetDescriptionField(sharedVars->selectedPkmnId);
	DrawPkmnSprite(sharedVars->selectedPkmnId);

	SetFormTitle(sharedVars);
}

void SetDescriptionField(UInt16 selectedPkmnId)
{
	FieldType *fld = GetObjectPtr(PkmnMainDescField);
	char *text = pokeDescrGet(selectedPkmnId);

	if (!text)
		text = (char*)emptyString;

	FldSetTextPtr(fld, text);
	FldRecalculateField(fld, true);
}

static void FreeDescriptionField(void)
{
	FieldType *fld = GetObjectPtr(PkmnMainDescField);
	void *ptr = FldGetTextPtr(fld);

	FldSetTextPtr(fld, (char*)emptyString);

	if (ptr && ptr != (char*)emptyString)
		MemPtrFree(ptr);
}

static void DrawTypes(const struct PokeInfo *info)
{
	MemHandle h;
	BitmapPtr bitmapP;

	h = DmGetResource(bitmapRsc, POKEMON_TYPE_IMAGES_BASE + (UInt8)info->type[0]);
	ErrFatalDisplayIf(!h, "Failed to load type bmp");

	bitmapP = (BitmapPtr)MemHandleLock(h);
	ErrFatalDisplayIf(!bitmapP, "Failed to lock type bmp");

	WinDrawBitmap(bitmapP, 1, 82);
	MemPtrUnlock(bitmapP);
	DmReleaseResource(h);

	if (info->type[1] != PokeTypeNone)
	{
		h = DmGetResource(bitmapRsc, POKEMON_TYPE_IMAGES_BASE + (UInt8)info->type[1]);
		ErrFatalDisplayIf(!h, "Failed to load type bmp");

		bitmapP = (BitmapPtr)MemHandleLock(h);
		ErrFatalDisplayIf(!bitmapP, "Failed to lock type bmp");

		WinDrawBitmap(bitmapP, 34, 82);
		MemPtrUnlock(bitmapP);
		DmReleaseResource(h);
	}
}

void SetLabelInfo(UInt16 labelId, UInt8 stat, FormType *frm)
{
	Char *str;

	str = (Char *)MemPtrNew(sizeof(Char[4]));
	if ((UInt32)str == 0)
		return;
	MemSet(str, sizeof(Char[4]), 0);

	StrIToA(str, stat);
	FrmCopyLabel(frm, labelId, str);

	MemPtrFree(str);
}

void SetFormTitle(SharedVariables *sharedVars)
{
	char titleStr[24];

	StrCopy(titleStr, pokeNameGet(sharedVars->selectedPkmnId));
	StrCat(titleStr, " #");
	StrIToA(titleStr + StrLen(titleStr), sharedVars->selectedPkmnId);

	FrmCopyTitle(FrmGetActiveForm(), titleStr);
}

static void PkmnDescriptionScroll(WinDirectionType direction)
{
	Int16 value;
	Int16 min;
	Int16 max;
	Int16 pageSize;
	UInt16 linesToScroll;
	FieldPtr fld;
	ScrollBarPtr bar;

	fld = GetObjectPtr(PkmnMainDescField);

	if (FldScrollable(fld, direction))
	{
		linesToScroll = FldGetVisibleLines(fld) - 1;
		FldScrollField(fld, linesToScroll, direction);
	}
}

static void unregisterCurrentPng(void)
{
	struct DrawState *ds;

	if (FtrGet(appFileCreator, ftrPokeImage, (UInt32*)&ds) == errNone && ds)
	{
		imgDrawStateFree(ds);
		FtrUnregister(appFileCreator, 0);
	}
}

/*
 * FUNCTION: PkmnMainFormDoCommand
 *
 * DESCRIPTION: This routine performs the menu command specified.
 *
 * PARAMETERS:
 *
 * command
 *     menu item id
 */

static Boolean PkmnMainFormDoCommand(UInt16 command)
{
	Boolean handled = false;

	switch (command)
	{
	case PkmnMainBackButton:
	{
		FrmGotoForm(MainForm);
		handled = true;
		break;
	}

	default:
		break;
	}

	return handled;
}

struct m68kLCDC {	//@0xfffffa00
	volatile UInt32 LSSA;
	UInt8 rfu0[1];
	volatile UInt8 LVPW;
	UInt8 rfu1[2];
	volatile UInt16 LXMAX;
	volatile UInt16 LYMAX;
	UInt8 rfu2[12];
	volatile UInt16 LCXP;
	volatile UInt16 LCYP;
	volatile UInt16 LCWCH;
	UInt8 rfu3[1];
	volatile UInt8 LBLKC;
	volatile UInt8 LPICF;
	volatile UInt8 LPOLCF;
	UInt8 rfu4[1];
	volatile UInt8 LACDRC;
	UInt8 rfu5[1];
	volatile UInt8 LPXCD;
	UInt8 rfu6[1];
	volatile UInt8 LCKCON;
	UInt8 rfu7[1];
	volatile UInt8 LLBAR;
	UInt8 rfu8[1];
	volatile UInt8 LOTCR;
	UInt8 rfu9[1];
	volatile UInt8 LPOSR;
	UInt8 rfuA[3];
	volatile UInt8 LFRCM;
	volatile UInt16 LGPMR;
};

static const RectangleType imageRect = {.topLeft = {.x = 1, .y = 16}, .extent = {.x = 64, .y = 64, }};
/*
	There is black magic in play here, tread with caution...
*/
static void drawMagicandTrackPenRelease(Int16 x, Int16 y)
{
	struct m68kLCDC *LCDC = (struct m68kLCDC*)0xfffffa00;
	UInt8 prevVPW, prevLBAR, prevLPICF;
	UInt8 *restrict newSSA = NULL;
	Boolean down, success = false;
	SharedVariables *sharedVars;
	UInt32 prevSSA, romVersion;
	MemHandle imgMemHandle;
	DmOpenRef dbRef;
	UInt16 i, r, c;

	//this hackery is only for OS 1 & 2, which lack greyscale mode!
	if (errNone == FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion) && romVersion >= sysMakeROMVersion(3,0,0,sysROMStageDevelopment,0))
		return;

	if (errNone != FtrGet(appFileCreator, ftrShrdVarsNum, (UInt32*)&sharedVars))
		return;

	dbRef = DmOpenDatabaseByTypeCreator('pSPR', 'PKSP', dmModeReadOnly);
	if (dbRef) {

		imgMemHandle = DmGet1Resource('pSPT', sharedVars->selectedPkmnId);
		if (imgMemHandle) {

			MemHandle mh = DmNewHandle((DmOpenRef)(SysCurAppInfoPV20()->dmAccessP), 160u * 160u * 2u / 8u);
			if (mh)
				newSSA = MemHandleLock(mh);
			if (newSSA) {

				struct DrawState *ds = NULL;

				MemSemaphoreReserve(true);
				if (imgDecode(&ds, MemHandleLock(imgMemHandle), MemHandleSize(imgMemHandle), 64, 64, 2)) {

					static const UInt16 expandTab[] = {0x0000, 0x0005, 0x000a, 0x000f, 0x0050, 0x0055, 0x005a, 0x005f, 0x00a0, 0x00a5, 0x00aa, 0x00af, 0x00f0, 0x00f5, 0x00fa, 0x00ff, 0x0500, 0x0505, 0x050a, 0x050f, 0x0550, 0x0555, 0x055a, 0x055f, 0x05a0, 0x05a5, 0x05aa, 0x05af, 0x05f0, 0x05f5, 0x05fa, 0x05ff, 0x0a00, 0x0a05, 0x0a0a, 0x0a0f, 0x0a50, 0x0a55, 0x0a5a, 0x0a5f, 0x0aa0, 0x0aa5, 0x0aaa, 0x0aaf, 0x0af0, 0x0af5, 0x0afa, 0x0aff, 0x0f00, 0x0f05, 0x0f0a, 0x0f0f, 0x0f50, 0x0f55, 0x0f5a, 0x0f5f, 0x0fa0, 0x0fa5, 0x0faa, 0x0faf, 0x0ff0, 0x0ff5, 0x0ffa, 0x0fff, 0x5000, 0x5005, 0x500a, 0x500f, 0x5050, 0x5055, 0x505a, 0x505f, 0x50a0, 0x50a5, 0x50aa, 0x50af, 0x50f0, 0x50f5, 0x50fa, 0x50ff, 0x5500, 0x5505, 0x550a, 0x550f, 0x5550, 0x5555, 0x555a, 0x555f, 0x55a0, 0x55a5, 0x55aa, 0x55af, 0x55f0, 0x55f5, 0x55fa, 0x55ff, 0x5a00, 0x5a05, 0x5a0a, 0x5a0f, 0x5a50, 0x5a55, 0x5a5a, 0x5a5f, 0x5aa0, 0x5aa5, 0x5aaa, 0x5aaf, 0x5af0, 0x5af5, 0x5afa, 0x5aff, 0x5f00, 0x5f05, 0x5f0a, 0x5f0f, 0x5f50, 0x5f55, 0x5f5a, 0x5f5f, 0x5fa0, 0x5fa5, 0x5faa, 0x5faf, 0x5ff0, 0x5ff5, 0x5ffa, 0x5fff, 0xa000, 0xa005, 0xa00a, 0xa00f, 0xa050, 0xa055, 0xa05a, 0xa05f, 0xa0a0, 0xa0a5, 0xa0aa, 0xa0af, 0xa0f0, 0xa0f5, 0xa0fa, 0xa0ff, 0xa500, 0xa505, 0xa50a, 0xa50f, 0xa550, 0xa555, 0xa55a, 0xa55f, 0xa5a0, 0xa5a5, 0xa5aa, 0xa5af, 0xa5f0, 0xa5f5, 0xa5fa, 0xa5ff, 0xaa00, 0xaa05, 0xaa0a, 0xaa0f, 0xaa50, 0xaa55, 0xaa5a, 0xaa5f, 0xaaa0, 0xaaa5, 0xaaaa, 0xaaaf, 0xaaf0, 0xaaf5, 0xaafa, 0xaaff, 0xaf00, 0xaf05, 0xaf0a, 0xaf0f, 0xaf50, 0xaf55, 0xaf5a, 0xaf5f, 0xafa0, 0xafa5, 0xafaa, 0xafaf, 0xaff0, 0xaff5, 0xaffa, 0xafff, 0xf000, 0xf005, 0xf00a, 0xf00f, 0xf050, 0xf055, 0xf05a, 0xf05f, 0xf0a0, 0xf0a5, 0xf0aa, 0xf0af, 0xf0f0, 0xf0f5, 0xf0fa, 0xf0ff, 0xf500, 0xf505, 0xf50a, 0xf50f, 0xf550, 0xf555, 0xf55a, 0xf55f, 0xf5a0, 0xf5a5, 0xf5aa, 0xf5af, 0xf5f0, 0xf5f5, 0xf5fa, 0xf5ff, 0xfa00, 0xfa05, 0xfa0a, 0xfa0f, 0xfa50, 0xfa55, 0xfa5a, 0xfa5f, 0xfaa0, 0xfaa5, 0xfaaa, 0xfaaf, 0xfaf0, 0xfaf5, 0xfafa, 0xfaff, 0xff00, 0xff05, 0xff0a, 0xff0f, 0xff50, 0xff55, 0xff5a, 0xff5f, 0xffa0, 0xffa5, 0xffaa, 0xffaf, 0xfff0, 0xfff5, 0xfffa, 0xffff, };
					const volatile UInt8 *src = imgGetBits(ds);
					UInt16 *dst = (UInt16*)newSSA;

					MemSet(newSSA, 160u * 160u * 2u / 8u, 0);

					dst += 160 * 16 * 2 / 16;
					for (r = 0; r < 128; r++) {

						dst += 2;
						for (c = 0; c < 128 / 8; c++) {

							*dst++ = expandTab[src[c]];
						}
						dst += 2;
						if (r & 1)
							src += 64 * 2 / 8;
					}

					prevSSA = LCDC->LSSA;
					LCDC->LSSA = (UInt32)newSSA;
					prevVPW = LCDC->LVPW;
					prevLBAR = LCDC->LLBAR;
					LCDC->LLBAR = 20;
					LCDC->LVPW = 20;
					prevLPICF = LCDC->LPICF;
					LCDC->LPICF |= 1;

					success = true;
					imgDrawStateFree(ds);
				}
				MemSemaphoreRelease(true);
				MemHandleUnlock(imgMemHandle);
				DmReleaseResource(imgMemHandle);
			}
		}
		DmCloseDatabase(dbRef);
	}

	if (success) {

		do {
			EvtGetPen(&x, &y, &down);
		} while (down);
		LCDC->LPICF = prevLPICF;
		LCDC->LLBAR = prevLBAR;
		LCDC->LVPW = prevVPW;
		LCDC->LSSA = prevSSA;

		FrmDrawForm(FrmGetActiveForm());
		DrawPkmnSprite(sharedVars->selectedPkmnId);
	}
	if (newSSA)
		MemPtrFree(newSSA);
}

/*
 * FUNCTION: PkmnMainFormHandleEvent
 *
 * DESCRIPTION:
 *
 * This routine is the event handler for the "PkmnMainForm" of this
 * application.
 *
 * PARAMETERS:
 *
 * eventP
 *     a pointer to an EventType structure
 *
 * RETURNED:
 *     true if the event was handled and should not be passed to
 *     FrmHandleEvent
 */

Boolean PkmnMainFormHandleEvent(EventType *eventP)
{
	Boolean handled = false;
	FormType *frmP;

	switch (eventP->eType)
	{
	case penDownEvent:
		if (RctPtInRectangle(eventP->screenX, eventP->screenY, &imageRect)) {
			drawMagicandTrackPenRelease(eventP->screenX, eventP->screenY);
			return true;
		}
		break;

	case ctlSelectEvent:
		return PkmnMainFormDoCommand(eventP->data.menu.itemID);

	case frmOpenEvent:
		frmP = FrmGetActiveForm();
		FrmDrawForm(frmP);
		LoadPkmnStats();
		handled = true;
		break;

	case keyDownEvent:
		if (eventP->data.keyDown.chr == vchrPageUp)
		{
			PkmnDescriptionScroll(winUp);
			handled = true;
		}
		else if (eventP->data.keyDown.chr == vchrPageDown)
		{
			PkmnDescriptionScroll(winDown);
			handled = true;
		}
		break;

	case popSelectEvent:
		if (eventP->data.popSelect.selection == 1)
		{
			FrmGotoForm(PkmnTypeForm);
		}
		break;

	case frmCloseEvent:
		//no matter why we're closing, free things we allocated
		unregisterCurrentPng();
		FreeDescriptionField();
		break;

	default:
		break;
	}

	return handled;
}
