/*
** file_idcl.cpp
**
**---------------------------------------------------------------------------
** Copyright 2019 Braden Obrzut
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
** A not so serious Wolfenstein II resource file loader.  All we care about is
** getting the Wolfstone files out of it, so don't expect this code to be
** complete!
**
*/

#include "doomerrors.h"
#include "resourcefile.h"

#include <algorithm>
#include <memory>

#include <kraken.h>

#define Printf printf

#pragma pack(push, 1)
namespace IDCL
{
	struct B
	{
		struct FIdclHeader
		{
			char magic[4];
			uint32_t version;
			char pad[24];
			uint32_t files;
			char pas2[4];
			uint32_t ids;
			char pad3[20];
			uint64_t stringTableOffset;
			char pad4[8];
			uint64_t dirOffset;
			char pad5[8];
			uint64_t idOffset;
			uint64_t dataOffset;
		};

		struct FIdclEntry
		{
			char pad[32];
			uint64_t name;
			char pad2[16];
			uint64_t offset;
			uint64_t compressedSize;
			uint64_t size;
			char pad3[24];
			uint32_t typeIndicator; // Don't know, but seems to tell something about the file encoding
			char pad4[36];
		};

		struct FIdclStringRef
		{
			uint64_t type;
			uint64_t name;
		};
	};

	struct C : B
	{
		struct FIdclHeader
		{
			char magic[4];
			uint32_t version;
			char pad[32];
			uint32_t files;
			char pas2[4];
			uint32_t ids;
			char pad3[20];
			uint64_t stringTableOffset;
			char pad4[8];
			uint64_t dirOffset;
			char pad5[8];
			uint64_t idOffset;
			uint64_t dataOffset;
		};
	};
}
#pragma pack(pop)

struct FIdclLump : public FResourceLump
{
	int64_t CompressedSize;
	int64_t Position;

	FileReader *GetReader()
	{
		// Uncompressed?
		if(CompressedSize == LumpSize)
		{
			Owner->Reader->Seek(Position, SEEK_SET);
			return Owner->Reader;
		}
		return NULL;
	}

	int FillCache()
	{
		if(CompressedSize == LumpSize)
		{
			if(const char* buffer = Owner->Reader->GetBuffer())
			{
				// File already in memory
				Cache = const_cast<char*>(buffer) + Position;
				RefCount = -1;
				return -1;
			}
		}

		Owner->Reader->Seek(Position, SEEK_SET);
		Cache = new char[LumpSize];
		if(CompressedSize == LumpSize)
			Owner->Reader->Read(Cache, LumpSize);
		else
		{
			auto cdata = std::make_unique<uint8_t[]>(CompressedSize);
			Owner->Reader->Read(cdata.get(), CompressedSize);
			if(Kraken_Decompress(cdata.get(), CompressedSize, (uint8_t*)Cache, LumpSize) < 0)
				throw CFatalError("Kraken decompression failed.");
		}

		RefCount = 1;
		return 1;
	}
};

template<typename Ver>
class FIdclFile : public FResourceFile
{
	std::unique_ptr<FIdclLump[]> Lumps;

public:
	FIdclFile(const char * filename, FileReader *file);
	FResourceLump *GetLump(int num) { return &Lumps[num]; }
	bool Open(bool quiet);
};

template<typename Ver>
FIdclFile<Ver>::FIdclFile(const char *filename, FileReader *file) : FResourceFile(filename, file)
{
	NumLumps = 0;
	Lumps = NULL;
}

