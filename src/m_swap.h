#ifndef __M_SWAP__
#define __M_SWAP__

#include <cstdint>

constexpr uint16_t ReadLittleShort(const uint8_t * const ptr)
{
	return uint16_t(uint8_t(*ptr)) |
		(uint16_t(uint8_t(*(ptr+1)))<<8);
}

constexpr uint16_t ReadBigShort(const uint8_t * const ptr)
{
	return uint16_t(uint8_t(*(ptr+1))) |
		(uint16_t(uint8_t(*ptr))<<8);
}

constexpr uint32_t ReadLittle24(const uint8_t * const ptr)
{
	return uint32_t(uint8_t(*ptr)) |
		(uint32_t(uint8_t(*(ptr+1)))<<8) |
		(uint32_t(uint8_t(*(ptr+2)))<<16);
}

constexpr uint32_t ReadBig24(const uint8_t * const ptr)
{
	return uint32_t(uint8_t(*(ptr+2))) |
		(uint32_t(uint8_t(*(ptr+1)))<<8) |
		(uint32_t(uint8_t(*ptr))<<16);
}

constexpr uint32_t ReadLittleLong(const uint8_t * const ptr)
{
	return uint32_t(uint8_t(*ptr)) |
		(uint32_t(uint8_t(*(ptr+1)))<<8) |
		(uint32_t(uint8_t(*(ptr+2)))<<16) |
		(uint32_t(uint8_t(*(ptr+3)))<<24);
}

constexpr uint32_t ReadBigLong(const uint8_t * const ptr)
{
	return (uint32_t(uint8_t(*ptr))<<24) |
		(uint32_t(uint8_t(*(ptr+1)))<<16) |
		(uint32_t(uint8_t(*(ptr+2)))<<8) |
		uint32_t(uint8_t(*(ptr+3)));
}

// After the fact Byte Swapping ------------------------------------------------

constexpr uint16_t SwapShort(uint16_t x)
{
	return ((x&0xFF)<<8) | ((x>>8)&0xFF);
}

constexpr uint32_t SwapLong(uint32_t x)
{
	return ((x&0xFF)<<24) |
		(((x>>8)&0xFF)<<16) |
		(((x>>16)&0xFF)<<8) |
		((x>>24)&0xFF);
}

constexpr uint64_t SwapLongLong(uint64_t x)
{
	return ((x&0xFF)<<56) |
		(((x>>8)&0xFF)<<48) |
		(((x>>16)&0xFF)<<40) |
		(((x>>24)&0xFF)<<32) |
		(((x>>32)&0xFF)<<24) |
		(((x>>40)&0xFF)<<16) |
		(((x>>48)&0xFF)<<8) |
		((x>>56)&0xFF);
}

#ifdef __BIG_ENDIAN__
#define BigShort(x) (x)
#define BigLong(x) (x)
#define BigLongLong(x) (x)
#define LittleShort SwapShort
#define LittleLong SwapLong
#define LittleLongLong SwapLongLong
#else
#define BigShort SwapShort
#define BigLong SwapLong
#define BigLongLong SwapLongLong
#define LittleShort(x) (x)
#define LittleLong(x) (x)
#define LittleLongLong(x) (x)
#endif

#endif /* __M_SWAP__ */
