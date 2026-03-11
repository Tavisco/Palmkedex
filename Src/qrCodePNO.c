#include "qrCodePNO.h"
#include "pnoRuntime.h"
#include "qrcode.h"



int8_t __attribute__((used)) ArmletMain(void *emulStateP, struct QrCodeInitBytesParams *pp, void *call68kFuncPtr);
int8_t __attribute__((used)) ArmletMain(void *emulStateP, struct QrCodeInitBytesParams *pp, void *call68kFuncPtr)
{
	const uint8_t *data = (const uint8_t*)read32(&pp->data);
	uint8_t *modules = (uint8_t*)read32(&pp->modules);
	QRCode *qrcode = (QRCode*)read32(&pp->qrcode);
	uint32_t length = read32(&pp->length);

	armCallsInit(emulStateP, call68kFuncPtr);
	return qrcode_initBytesEx(qrcode, modules, pp->version, pp->ecc, data, length);
}
