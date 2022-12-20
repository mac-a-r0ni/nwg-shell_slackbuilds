#ifndef __COMMON
#define __COMMON 1

#include "config.h"
#include "types.h"

#if __GNUC__
#define __PRINTF_N__(no)  __attribute__((__format__(__printf__, (no), (no)+1)))
#else
#define __PRINTF_N__(no)
#endif
#define __PRINTF__   __PRINTF_N__(1)
#define __PRINTF_2__ __PRINTF_N__(2)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define SWAP32(x) \
    ((((x) & 0x000000ff ) << 24) | \
     (((x) & 0x0000ff00 ) <<  8) | \
     (((x) & 0x00ff0000 ) >>  8) | \
     (((x) & 0xff000000 ) >> 24))

#define SWAP16(x) \
    ((((x) & 0x00ff ) << 8) | \
     (((x) & 0xff00 ) >> 8))

#ifdef WORDS_BIGENDIAN
#define SWAP_LE_16(x) SWAP16(x)
#define SWAP_LE_32(x) SWAP32(x)
#define SWAP_LE_16_INPLACE(x) x = SWAP16(x)
#define SWAP_LE_32_INPLACE(x) x = SWAP32(x)
#else
#define SWAP_LE_16(x) (x)
#define SWAP_LE_32(x) (x)
#define SWAP_LE_16_INPLACE(x)
#define SWAP_LE_32_INPLACE(x)
#endif

#define PIXEL_ARGB(a, r, g, b)  ((a) << 24) | ((r) << 16) | ((g) << 8) | (b)

#define PIXEL_A(argb)  (((argb) >> 24) & 0xff)
#define PIXEL_R(argb)  (((argb) >> 16) & 0xff)
#define PIXEL_G(argb)  (((argb) >>  8) & 0xff)
#define PIXEL_B(argb)  (((argb)      ) & 0xff)

#ifndef WORDS_BIGENDIAN
#define A_VAL(p) ((uint8_t *)(p))[3]
#define R_VAL(p) ((uint8_t *)(p))[2]
#define G_VAL(p) ((uint8_t *)(p))[1]
#define B_VAL(p) ((uint8_t *)(p))[0]
#else
#define A_VAL(p) ((uint8_t *)(p))[0]
#define R_VAL(p) ((uint8_t *)(p))[1]
#define G_VAL(p) ((uint8_t *)(p))[2]
#define B_VAL(p) ((uint8_t *)(p))[3]
#endif

#define CLIP(x, y, w, h, xx, yy, ww, hh) \
    if (x < (xx)) { w += (x - (xx)); x = (xx); } \
    if (y < (yy)) { h += (y - (yy)); y = (yy); } \
    if ((x + w) > ((xx) + (ww))) { w = (ww) - (x - xx); } \
    if ((y + h) > ((yy) + (hh))) { h = (hh) - (y - yy); }

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#endif
