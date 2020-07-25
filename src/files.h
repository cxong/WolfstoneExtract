#ifndef FILES_H
#define FILES_H

#include <cstdio>
#include <cstdint>
#include "m_swap.h"

class FileReaderBase
{
public:
	virtual ~FileReaderBase() {}
	virtual long long Read (void *buffer, long long len) = 0;

	FileReaderBase &operator>> (uint8_t &v)
	{
		Read (&v, 1);
		return *this;
	}

	FileReaderBase &operator>> (int8_t &v)
	{
		Read (&v, 1);
		return *this;
	}

	FileReaderBase &operator>> (uint16_t &v)
	{
		Read (&v, 2);
		v = LittleShort(v);
		return *this;
	}

	FileReaderBase &operator>> (int16_t &v)
	{
		Read (&v, 2);
		v = LittleShort(v);
		return *this;
	}

	FileReaderBase &operator>> (uint32_t &v)
	{
		Read (&v, 4);
		v = LittleLong(v);
		return *this;
	}
};


class FileReader : public FileReaderBase
{
public:
	FileReader ();
	FileReader (const char *filename);
	FileReader (FILE *file);
	FileReader (FILE *file, long long length);
	bool Open (const char *filename);
	virtual ~FileReader ();

	virtual long long Tell () const;
	virtual long Seek (long long offset, int origin);
	virtual long long Read (void *buffer, long long len);
	virtual char *Gets(char *strbuf, int len);
	long long GetLength () const { return Length; }

	// If you use the underlying FILE without going through this class,
	// you must call ResetFilePtr() before using this class again.
	void ResetFilePtr ();

	FILE *GetFile () const { return File; }
	virtual const char *GetBuffer() const { return NULL; }

	FileReader &operator>> (uint8_t &v)
	{
		Read (&v, 1);
		return *this;
	}

	FileReader &operator>> (int8_t &v)
	{
		Read (&v, 1);
		return *this;
	}

	FileReader &operator>> (uint16_t &v)
	{
		Read (&v, 2);
		v = LittleShort(v);
		return *this;
	}

	FileReader &operator>> (int16_t &v)
	{
		Read (&v, 2);
		v = LittleShort(v);
		return *this;
	}

	FileReader &operator>> (uint32_t &v)
	{
		Read (&v, 4);
		v = LittleLong(v);
		return *this;
	}


protected:
	FileReader (const FileReader &other, long long length);

	char *GetsFromBuffer(const char * bufptr, char *strbuf, long long len);

	FILE *File;
	long long Length;
	long long StartPos;
	long long FilePos;

private:
	long long CalcFileLen () const;
protected:
	bool CloseOnDestruct;
};

class MemoryReader : public FileReader
{
public:
	MemoryReader (const char *buffer, long length);
	~MemoryReader ();

	virtual long long Tell () const;
	virtual long Seek (long long offset, int origin);
	virtual long long Read (void *buffer, long long len);
	virtual char *Gets(char *strbuf, int len);
	virtual const char *GetBuffer() const { return bufptr; }

protected:
	const char * bufptr;
};

#endif
