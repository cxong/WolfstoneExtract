/*
** soundbank.cpp
**
**---------------------------------------------------------------------------
** Copyright 2020 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
** Loader for Wwise Soundbanks.  May be incomplete since this is for extracting
** Wolfstone sounds from Wolfenstein II.
**
*/

#include "doomerrors.h"
#include "files.h"
#include "m_swap.h"
#include "soundbank.h"
#include "tarray.h"
#include "zstring.h"

#include <cstdint>
#include <cstdio>
#include <packed_codebooks_aoTuV_603.h>
#include <sstream>
#include <wwriff.h>

constexpr uint32_t MakeId(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
	return uint32_t(a)|(uint32_t(b)<<8)|(uint32_t(c)<<16)|(uint32_t(d)<<24);
}

#pragma pack(push, 1)

struct FSbSection
{
	uint32_t Id;
	uint32_t Length;
};

struct FSbHeader
{
	constexpr static uint32_t Magic = MakeId('B','K','H','D');

	uint32_t Version;
	uint32_t SoundbankId;
	uint32_t pad[2];
};

struct FSbIndex
{
	constexpr static uint32_t Magic = MakeId('D','I','D','X');

	uint32_t WemId;
	uint32_t Offset;
	uint32_t Length;
};

struct FSbData
{
	constexpr static uint32_t Magic = MakeId('D','A','T','A');
};

#pragma pack(pop)

FSoundbank::FSoundbank(FileReader *reader)
{
	TMap<uint32_t, Entry> chunkMap;

	FSbSection section;
	uint64_t offset = 8;
	bool foundFourCC = false;
	while(reader->Read(&section, sizeof(section)) == sizeof(section))
	{
		section.Id = LittleLong(section.Id);
		section.Length = LittleLong(section.Length);

		if(!foundFourCC && section.Id != FSbHeader::Magic)
			throw CRecoverableError("Missing BKHD fourcc");
		foundFourCC = true;

		chunkMap[section.Id] = {offset, section.Length};

		offset += section.Length + sizeof(section);
		reader->Seek(section.Length, SEEK_CUR);
	}

	auto dataChunk = chunkMap.CheckKey(FSbData::Magic);
	if(!dataChunk)
		throw CRecoverableError("Could not locate sound data in bank");

	auto indexChunk = chunkMap.CheckKey(FSbIndex::Magic);
	if(!indexChunk)
		throw CRecoverableError("Could not locate bank index");

	Sounds.Resize(indexChunk->Length/sizeof(FSbIndex));
	reader->Seek(indexChunk->Offset, SEEK_SET);
	for(unsigned int i = 0; i < indexChunk->Length/sizeof(FSbIndex); ++i)
	{
		FSbIndex index;
		if(reader->Read(&index, sizeof(index)) != sizeof(index))
			throw CRecoverableError("Failed to read bank index");

		index.WemId = LittleLong(index.WemId);
		index.Offset = LittleLong(index.Offset);
		index.Length = LittleLong(index.Length);
		Sounds[i] = {dataChunk->Offset + index.Offset, index.Length};
	}

	printf("Processing sound bank ");
	for(unsigned int i = 0; i < Sounds.Size(); ++i)
	{
		printf("[%5d/%5d]\b\b\b\b\b\b\b\b\b\b\b\b\b", i+1, Sounds.Size());
		fflush(stdout);

		auto& sound = Sounds[i];

		sound.Data.Resize(sound.Length);

		reader->Seek(sound.Offset, SEEK_SET);
		if(reader->Read(&sound.Data[0], sound.Length) != sound.Length)
		{
			puts("");
			throw CRecoverableError("Failed to read sound data");
		}

		// Convert to standard Vorbis
		std::istringstream istream{{(const char*)&sound.Data[0], sound.Length}};
		Wwise_RIFF_Vorbis decoder(istream, packed_codebooks_aoTuV_603, sizeof(packed_codebooks_aoTuV_603), false, false, kNoForcePacketFormat);

		std::ostringstream ostream;
		decoder.generate_ogg(ostream);

		sound.Data.Resize(ostream.str().length());
		memcpy(&sound.Data[0], ostream.str().data(), ostream.str().length());
	}
	puts("");
}
