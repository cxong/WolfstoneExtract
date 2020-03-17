// idCrypt v1.0 by emoose
// Code licensed under GPL 3.0.

#include "idCrypt.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern long hash_data(const void* pbBuf1, int cbBuf1, const void* pbBuf2, int cbBuf2, const void* pbBuf3, int cbBuf3, void* pbSecret, int cbSecret, void* output);
extern long crypt_data(bool decrypt, void* pbInput, int cbInput, void* pbEncKey, int cbEncKey, void* pbIV, int cbIV, void* pbOutput, unsigned long* cbOutput);
extern void gen_random(uint8_t* buffer, unsigned long size);

#define NT_SUCCESS(Status)          (Status >= 0)

// static string used during key derivation
const char* keyDeriveStatic = "swapTeam\n";

int idCrypt(idCrypt_t* state, uint8_t* fileData, long size, const char* internalPath, bool decrypt)
{
	// In case we return early ensure that our variables are in a defined state.
	memset(state, 0, sizeof(idCrypt_t));

	if (decrypt)
		memcpy(state->fileSalt, fileData, 0xC);
	else
		gen_random(state->fileSalt, 0xC);

	int res = hash_data((void*)state->fileSalt, 0xC, (void*)keyDeriveStatic, 0xA, internalPath, strlen(internalPath), NULL, 0, state->encKey);
	if (!NT_SUCCESS(res))
	{
		state->errstatus = res;
		return EC_EncKeyFailed;
	}

	if (decrypt)
		memcpy(state->fileIV, fileData + 0xC, 0x10);
	else
		gen_random(state->fileIV, 0x10);

	uint8_t fileIV_backup[0x10];
	memcpy(fileIV_backup, state->fileIV, 0x10); // make a backup of IV because BCrypt can overwrite it (and make you waste an hour debugging in the process...)

	uint8_t* fileText = fileData;
	long fileTextSize = size;

	if (decrypt) // change fileText pointer + verify HMAC if we're decrypting
	{
		fileText = fileData + 0x1C;
		fileTextSize = size - 0x1C - 0x20;

		uint8_t* fileHmac = fileData + (size - 0x20);

		res = hash_data((void*)state->fileSalt, 0xC, (void*)state->fileIV, 0x10, (void*)fileText, fileTextSize, state->encKey, 0x20, state->hmac);
		if (!NT_SUCCESS(res))
		{
			state->errstatus = res;
			return EC_HmacFailed;
		}

		if (memcmp(state->hmac, fileHmac, 0x20))
			state->badhmac = true;
	}

	// call crypt_data with NULL buffer to get the buffer size
	res = crypt_data(decrypt, fileText, fileTextSize, state->encKey, 0x10, state->fileIV, 0x10, 0, &state->cryptedTextSize);
	if (!NT_SUCCESS(res))
	{
		state->errstatus = res;
		return EC_CryptFailed;
	}

	memcpy(state->fileIV, fileIV_backup, 0x10); // BCryptEncrypt overwrites the IV, so restore it from backup

	// now allocate that buffer and call crypt_data for realsies
	state->cryptedText = (uint8_t*)malloc(state->cryptedTextSize);
	res = crypt_data(decrypt, fileText, fileTextSize, state->encKey, 0x10, state->fileIV, 0x10, state->cryptedText, &state->cryptedTextSize);
	if (!NT_SUCCESS(res))
	{
		state->errstatus = res;
		return EC_CryptFailed;
	}

	memcpy(state->fileIV, fileIV_backup, 0x10); // BCryptEncrypt overwrites the IV, so restore it from backup

	if (!decrypt)
	{
		// Calculate hmac
		res = hash_data((void*)state->fileSalt, 0xC, (void*)state->fileIV, 0x10, (void*)state->cryptedText, state->cryptedTextSize, state->encKey, 0x20, state->hmac);
		if (!NT_SUCCESS(res))
		{
			state->errstatus = res;
			return EC_HmacFailed;
		}
	}

	return 0;
}
