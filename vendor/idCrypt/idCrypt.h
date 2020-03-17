// idCrypt v1.0 by emoose
// Code licensed under GPL 3.0.

#ifndef idCrypt_h
#define idCrypt_h

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
	EC_None = 0,
	EC_EncKeyFailed = 3,
	EC_HmacFailed = 4,
	EC_CryptFailed = 5
};
	

typedef struct
{
	uint8_t* cryptedText; // Buffer will be malloc'd here
	unsigned long cryptedTextSize;
	uint8_t encKey[0x20];
	uint8_t hmac[0x20];
	uint8_t fileIV[0x10];
	uint8_t fileSalt[0xC];

	int errstatus;
	bool badhmac;
} idCrypt_t;

extern int idCrypt(idCrypt_t* state, uint8_t* fileData, long size, const char* internalPath, bool decrypt);

#ifdef __cplusplus
}
#endif

#endif
