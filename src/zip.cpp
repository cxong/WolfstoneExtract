/*
** zip.cpp
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
**
*/

#include "m_swap.h"
#include "resourcefile.h"
#include "zip.h"

#include <cstdint>
#include <memory>
#include <tuple>

#pragma pack(push, 1)

struct FFileHeader
{
	char Magic[4] = {'P','K',3,4};
	uint8_t VersionLo = 20;
	uint8_t VersionHi = 0;
	uint16_t Flags = 0;
	uint16_t Method = 0;
	uint16_t MTime = 0;
	uint16_t MDate;
	uint32_t Crc32;
	uint32_t CSize;
	uint32_t USize;
	uint16_t FileNameLength;
	uint16_t ExtraLength = 0;
};

struct FDirectoryFile
{
	char Magic[4] = {'P','K',1,2};
	uint8_t VersionMadeLo = 20;
	uint8_t VersionMadeHi = 0;
	uint8_t VersionExtractLo = 20;
	uint8_t VersionExtractHi = 0;
	uint16_t Flags = 0;
	uint16_t Method = 0;
	uint16_t MTime = 0;
	uint16_t MDate;
	uint32_t Crc32;
	uint32_t CSize;
	uint32_t USize;
	uint16_t FileNameLength;
	uint16_t ExtraLength = 0;
	uint16_t CommentLength = 0;
	uint16_t DiskNumber = 0;
	uint16_t InternalAttrib = 0;
	uint32_t ExternalAttrib = 0;
	uint32_t Offset;
};

struct FEndCentralDirectory
{
	char Magic[4] = {'P','K',5,6};
	uint16_t DiskNumber = 0;
	uint16_t CentralDirectoryDisk = 0;
	uint16_t CurDiskEntries;
	uint16_t TotalEntries;
	uint32_t CentralDirectorySize;
	uint32_t CentralDirectoryOffset;
	uint16_t CommentLength = 0;
};

#pragma pack(pop)

static void LittleEndian(FFileHeader &head)
{
	head.Flags = LittleShort(head.Flags);
	head.Method = LittleShort(head.Method);
	head.MTime = LittleShort(head.MTime);
	head.MDate = LittleShort(head.MDate);
	head.Crc32 = LittleLong(head.Crc32);
	head.CSize = LittleLong(head.CSize);
	head.USize = LittleLong(head.USize);
	head.FileNameLength = LittleShort(head.FileNameLength);
	head.ExtraLength = LittleShort(head.ExtraLength);
}

static void LittleEndian(FDirectoryFile &entry)
{
	entry.Flags = LittleShort(entry.Flags);
	entry.Method = LittleShort(entry.Method);
	entry.MTime = LittleShort(entry.MTime);
	entry.MDate = LittleShort(entry.MDate);
	entry.Crc32 = LittleLong(entry.Crc32);
	entry.CSize = LittleLong(entry.CSize);
	entry.USize = LittleLong(entry.USize);
	entry.FileNameLength = LittleShort(entry.FileNameLength);
	entry.ExtraLength = LittleShort(entry.ExtraLength);
	entry.CommentLength = LittleShort(entry.CommentLength);
	entry.DiskNumber = LittleShort(entry.DiskNumber);
	entry.InternalAttrib = LittleShort(entry.InternalAttrib);
	entry.ExternalAttrib = LittleLong(entry.ExternalAttrib);
	entry.Offset = LittleLong(entry.Offset);
}

static void LittleEndian(FEndCentralDirectory &dir)
{
	dir.DiskNumber = LittleShort(dir.DiskNumber);
	dir.CentralDirectoryDisk = LittleShort(dir.CentralDirectoryDisk);
	dir.CurDiskEntries = LittleShort(dir.CurDiskEntries);
	dir.TotalEntries = LittleShort(dir.TotalEntries);
	dir.CentralDirectorySize = LittleLong(dir.CentralDirectorySize);
	dir.CentralDirectoryOffset = LittleLong(dir.CentralDirectoryOffset);
	dir.CommentLength = LittleShort(dir.CommentLength);
}

void FZip::AddFile(FString path, FResourceLump *data)
{
	Entries.insert({path, data});
}

std::vector<uint8_t> FZip::Build() const
{
	std::vector<std::tuple<FFileHeader, FDirectoryFile>> fileHeaders;
	fileHeaders.reserve(Entries.size());

	uint32_t offset = 0;
	uint32_t totalNamesLength = 0;

	// First pass: Calculate sizing so we can allocate our buffer
	for(const auto& [ path, lump ] : Entries)
	{
		FFileHeader fh;
		FDirectoryFile df;

		df.MDate = fh.MDate = Date;
		fh.CSize = fh.USize = df.CSize = df.USize = lump->LumpSize;
		fh.FileNameLength = df.FileNameLength = uint16_t(path.Len());
		df.Offset = offset;

		offset += sizeof(FFileHeader) + fh.FileNameLength + fh.ExtraLength + fh.CSize;
		totalNamesLength += fh.FileNameLength;

		fileHeaders.emplace_back(fh, df);
	}

	FEndCentralDirectory ecd;
	ecd.TotalEntries = ecd.CurDiskEntries = uint16_t(Entries.size());
	ecd.CentralDirectorySize = sizeof(FDirectoryFile)*ecd.TotalEntries + totalNamesLength;
	ecd.CentralDirectoryOffset = offset;

	std::vector<uint8_t> buffer;
	buffer.resize(size_t(offset) + ecd.CentralDirectorySize + sizeof(FEndCentralDirectory));
	uint8_t *dest = buffer.data();

	// Second pass: Write out the file header/data
	auto headers = fileHeaders.begin();
	for(const auto& [ path, lump ] : Entries)
	{
		auto& [fh, dh] = *headers++;

		auto data = lump->CacheLump();

		fh.Crc32 = dh.Crc32 = mz_crc32(0, (const uint8_t*)data, lump->LumpSize);

		LittleEndian(fh);
		memcpy(dest, &fh, sizeof(fh));
		dest += sizeof(fh);

		memcpy(dest, path.GetChars(), path.Len());
		dest += path.Len();

		memcpy(dest, data, lump->LumpSize);
		dest += lump->LumpSize;

		lump->ReleaseCache();
	}

	// Third pass: Write the central directory
	headers = fileHeaders.begin();
	for(const auto& [ path, lump ] : Entries)
	{
		auto& dh = std::get<1>(*headers++);

		LittleEndian(dh);
		memcpy(dest, &dh, sizeof(dh));
		dest += sizeof(dh);

		memcpy(dest, path.GetChars(), path.Len());
		dest += path.Len();
	}

	// Finish the zip file
	LittleEndian(ecd);
	memcpy(dest, &ecd, sizeof(ecd));

	return buffer;
}
