#if !defined(ADAPTER_H) && !defined(_WIN32)
#define ADAPTER_H

#include "x86intrin.h"

#define WINAPI

#define __forceinline inline __attribute__((always_inline))

#define _byteswap_ushort __builtin_bswap16
#define _byteswap_ulong __builtin_bswap32
#define _byteswap_uint64 __builtin_bswap64

constexpr unsigned char _BitScanReverse(unsigned long * Index, unsigned long Mask) {
  if(Mask == 0)
    return 1;
  *Index = 31 - __builtin_clz(Mask);
  return 0;
}

constexpr unsigned char _BitScanForward(unsigned long * Index, unsigned long Mask) {
  if(Mask == 0)
    return 1;
  *Index = 31 - __builtin_ctz(Mask);
  return 0;
}

#endif
