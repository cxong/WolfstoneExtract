#include "idCrypt.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _MSC_VER
#include <errno.h>

#define strcpy_s(a,b,c) strcpy((a),(c))
#define sprintf_s(a,b,...) sprintf((a),__VA_ARGS__)

static int fopen_s(FILE* restrict * restrict streamptr, const char* restrict filename, const char* restrict mode)
{
	if((*streamptr = fopen(filename, mode)))
		return 0;
	return errno;
}
#endif

int main(int argc, char *argv[])
{
	printf("idCrypt v1.0 - by emoose\n\n");

	if (argc < 3)
	{
		printf("Usage:\n\tidCrypt.exe <file-path> <internal-file-path>\n\n");
		printf("Example:\n\tidCrypt.exe D:\\english.bfile strings/english.lang\n\n");
		printf("If a .bfile is supplied it'll be decrypted to <file-path>.dec\n");
		printf("Otherwise the file will be encrypted to <file-path>.bfile\n\n");
		printf("You _must_ use the correct internal filepath for decryption to succeed!\n");
		return 1;
	}
	bool decrypt = false;

	char* filePath = argv[1];
	char* internalPath = argv[2];
	char* dot = strrchr(filePath, '.');

	// copy extension and make it lowercase, if it's a .bfile we'll switch to decryption mode
	if (dot)
	{
		char lowerExt[256];
		strcpy_s(lowerExt, 256, dot);

		for (int i = 0; lowerExt[i]; i++)
			lowerExt[i] = tolower(lowerExt[i]);

		decrypt = !strcmp(lowerExt, ".bfile") || !strcmp(lowerExt, ".bfile;binaryfile");
	}

	char destPath[256];
	sprintf_s(destPath, 256, "%s.%s", filePath, decrypt ? "dec" : "bfile");

	FILE* file;
	int res = 0;
	if (res = fopen_s(&file, filePath, "rb") != 0)
	{
		printf("Failed to open %s for reading (error %d)\n", filePath, res);
		return 2;
	}

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	uint8_t* fileData = (uint8_t*)malloc(size);
	if(fread(fileData, 1, size, file) != size)
	{
		printf("Failed to read %ld bytes from file\n", size);
		return 2;
	}
	fclose(file);

	idCrypt_t state;
	res = idCrypt(&state, fileData, size, internalPath, decrypt);
	if(res != 0)
	{
		switch(res)
		{
			case EC_EncKeyFailed:
				printf("Failed to derive encryption key (error 0x%x)\n", state.errstatus);
				break;
			case EC_HmacFailed:
				printf("Failed to create HMAC hash of ciphertext (error 0x%x)\n", state.errstatus);
				break;
			case EC_CryptFailed:
				printf("Failed to %s data (error 0x%x)\n", decrypt ? "decrypt" : "encrypt", state.errstatus);
				printf("Did you use the correct internal file name?\n");
				break;
			default:
				break;
		}
		return res;
	}

	if(state.badhmac)
		printf("Warning: HMAC hash check failed, decrypted data might not be valid!\n");

	free(fileData);
	
	res = 0;
	if (res = fopen_s(&file, destPath, "wb+") != 0)
	{
		printf("Failed to open %s for writing (error %d)\n", destPath, res);
		return 6;
	}

	if(decrypt)
		fwrite(state.cryptedText, 1, state.cryptedTextSize, file);
	else
	{
		fwrite(state.fileSalt, 1, 0xC, file);
		fwrite(state.fileIV, 1, 0x10, file);
		fwrite(state.cryptedText, 1, state.cryptedTextSize, file);
		fwrite(state.hmac, 1, 0x20, file);
	}

	fclose(file);

	//free(cryptedText);
	// ^ causes an error, wtf?

	printf("%s succeeded! Wrote to %s\n", decrypt ? "Decryption" : "Encryption", destPath);	
	return 0;
}
