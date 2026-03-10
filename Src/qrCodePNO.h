#ifndef _QR_CODE_PNO_H_
#define _QR_CODE_PNO_H_

#include "qrcode.h"

struct QrCodeInitBytesParams {
	QRCode *qrcode;
	uint32_t length;
	const uint8_t *data;
	uint8_t *modules;
	uint8_t version;
	uint8_t ecc;
};



#endif