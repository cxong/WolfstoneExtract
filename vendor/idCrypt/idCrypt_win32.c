// idCrypt v1.0 by emoose
// Code licensed under GPL 3.0.

#include <Windows.h>

#include <bcrypt.h>
#include <ncrypt.h>

#define NT_SUCCESS(Status)          (((NTSTATUS)(Status)) >= 0)

long crypt_data(bool decrypt, void* pbInput, int cbInput, void* pbEncKey, int cbEncKey, void* pbIV, int cbIV, void* pbOutput, unsigned long* cbOutput)
{
	BCRYPT_ALG_HANDLE hashAlg;
	NTSTATUS res = ERROR_SUCCESS;
	DWORD unk = 0;

	if (!NT_SUCCESS(res = BCryptOpenAlgorithmProvider(&hashAlg, BCRYPT_AES_ALGORITHM, NULL, 0)))
		return res;

	DWORD blockSize = 0;
	if (!NT_SUCCESS(res = BCryptGetProperty(hashAlg, BCRYPT_BLOCK_LENGTH, (PBYTE)&blockSize, sizeof(DWORD), &unk, 0)))
		return res;

	BCRYPT_KEY_HANDLE hKey = NULL;
	DWORD keySize = 0;
	if (!NT_SUCCESS(res = BCryptGetProperty(hashAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&keySize, sizeof(DWORD), &unk, 0)))
		return res;

	BYTE* key = (BYTE*)malloc(keySize);

	if (!NT_SUCCESS(res = BCryptGenerateSymmetricKey(hashAlg, &hKey, key, keySize, (PBYTE)pbEncKey, cbEncKey, 0)))
		return res;

	if(decrypt)
		res = BCryptDecrypt(hKey, (PBYTE)pbInput, cbInput, 0, (PBYTE)pbIV, cbIV, (PBYTE)pbOutput, *cbOutput, cbOutput, BCRYPT_BLOCK_PADDING);
	else
		res = BCryptEncrypt(hKey, (PBYTE)pbInput, cbInput, 0, (PBYTE)pbIV, cbIV, (PBYTE)pbOutput, *cbOutput, cbOutput, BCRYPT_BLOCK_PADDING);

	BCryptDestroyKey(hKey);
	BCryptCloseAlgorithmProvider(hashAlg, 0);

	free(key);

	return res;
}

long hash_data(const void* pbBuf1, int cbBuf1, const void* pbBuf2, int cbBuf2, const void* pbBuf3, int cbBuf3, void* pbSecret, int cbSecret, void* output)
{
	BCRYPT_ALG_HANDLE hashAlg;
	NTSTATUS res = ERROR_SUCCESS;
	DWORD unk = 0;

	if (!NT_SUCCESS(res = BCryptOpenAlgorithmProvider(&hashAlg, BCRYPT_SHA256_ALGORITHM, NULL, pbSecret ? BCRYPT_ALG_HANDLE_HMAC_FLAG : 0)))
		return res;

	DWORD hashObjectSize = 0;
	if (!NT_SUCCESS(res = BCryptGetProperty(hashAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&hashObjectSize, sizeof(DWORD), &unk, 0)))
		return res;

	PBYTE hashObject = (PBYTE)HeapAlloc(GetProcessHeap(), 0, hashObjectSize);
	if (!hashObject)
		return -1;

	DWORD hashSize = 0;
	if (!NT_SUCCESS(res = BCryptGetProperty(hashAlg, BCRYPT_HASH_LENGTH, (PBYTE)&hashSize, sizeof(DWORD), &unk, 0)))
		return res;

	BCRYPT_HASH_HANDLE hashHandle;
	if (!NT_SUCCESS(res = BCryptCreateHash(hashAlg, &hashHandle, hashObject, hashObjectSize, (PBYTE)pbSecret, cbSecret, 0)))
		return res;

	if(pbBuf1)
		if (!NT_SUCCESS(res = BCryptHashData(hashHandle, (PBYTE)pbBuf1, cbBuf1, 0)))
			return res;

	if (pbBuf2)
		if (!NT_SUCCESS(res = BCryptHashData(hashHandle, (PBYTE)pbBuf2, cbBuf2, 0)))
			return res;

	if (pbBuf3)
		if (!NT_SUCCESS(res = BCryptHashData(hashHandle, (PBYTE)pbBuf3, cbBuf3, 0)))
			return res;

	res = BCryptFinishHash(hashHandle, (PBYTE)output, hashSize, 0);
	BCryptCloseAlgorithmProvider(hashAlg, 0);

	HeapFree(GetProcessHeap(), 0, hashObject);

	return res;
}

void gen_random(uint8_t* buffer, unsigned long size)
{
	BCryptGenRandom(NULL, buffer, size, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
}
