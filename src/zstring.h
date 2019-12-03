#pragma once
/*
** zstring.h
**
**---------------------------------------------------------------------------
** Copyright 2005-2007 Randy Heit
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

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

#include <utility>

#ifdef _WIN32
std::wstring WideString(const char *);
#endif

struct FStringData
{
	unsigned int Len;		// Length of string, excluding terminating null
	unsigned int AllocLen;	// Amount of memory allocated for string
	int RefCount;			// < 0 means it's locked
	// char StrData[xxx];

	char *Chars()
	{
		return (char *)(this + 1);
	}

	const char *Chars() const
	{
		return (const char *)(this + 1);
	}

	char *AddRef()
	{
		if (RefCount < 0)
		{
			return (char *)(MakeCopy() + 1);
		}
		else
		{
			RefCount++;
			return (char *)(this + 1);
		}
	}

	void Release()
	{
		assert (RefCount != 0);

		if (--RefCount <= 0)
		{
			Dealloc();
		}
	}

	FStringData *MakeCopy();

	static FStringData *Alloc (size_t strlen);
	FStringData *Realloc (size_t newstrlen);
	void Dealloc ();
};

struct FNullStringData
{
	unsigned int Len;
	unsigned int AllocLen;
	int RefCount;
	char Nothing[2];
};

enum ELumpNum
{
};

class FString
{
public:
	FString () { ResetToNull(); }

	// Copy constructors
	FString (const FString &other) { AttachToOther (other); }
	FString (FString &&other) : Chars(other.Chars) { other.ResetToNull(); }
	FString (const char *copyStr);
	FString (const char *copyStr, size_t copyLen);
	FString (char oneChar);
	// This is intentionally #ifdef'd. The only code which needs this is parts of the Windows backend that receive Unicode text from the system.
#ifdef _WIN32
	explicit FString(const wchar_t *copyStr);
	FString &operator = (const wchar_t *copyStr);
	std::wstring WideString() const { return ::WideString(Chars); }
#endif

	// Concatenation constructors
	FString (const FString &head, const FString &tail);
	FString (const FString &head, const char *tail);
	FString (const FString &head, char tail);
	FString (const char *head, const FString &tail);
	FString (const char *head, const char *tail);
	FString (char head, const FString &tail);

	// Other constructors
	FString (ELumpNum);	// Create from a lump

	~FString ();

	// Discard string's contents, create a new buffer, and lock it.
	char *LockNewBuffer(size_t len);

	char *LockBuffer();		// Obtain write access to the character buffer
	void UnlockBuffer();	// Allow shared access to the character buffer

	void Swap(FString &other)
	{
		std::swap(Chars, other.Chars);
	}

	operator const char *() const { return Chars; }

	const char *GetChars() const { return Chars; }

	const char &operator[] (int index) const { return Chars[index]; }
#if defined(_WIN32) && !defined(_WIN64) && defined(_MSC_VER)
	// Compiling 32-bit Windows source with MSVC: size_t is typedefed to an
	// unsigned int with the 64-bit portability warning attribute, so the
	// prototype cannot substitute unsigned int for size_t, or you get
	// spurious warnings.
	const char &operator[] (size_t index) const { return Chars[index]; }
#else
	const char &operator[] (unsigned int index) const { return Chars[index]; }
#endif
	const char &operator[] (unsigned long index) const { return Chars[index]; }
	const char &operator[] (unsigned long long index) const { return Chars[index]; }

	FString &operator = (const FString &other);
	FString &operator = (FString &&other);
	FString &operator = (const char *copyStr);

	FString operator + (const FString &tail) const;
	FString operator + (const char *tail) const;
	FString operator + (char tail) const;
	friend FString operator + (const char *head, const FString &tail);
	friend FString operator + (char head, const FString &tail);

	FString &operator += (const FString &tail);
	FString &operator += (const char *tail);
	FString &operator += (char tail);
	FString &AppendCStrPart (const char *tail, size_t tailLen);
	FString &CopyCStrPart(const char *tail, size_t tailLen);

	FString &operator << (const FString &tail) { return *this += tail; }
	FString &operator << (const char *tail) { return *this += tail; }
	FString &operator << (char tail) { return *this += tail; }

	const char &Front() const { assert(IsNotEmpty()); return Chars[0]; }
	const char &Back() const { assert(IsNotEmpty()); return Chars[Len() - 1]; }

	FString Left (size_t numChars) const;
	FString Right (size_t numChars) const;
	FString Mid (size_t pos, size_t numChars = ~(size_t)0) const;

	void DeleteLastCharacter();

	long IndexOf (const FString &substr, long startIndex=0) const;
	long IndexOf (const char *substr, long startIndex=0) const;
	long IndexOf (char subchar, long startIndex=0) const;

	long IndexOfAny (const FString &charset, long startIndex=0) const;
	long IndexOfAny (const char *charset, long startIndex=0) const;

	// This is only kept for backwards compatibility with old ZScript versions that used this function and depend on its bug.
	long LastIndexOf (char subchar) const;
	long LastIndexOfBroken (const FString &substr, long endIndex) const;
	long LastIndexOf (char subchar, long endIndex) const;

	long LastIndexOfAny (const FString &charset) const;
	long LastIndexOfAny (const char *charset) const;
	long LastIndexOfAny (const FString &charset, long endIndex) const;
	long LastIndexOfAny (const char *charset, long endIndex) const;

	long LastIndexOf (const FString &substr) const;
	long LastIndexOf (const FString &substr, long endIndex) const;
	long LastIndexOf (const char *substr) const;
	long LastIndexOf (const char *substr, long endIndex) const;
	long LastIndexOf (const char *substr, long endIndex, size_t substrlen) const;

	void ToUpper ();
	void ToLower ();

	void StripLeft ();
	void StripLeft (const FString &charset);
	void StripLeft (const char *charset);

	void StripRight ();
	void StripRight (const FString &charset);
	void StripRight (const char *charset);

	void StripLeftRight ();
	void StripLeftRight (const FString &charset);
	void StripLeftRight (const char *charset);

	void Insert (size_t index, const FString &instr);
	void Insert (size_t index, const char *instr);
	void Insert (size_t index, const char *instr, size_t instrlen);

	template<typename Func>
	void ReplaceChars (Func IsOldChar, char newchar)
	{
		size_t i, j;

		LockBuffer();
		for (i = 0, j = Len(); i < j; ++i)
		{
			if (IsOldChar(Chars[i]))
			{
				Chars[i] = newchar;
			}
		}
		UnlockBuffer();
	}

	void ReplaceChars (char oldchar, char newchar);
	void ReplaceChars (const char *oldcharset, char newchar);

	template<typename Func>
	void StripChars (Func IsKillChar)
	{
		size_t read, write, mylen;

		LockBuffer();
		for (read = write = 0, mylen = Len(); read < mylen; ++read)
		{
			if (!IsKillChar(Chars[read]))
			{
				Chars[write++] = Chars[read];
			}
		}
		Chars[write] = '\0';
		ReallocBuffer (write);
		UnlockBuffer();
	}

	void StripChars (char killchar);
	void StripChars (const char *killcharset);

	void MergeChars (char merger);
	void MergeChars (char merger, char newchar);
	void MergeChars (const char *charset, char newchar);

	void Substitute (const FString &oldstr, const FString &newstr);
	void Substitute (const char *oldstr, const FString &newstr);
	void Substitute (const FString &oldstr, const char *newstr);
	void Substitute (const char *oldstr, const char *newstr);
	void Substitute (const char *oldstr, const char *newstr, size_t oldstrlen, size_t newstrlen);

	bool IsInt () const;
	bool IsFloat () const;
	int64_t ToLong (int base=0) const;
	uint64_t ToULong (int base=0) const;
	double ToDouble () const;

	size_t Len() const { return Data()->Len; }
	bool IsEmpty() const { return Len() == 0; }
	bool IsNotEmpty() const { return Len() != 0; }

	void Truncate (size_t newlen);
	void Remove(size_t index, size_t remlen);

	int Compare (const FString &other) const { return strcmp (Chars, other.Chars); }
	int Compare (const char *other) const { return strcmp (Chars, other); }
	int Compare(const FString &other, int len) const { return strncmp(Chars, other.Chars, len); }
	int Compare(const char *other, int len) const { return strncmp(Chars, other, len); }

	int CompareNoCase (const FString &other) const { return stricmp (Chars, other.Chars); }
	int CompareNoCase (const char *other) const { return stricmp (Chars, other); }
	int CompareNoCase(const FString &other, int len) const { return strnicmp(Chars, other.Chars, len); }
	int CompareNoCase(const char *other, int len) const { return strnicmp(Chars, other, len); }

	enum EmptyTokenType
	{
		TOK_SKIPEMPTY = 0,
		TOK_KEEPEMPTY = 1,
	};

protected:
	const FStringData *Data() const { return (FStringData *)Chars - 1; }
	FStringData *Data() { return (FStringData *)Chars - 1; }

	void ResetToNull()
	{
		NullString.RefCount++;
		Chars = &NullString.Nothing[0];
	}

	void AttachToOther (const FString &other);
	void AllocBuffer (size_t len);
	void ReallocBuffer (size_t newlen);

	static void StrCopy (char *to, const char *from, size_t len);
	static void StrCopy (char *to, const FString &from);

	char *Chars;

	static FNullStringData NullString;

	friend struct FStringData;

public:
	bool operator == (const FString &other) const
	{
		return Compare(other) == 0;
	}

	bool operator != (const FString &other) const
	{
		return Compare(other) != 0;
	}

	bool operator < (const FString &other) const
	{
		return Compare(other) < 0;
	}

	bool operator > (const FString &other) const
	{
		return Compare(other) > 0;
	}

	bool operator <= (const FString &other) const
	{
		return Compare(other) <= 0;
	}

	bool operator >= (const FString &other) const
	{
		return Compare(other) >= 0;
	}

	bool operator == (const char *) const = delete;
	bool operator != (const char *) const = delete;
	bool operator <  (const char *) const = delete;
	bool operator >  (const char *) const = delete;
	bool operator <= (const char *) const = delete;
	bool operator >= (const char *) const = delete;

private:
};

bool operator == (const char *, const FString &) = delete;
bool operator != (const char *, const FString &) = delete;
bool operator <  (const char *, const FString &) = delete;
bool operator >  (const char *, const FString &) = delete;
bool operator <= (const char *, const FString &) = delete;
bool operator >= (const char *, const FString &) = delete;
