#include <stdio.h>
#include <string.h>

#include "exif.h"

#define EXIF_DEBUG 0
#if EXIF_DEBUG
#define D(...) printf(__VA_ARGS__)
#else
#define D(...)
#endif

// IFD types
#define IFD_TYPE_BYTE       1   //  8-bit unsigned integer
#define IFD_TYPE_ASCII      2   //  8-bit byte that contains a 7-bit ASCII code; the last byte must be NUL (binary zero)
#define IFD_TYPE_SHORT      3   // 16-bit (2-byte) unsigned integer
#define IFD_TYPE_LONG       4   // 32-bit (4-byte) unsigned integer
#define IFD_TYPE_RATIONAL   5   // Two LONGs:  the first represents the numerator of a fraction; the second, the denominator

#define IFD_TYPE_SBYTE      6   //  8-bit signed (twos-complement) integer
#define IFD_TYPE_UNDEFINED  7   //  8-bit byte that may contain anything, depending on the definition of the field
#define IFD_TYPE_SSHORT     8   // 16-bit (2-byte) signed (twos-complement) integer
#define IFD_TYPE_SLONG      9   // 32-bit (4-byte) signed (twos-complement) integer
#define IFD_TYPE_SRATIONAL 10   // Two SLONG’s:  the first represents the numerator of a fraction, the second the denominator
#define IFD_TYPE_FLOAT     11   // Single precision (4-byte) IEEE format
#define IFD_TYPE_DOUBLE    12   // Double precision (8-byte) IEEE format

// IFD tags
#define TIFF_Orientation   0x0112
#define TIFF_ExifIFD       0x8769
#define TIFF_GpsIFD        0x8825

static unsigned int
get_u16(const unsigned char *p, int swap)
{
   if (swap)
      return p[0] << 8 | p[1];
   else
      return p[1] << 8 | p[0];
}

static unsigned int
get_u32(const unsigned char *p, int swap)
{
   if (swap)
      return p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
   else
      return p[3] << 24 | p[2] << 16 | p[1] << 8 | p[0];
}

/*
 * Parse TIFF IFD (Image File Directory)
 */
static void
tiff_parse_ifd(int lvl, const unsigned char *p, unsigned int len,
               const unsigned char *ifd, int swap, ExifInfo * ei)
{
   const unsigned char *pp = ifd;
   unsigned int        tag, type, cnt;
   unsigned int        iifd, nifd;

   D("%s: len=%x(%d)\n", __func__, len, len);

   if (pp + 2 - p > (int)len)
     {
        D("Bad IFD offset\n");
        return;
     }

   nifd = get_u16(pp, swap);
   pp += 2;

   for (iifd = 0; iifd < nifd; iifd++, pp += 12)
     {
        if (pp + 12 - p > (int)len)
          {
             D("Bad offset, break\n");
             return;
          }
        tag = get_u16(pp, swap);
        type = get_u16(pp + 2, swap);
        cnt = get_u32(pp + 4, swap);

#if EXIF_DEBUG
        unsigned int        val = get_u32(pp + 8, swap);

        D("%*s %3d/%3d: tag=%04x type=%2d cnt=%5d val=%9d(0x%08x)\n",
          4 * lvl, " ", iifd, nifd, tag, type, cnt, val, val);
#endif

        switch (tag)
          {
          case TIFF_Orientation:
             if (type == IFD_TYPE_SHORT && cnt == 1)
                ei->orientation = get_u16(pp + 8, swap);
#if EXIF_DEBUG
             break;
#else
             return;            // Done
#endif
#if EXIF_DEBUG
          case TIFF_ExifIFD:
          case TIFF_GpsIFD:
             if (lvl > 0)
                break;
             tiff_parse_ifd(lvl + 1, p, len, p + val, swap, ei);
             break;
#endif
          }
     }
}

/*
 * Parse EXIF data (Exif.. are not part of the TIFF file)
 */
int
exif_parse(const void *data, unsigned int len, ExifInfo * ei)
{
   const unsigned char *ptr = data;
   int                 swap;
   unsigned int        word;

   D("%s: len=%x(%d)\n", __func__, len, len);

   if (memcmp(ptr, "Exif", 4) != 0)
     {
        D("Not EXIF data\n");
        return 1;
     }

   ptr += 6;                    // ptr-> TIFF header
   len -= 6;

   word = ptr[0] << 8 | ptr[1];
   if (word == 0x4949)          // II
      swap = 0;
   else if (word == 0x4d4d)     // MM
      swap = 1;
   else
      return 1;

   word = get_u16(ptr + 2, swap);
   if (word != 42)
     {
        D("Bad TIFF version: %d\n", word);
        return 1;
     }

   word = get_u32(ptr + 4, swap);
   if (word > len)
     {
        D("Bad IFD offset: %d\n", word);
        return 1;
     }

   tiff_parse_ifd(0, ptr, len, ptr + word, swap, ei);

   switch (ei->orientation)
     {
     default:
     case ORIENT_TOPLEFT:
     case ORIENT_TOPRIGHT:
     case ORIENT_BOTRIGHT:
     case ORIENT_BOTLEFT:
        ei->swap_wh = 0;
        break;
     case ORIENT_LEFTTOP:
     case ORIENT_RIGHTTOP:
     case ORIENT_RIGHTBOT:
     case ORIENT_LEFTBOT:
        ei->swap_wh = 1;
        break;
     }

   D("Orientation: %d (swap w/h=%d)\n", ei->orientation, ei->swap_wh);

   return len;
}
