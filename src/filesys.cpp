/*
** filesys.cpp
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define USE_WINDOWS_DWORD
#define USE_WINDOWS_BOOLEAN
#include <windows.h>
#include <direct.h>
#include <shlobj.h>
#else
#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#include <sys/attr.h>
#include <sys/mount.h>
#include <mach-o/dyld.h>
#endif
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif

#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include "filesys.h"
#include "zstring.h"

#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif

namespace FileSys {

#ifdef _WIN32
// Utility functions for converting to and from wchar_t since we want to use
// UTF8 so that the majority of our file system code is platform independent.
static inline void ConvertName(const char* filename, wchar_t* out, const int len=MAX_PATH)
{
	MultiByteToWideChar(CP_UTF8, 0, filename, -1, out, len);
}
static inline void ConvertName(const wchar_t* filename, char* out, const int len=MAX_PATH)
{
	WideCharToMultiByte(CP_UTF8, 0, filename, -1, out, len, NULL, NULL);
}

#ifdef _M_X64
static const bool IsWinNT = true;
#else
static bool IsWinNT = true;
#endif
#endif

// Converts a relative filename to an absolute name
static void FullFileName(const char* filename, char* dest)
{
#ifdef _WIN32
	wchar_t path[MAX_PATH], fullpath[MAX_PATH];
	ConvertName(filename, path);
	_wfullpath(fullpath, path, MAX_PATH);
	ConvertName(fullpath, dest);
#else
	if(realpath(filename, dest) == NULL)
		strncpy(dest, filename, MAX_PATH);
#endif
}

static bool CreateDirectoryIfNeeded(const char* path)
{
#ifdef _WIN32
	struct _stat dirStat;
	if(IsWinNT)
	{
		wchar_t wpath[MAX_PATH];
		ConvertName(path, wpath);
		if(_wstat(wpath, &dirStat) == -1)
		{
			if(_wmkdir(wpath) == -1)
				return false;
		}
	}
	else
	{
		if(_stat(path, &dirStat) == -1)
		{
			if(_mkdir(path) == -1)
				return false;
		}
	}
	return true;
#else
	struct stat dirStat;
	if(stat(path, &dirStat) == -1)
	{
		if(mkdir(path, S_IRWXU) == -1)
			return false;
	}
	return true;
#endif
}

}

File::File(const FString &filename)
{
	init(filename);
}

File::File(const File &dir, const FString &filename)
{
	init(dir.getDirectory() + PATH_SEPARATOR + filename);
}

void File::init(FString filename)
{
	this->filename = filename;
	directory = false;
	existing = false;
	writable = false;

#ifdef _WIN32
	if(FileSys::IsWinNT)
	{
		wchar_t wname[MAX_PATH];
		FileSys::ConvertName(filename, wname);
		DWORD fAttributes = GetFileAttributesW(wname);
		if(fAttributes != INVALID_FILE_ATTRIBUTES)
		{
			existing = true;
			if(fAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				directory = true;
				WIN32_FIND_DATAW fdata;
				HANDLE hnd = INVALID_HANDLE_VALUE;
				FileSys::ConvertName(filename + "\\*", wname);
				hnd = FindFirstFileW(wname, &fdata);
				if(hnd != INVALID_HANDLE_VALUE)
				{
					do
					{
						char fname[MAX_PATH];
						FileSys::ConvertName(fdata.cFileName, fname);
						files.Push(fname);
					}
					while(FindNextFileW(hnd, &fdata) != 0);
				}
			}
		}
	}
	else
	{
		DWORD fAttributes = GetFileAttributesA(filename);
		if(fAttributes != INVALID_FILE_ATTRIBUTES)
		{
			existing = true;
			if(fAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				directory = true;
				WIN32_FIND_DATA fdata;
				HANDLE hnd = INVALID_HANDLE_VALUE;
				hnd = FindFirstFileA((filename + "\\*").GetChars(), &fdata);
				if(hnd != INVALID_HANDLE_VALUE)
				{
					do
					{
						files.Push(fdata.cFileName);
					}
					while(FindNextFileA(hnd, &fdata) != 0);
				}
			}
		}
	}

	// Can't find an easy way to test writability on Windows so
	writable = true;
#else
	struct stat statRet;
	if(stat(filename, &statRet) == 0)
		existing = true;

	if(existing)
	{
		if((statRet.st_mode & S_IFDIR))
		{
			directory = true;

			// Populate a base list.
			DIR *direct = opendir(filename);
			if(direct != NULL)
			{
				dirent *file = NULL;
				while((file = readdir(direct)) != NULL)
					files.Push(file->d_name);
			}
			closedir(direct);
		}

		// Check writable
		if(access(filename, W_OK) == 0)
			writable = true;
	}
#endif
}

FString File::getDirectory() const
{
	if(directory)
	{
		if(filename[filename.Len()-1] == '\\' || filename[filename.Len()-1] == '/')
			return filename.Left(filename.Len()-1);
		return filename;
	}

	long dirSepPos = filename.LastIndexOfAny("/\\");
	if(dirSepPos != -1)
		return filename.Left(dirSepPos);
	return FString(".");
}

FString File::getFileName() const
{
	if(directory)
		return FString();

	long dirSepPos = filename.LastIndexOfAny("/\\");
	if(dirSepPos != -1)
		return filename.Mid(dirSepPos+1);
	return filename;
}

FString File::getInsensitiveFile(const FString &filename, bool sensitiveExtension) const
{
#ifdef _WIN32
	// Insensitive filesystem, so just return the filename
	return filename;
#else
#ifdef __APPLE__
	{
		// Mac supports both case insensitive and sensitive file systems
		attrlist pathAttr = {
			ATTR_BIT_MAP_COUNT, 0,
			0, ATTR_VOL_CAPABILITIES, 0, 0, 0
		};
		struct statfs fs;
		struct
		{
			u_int32_t length;
			vol_capabilities_attr_t cap;
		} __attribute__((aligned(4), packed)) vol;

		int attrError = -1;
		if(statfs(this->filename, &fs) == 0)
			attrError = getattrlist(fs.f_mntonname, &pathAttr, &vol, sizeof(vol), 0);
		if(attrError || !(vol.cap.capabilities[0] & vol.cap.valid[0] & VOL_CAP_FMT_CASE_SENSITIVE))
		{
			// We assume case insensitive (since it's the default) unless the volcap tells us otherwise
			return filename;
		}
	}
#endif

	const TArray<FString> &files = getFileList();
	FString extension = filename.Mid(filename.LastIndexOf('.')+1);

	for(unsigned int i = 0;i < files.Size();++i)
	{
		if(files[i].CompareNoCase(filename) == 0)
		{
			if(!sensitiveExtension || files[i].Mid(files[i].LastIndexOf('.')+1).Compare(extension) == 0)
				return files[i];
		}
	}
	return filename;
#endif
}

bool File::makeDir()
{
	if(FileSys::CreateDirectoryIfNeeded(getPath()))
	{
		directory = true;
		existing = true;
		return true;
	}
	return false;
}

/**
 * Open a file while handling any non-English character sets and
 * respecting our virtual renaming table.
 */
FILE *File::open(const char* mode) const
{
	char path[MAX_PATH];
	FileSys::FullFileName(filename, path);
	FString fn = filename;

#ifdef _WIN32
	if(FileSys::IsWinNT)
	{
		wchar_t wname[MAX_PATH], wmode[MAX_PATH];
		FileSys::ConvertName(fn, wname);
		FileSys::ConvertName(mode, wmode);
		return _wfopen(wname, wmode);
	}
#endif
	return fopen(fn, mode);
}

bool File::remove()
{
	char path[MAX_PATH];
	FileSys::FullFileName(filename, path);
	FString fn = filename;
#ifdef _WIN32
	if(FileSys::IsWinNT)
	{
		wchar_t wname[MAX_PATH];
		FileSys::ConvertName(fn, wname);
		return DeleteFileW(wname) != 0;
	}
	else
		return DeleteFileA(fn) != 0;
#else
	return unlink(fn) == 0;
#endif
}