template<typename Ver>
bool FIdclFile<Ver>::Open(bool quiet)
{
	typename Ver::FIdclHeader header;

	Reader->Read(&header, sizeof(header));
	header.files = LittleLong(header.files);
	header.ids = LittleLong(header.ids);
	header.stringTableOffset = LittleLongLong(header.stringTableOffset);
	header.idOffset = LittleLongLong(header.idOffset);
	header.dirOffset = LittleLongLong(header.dirOffset);

	// Read string table directory
	uint64_t numStringTableEntries;
	std::unique_ptr<uint64_t[]> stringOffsets;
	Reader->Seek(header.stringTableOffset, SEEK_SET);
	Reader->Read(&numStringTableEntries, sizeof(numStringTableEntries));
	numStringTableEntries = LittleLongLong(numStringTableEntries);
	stringOffsets = std::make_unique<uint64_t[]>(numStringTableEntries);
	Reader->Read(stringOffsets.get(), numStringTableEntries*sizeof(uint64_t));

	// Read string table
	uint64_t strBufLen = 0;
	for(uint64_t i = 0;i < numStringTableEntries;++i)
		strBufLen = std::max(strBufLen, stringOffsets[i]);
	strBufLen += 1024; // Read enough extra to hopefully get the last string
	auto stringTableBuffer = std::make_unique<char[]>(strBufLen);
	strBufLen = Reader->Read(stringTableBuffer.get(), strBufLen);

	auto stringTable = std::make_unique<FString[]>(numStringTableEntries);
	for(uint64_t i = 0;i < numStringTableEntries;++i)
		stringTable[i] = FString(&stringTableBuffer[stringOffsets[i]]);

	// Read file ids
	auto fileIds = std::make_unique<typename Ver::FIdclStringRef[]>(header.files);
	Reader->Seek(header.idOffset + header.ids*4, SEEK_SET);
	Reader->Read(fileIds.get(), header.files*sizeof(typename Ver::FIdclStringRef));

	// Read directory
	auto dirEntries = std::make_unique<typename Ver::FIdclEntry[]>(header.files);
	Reader->Seek(header.dirOffset, SEEK_SET);
	Reader->Read(dirEntries.get(), header.files*sizeof(typename Ver::FIdclEntry));

	Lumps = std::make_unique<FIdclLump[]>(header.files+1);
	for(uint64_t i = 0;i < header.files;++i)
	{
		fileIds[i].type = LittleLongLong(fileIds[i].type);
		fileIds[i].name = LittleLongLong(fileIds[i].name);

		dirEntries[i].name = LittleLongLong(dirEntries[i].name);
		dirEntries[i].offset = LittleLongLong(dirEntries[i].offset);
		dirEntries[i].compressedSize = LittleLongLong(dirEntries[i].compressedSize);
		dirEntries[i].size = LittleLongLong(dirEntries[i].size);
		dirEntries[i].typeIndicator = LittleLong(dirEntries[i].typeIndicator);

		// Seemingly uncompressed entry could be compressed if typeIndicator is
		// 4, but the sizes will be in the header of the data.
		if(dirEntries[i].typeIndicator == 4 && dirEntries[i].compressedSize == dirEntries[i].size)
		{
			Reader->Seek(dirEntries[i].offset, SEEK_SET);
			Reader->Read(&dirEntries[i].size, 8);
			Reader->Read(&dirEntries[i].compressedSize, 8);

			dirEntries[i].offset += 16;
			dirEntries[i].compressedSize = LittleLongLong(dirEntries[i].compressedSize);
			dirEntries[i].size = LittleLongLong(dirEntries[i].size);

			// compressedSize of -1 means uncompressed
			if(dirEntries[i].compressedSize == std::numeric_limits<uint64_t>::max())
				dirEntries[i].compressedSize = dirEntries[i].size;
		}

		FString name = stringTable[fileIds[i].name];
		if(name.Right(4).Compare(".wl6") == 0 ||
			name.Right(19).Compare("sb_vo_wolfstone.bnk") == 0 ||
			name.Right(16).Compare("sb_wolfstone.bnk") == 0 ||
			name.Right(13).Compare("wolfstone.bnk") == 0 ||
			name.Left(8).Compare("strings/") == 0)
		{
			FIdclLump &lump = Lumps[NumLumps++];

			// Get rid of the path since we're only loading the embedded Wolf3D data
			FString realName = name.Mid(name.LastIndexOf('/')+1);
			if(name.Left(8).Compare("strings/") == 0)
				realName = name;
			lump.LumpNameSetup(realName);
			lump.Owner = this;
			lump.LumpSize = dirEntries[i].size;
			lump.CompressedSize = dirEntries[i].compressedSize;
			lump.Position = dirEntries[i].offset;
		}
	}

	if (!quiet) Printf(", %d lumps\n", NumLumps);

	return true;
}

FResourceFile *CheckIdcl(const char *filename, FileReader *file, bool quiet)
{
	if (file->GetLength() >= 8)
	{
		char magic[8];
		file->Seek(0, SEEK_SET);
		file->Read(&magic, 8);
		file->Seek(0, SEEK_SET);

		// The B version seems to indicate alternate translation? All the
		// English files and standard resources are C.
		if(!memcmp(magic, "IDCL\xC\x0\x0\x0", 8) || !memcmp(magic, "IDCL\xD\x0\x0\x0", 8))
		{
			FResourceFile *rf = new FIdclFile<IDCL::C>(filename, file);
			if(rf->Open(quiet)) return rf;
			rf->Reader = NULL; // to avoid destruction of reader
			delete rf;
		}
		else if(!memcmp(magic, "IDCL\xB\x0\x0\x0", 8))
		{
			FResourceFile *rf = new FIdclFile<IDCL::B>(filename, file);
			if(rf->Open(quiet)) return rf;
			rf->Reader = NULL; // to avoid destruction of reader
			delete rf;
		}
	}
	return NULL;
}
