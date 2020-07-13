// Linux port for idCrypt by Braden "Blzut3" Obrzut
// Code licensed under GPL 3.0.

#include <errno.h>
#include <linux/if_alg.h>
#include <linux/netlink.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <unistd.h>

#define CBIV_MAX 16

// Because I don't feel like linking to libm for one function
#define min(a,b) ((a) < (b) ? (a) : (b))

long crypt_data(bool decrypt, void* pbInput, int cbInput, void* pbEncKey, int cbEncKey, void* pbIV, int cbIV, void* pbOutput, unsigned long* cbOutput)
{
	// AES-CBC produces the same amount of output as input, so a buffer the same
	// size as the input is good.
	if(*cbOutput == 0)
	{
		*cbOutput = cbInput;
		return 0;
	}

	if(cbIV > CBIV_MAX)
		return -1;

	int algsock = socket(AF_ALG, SOCK_SEQPACKET, 0);
	if(algsock < 0)
		return errno;

	struct sockaddr_alg alg = {.salg_family = AF_ALG, .salg_type = "skcipher", .salg_name = "cbc(aes)" };
	if(bind(algsock, (struct sockaddr*)&alg, sizeof(alg)) < 0)
	{
		close(algsock);
		return errno;
	}

	if(setsockopt(algsock, SOL_ALG, ALG_SET_KEY, pbEncKey, cbEncKey) < 0)
	{
		close(algsock);
		return errno;
	}

	int sock = accept(algsock, NULL, 0);

	char buffer[CMSG_SPACE(4) + CMSG_SPACE(sizeof(struct af_alg_iv) + CBIV_MAX)] = {};
	struct iovec iov = {
		.iov_base = pbInput,
		.iov_len = cbInput
	};
	struct msghdr msg = {
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = buffer,
		.msg_controllen = CMSG_SPACE(4) + CMSG_SPACE(sizeof(struct af_alg_iv) + cbIV)
	};

	struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_ALG;
	cmsg->cmsg_type = ALG_SET_OP;
	cmsg->cmsg_len = CMSG_LEN(4);
	*(uint32_t*)CMSG_DATA(cmsg) = decrypt ? ALG_OP_DECRYPT : ALG_OP_ENCRYPT;

	cmsg = CMSG_NXTHDR(&msg, cmsg);
	cmsg->cmsg_level = SOL_ALG;
	cmsg->cmsg_type = ALG_SET_IV;
	cmsg->cmsg_len = CMSG_LEN(sizeof(struct af_alg_iv) + cbIV);
	struct af_alg_iv* iv = (struct af_alg_iv*)CMSG_DATA(cmsg);
	memcpy(iv->iv, pbIV, cbIV);
	iv->ivlen = cbIV;

	// How much should we send at once?
	uint32_t chunkSize;
	socklen_t rcvbufBytes = 4;
	if(getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &chunkSize, &rcvbufBytes) != 0)
		chunkSize = 4096; // Some small value should work.

	uint8_t* out = pbOutput;
	unsigned long outlen = *cbOutput;
	while(cbInput > 0)
	{
		iov.iov_len = min(cbInput, chunkSize);

		ssize_t bytes = sendmsg(sock, &msg, (cbInput > chunkSize ? MSG_MORE : 0));
		if(bytes < 0)
		{
			close(sock);
			close(algsock);
			return errno;
		}

		cbInput -= bytes;
		iov.iov_base += bytes;

		// We don't need to send any control messages on subsequent rounds.
		msg.msg_controllen = 0;

		while((bytes = recv(sock, out, outlen, MSG_DONTWAIT)) > 0)
		{
			out += bytes;
			outlen -= bytes;
		}
	}

	// Check for padding at the end.  This is done by looking at the last byte
	// which should be blocksize-bytesleft.  (AES-CBC uses 16 byte blocks.)
	// I guess we just hope that there's not a complete final block with a 15
	// as the last byte?
	uint8_t padding = ((uint8_t*)pbOutput)[*cbOutput-1];
	if(padding > 16)
		return -1;
	*cbOutput -= padding;
	
	close(sock);
	close(algsock);

	return 0;
}

long hash_data(const void* pbBuf1, int cbBuf1, const void* pbBuf2, int cbBuf2, const void* pbBuf3, int cbBuf3, void* pbSecret, int cbSecret, void* output)
{
	int algsock = socket(AF_ALG, SOCK_SEQPACKET, 0);
	if(algsock < 0)
		return errno;

	struct sockaddr_alg alg = {.salg_family = AF_ALG, .salg_type = "hash" };
	strcpy(alg.salg_name, pbSecret ? "hmac(sha256)" : "sha256");
	if(bind(algsock, (struct sockaddr*)&alg, sizeof(alg)) < 0)
	{
		close(algsock);
		return errno;
	}

	if(pbSecret)
	{
		if(setsockopt(algsock, SOL_ALG, ALG_SET_KEY, pbSecret, cbSecret) < 0)
		{
			close(algsock);
			return errno;
		}
	}

	int sock = accept(algsock, NULL, 0);

	if(pbBuf1 && send(sock, pbBuf1, cbBuf1, MSG_MORE) != cbBuf1)
	{
		close(sock);
		close(algsock);
		return errno;
	}
	if(pbBuf2 && send(sock, pbBuf2, cbBuf2, MSG_MORE) != cbBuf2)
	{
		close(sock);
		close(algsock);
		return errno;
	}
	if(pbBuf3 && send(sock, pbBuf3, cbBuf3, MSG_MORE) != cbBuf3)
	{
		close(sock);
		close(algsock);
		return errno;
	}

	// Finish digest
	if(send(sock, NULL, 0, 0) != 0)
	{
		close(sock);
		close(algsock);
		return errno;
	}

	ssize_t ret = recv(sock, output, 32, 0);

	close(sock);
	close(algsock);

	if(ret < 0)
		return errno;
	if(ret != 32)
		return 1;

	return 0;
}

void gen_random(uint8_t* buffer, unsigned long size)
{
	ssize_t bytes;
	while((bytes = getrandom(buffer, size, 0)) < size)
	{
		if(bytes < 0)
			return;

		size -= bytes;
	}
}
