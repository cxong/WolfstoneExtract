/*
** files.cpp
** Implements classes for reading from files or memory blocks
**
**---------------------------------------------------------------------------
** Copyright 1998-2008 Randy Heit
** Copyright 2005-2008 Christoph Oelckers
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
*/

#include "doomerrors.h"
#include "files.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>

#define I_Error(...) { \
	char buffer[4096]; \
	snprintf(buffer, 4096, __VA_ARGS__); \
	throw CRecoverableError(buffer); \
}

#ifdef _WIN32
#define ftell _ftelli64
#define fseek _fseeki64
#endif

//==========================================================================
//
// FileReader
//
// reads data from an uncompressed file or part of it
//
//==========================================================================

FileReader::FileReader ()
: File(NULL), Length(0), StartPos(0), FilePos(0), CloseOnDestruct(false)
{
}

FileReader::FileReader (const FileReader &other, long long length)
: File(other.File), Length(length), CloseOnDestruct(false)
{
	FilePos = StartPos = ftell (other.File);
}

FileReader::FileReader (const char *filename)
: File(NULL), Length(0), StartPos(0), FilePos(0), CloseOnDestruct(false)
{
	if (!Open(filename))
	{
		I_Error ("Could not open %s", filename);
	}
}

FileReader::FileReader (FILE *file)
: File(file), Length(0), StartPos(0), FilePos(0), CloseOnDestruct(false)
{
	Length = CalcFileLen();
}

FileReader::FileReader (FILE *file, long long length)
: File(file), Length(length), CloseOnDestruct(true)
{
	FilePos = StartPos = ftell (file);
}

FileReader::~FileReader ()
{
	if (CloseOnDestruct && File != NULL)
	{
		fclose (File);
		File = NULL;
	}
}

bool FileReader::Open (const char *filename)
{
	File = fopen(filename, "rb");
	if (File == NULL) return false;
	FilePos = 0;
	StartPos = 0;
	CloseOnDestruct = true;
	Length = CalcFileLen();
	return true;
}


void FileReader::ResetFilePtr ()
{
	FilePos = ftell (File);
}

long long FileReader::Tell () const
{
	return FilePos - StartPos;
}

long FileReader::Seek (long long offset, int origin)
{
	if (origin == SEEK_SET)
	{
		offset += StartPos;
	}
	else if (origin == SEEK_CUR)
	{
		offset += FilePos;
	}
	else if (origin == SEEK_END)
	{
		offset += StartPos + Length;
	}
	if (0 == fseek (File, offset, SEEK_SET))
	{
		FilePos = offset;
		return 0;
	}
	return -1;
}

long long FileReader::Read (void *buffer, long long len)
{
	assert(len >= 0);
	if (len <= 0) return 0;
	if (FilePos + len > StartPos + Length)
	{
		len = Length - FilePos + StartPos;
	}
	len = fread (buffer, 1, len, File);
	FilePos += len;
	return len;
}

char *FileReader::Gets(char *strbuf, int len)
{
	if (len <= 0 || FilePos >= StartPos + Length) return NULL;
	char *p = fgets(strbuf, len, File);
	if (p != NULL)
	{
		auto old = FilePos;
		FilePos = ftell(File);
		if (FilePos - StartPos > Length)
		{
			strbuf[Length - old + StartPos] = 0;
		}
	}
	return p;
}

char *FileReader::GetsFromBuffer(const char * bufptr, char *strbuf, long long len)
{
	if (len>Length-FilePos) len=Length-FilePos;
	if (len <= 0) return NULL;

	char *p = strbuf;
	while (len > 1)
	{
		if (bufptr[FilePos] == 0)
		{
			FilePos++;
			break;
		}
		if (bufptr[FilePos] != '\r')
		{
			*p++ = bufptr[FilePos];
			len--;
			if (bufptr[FilePos] == '\n') 
			{
				FilePos++;
				break;
			}
		}
		FilePos++;
	}
	if (p==strbuf) return NULL;
	*p++=0;
	return strbuf;
}

long long FileReader::CalcFileLen() const
{
	long long endpos;

	fseek (File, 0, SEEK_END);
	endpos = ftell (File);
	fseek (File, 0, SEEK_SET);
	return endpos;
}

//==========================================================================
//
// MemoryReader
//
// reads data from a block of memory
//
//==========================================================================

MemoryReader::MemoryReader (const char *buffer, long length)
{
	bufptr=buffer;
	Length=length;
	FilePos=0;
}

MemoryReader::~MemoryReader ()
{
}

long long MemoryReader::Tell () const
{
	return FilePos;
}

long MemoryReader::Seek (long long offset, int origin)
{
	switch (origin)
	{
	case SEEK_CUR:
		offset+=FilePos;
		break;

	case SEEK_END:
		offset+=Length;
		break;

	}
	FilePos=std::clamp<long long>(offset,0,Length);
	return 0;
}

long long MemoryReader::Read (void *buffer, long long len)
{
	if (len>Length-FilePos) len=Length-FilePos;
	if (len<0) len=0;
	memcpy(buffer,bufptr+FilePos,len);
	FilePos+=len;
	return len;
}

char *MemoryReader::Gets(char *strbuf, int len)
{
	return GetsFromBuffer(bufptr, strbuf, len);
}
