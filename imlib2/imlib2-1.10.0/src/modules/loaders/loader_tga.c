/*
 * loader_tga.c - Loader for Truevision Targa images
 *                for Imlib2
 *
 * by Dan Maas <dmaas@dcine.com>   May 15, 2000
 *
 * based on TGA specifications available at:
 * http://www.wotsit.org/cgi-bin/search.cgi?TGA
 *
 * header/footer structures courtesy of the GIMP Targa plugin
 */
#include "config.h"
#include "Imlib2_Loader.h"

#define DBG_PFX "LDR-tga"

static const char  *const _formats[] = { "tga" };

/* flip an inverted image - see RLE reading below */
static void         tgaflip(uint32_t * in, int w, int h, int fliph, int flipv);

/* TGA pixel formats */
#define TGA_TYPE_MAPPED      1
#define TGA_TYPE_COLOR       2
#define TGA_TYPE_GRAY        3
#define TGA_TYPE_MAPPED_RLE  9
#define TGA_TYPE_COLOR_RLE  10
#define TGA_TYPE_GRAY_RLE   11

/* TGA header flags */
#define TGA_DESC_ABITS      0x0f
#define TGA_DESC_HORIZONTAL 0x10
#define TGA_DESC_VERTICAL   0x20

#define TGA_SIGNATURE "TRUEVISION-XFILE"

typedef struct {
   unsigned char       idLength;
   unsigned char       colorMapType;
   unsigned char       imageType;
   unsigned char       colorMapIndexLo, colorMapIndexHi;
   unsigned char       colorMapLengthLo, colorMapLengthHi;
   unsigned char       colorMapSize;
   unsigned char       xOriginLo, xOriginHi;
   unsigned char       yOriginLo, yOriginHi;
   unsigned char       widthLo, widthHi;
   unsigned char       heightLo, heightHi;
   unsigned char       bpp;
   unsigned char       descriptor;
} tga_header;

typedef struct {
   unsigned int        extensionAreaOffset;
   unsigned int        developerDirectoryOffset;
   char                signature[16];
   char                dot;
   char                null;
} tga_footer;

/* Load up a TGA file
 *
 * As written this function only recognizes the following types of Targas:
 *		Type 02 - Uncompressed RGB, 24 or 32 bits
 *		Type 03 - Uncompressed grayscale, 8 bits
 *		Type 10 - RLE-compressed RGB, 24 or 32 bits
 *		Type 11 - RLE-compressed grayscale, 8 bits
 * There are several other (uncommon) Targa formats which this function can't currently handle
 */

static int
_load(ImlibImage * im, int load_data)
{
   int                 rc;
   const unsigned char *fptr;
   const tga_header   *header;
   const tga_footer   *footer;
   int                 footer_present;
   int                 rle, bpp, hasa, hasc, fliph, flipv;
   unsigned long       datasize;
   const unsigned char *bufptr, *bufend, *palette;
   uint32_t           *dataptr;
   int                 palcnt = 0, palbpp = 0;
   unsigned char       a, r, g, b;
   unsigned int        pix16;

   rc = LOAD_FAIL;

   if (im->fi->fsize < (int)(sizeof(tga_header)) ||
       (uintmax_t) im->fi->fsize > SIZE_MAX)
      return rc;

   fptr = im->fi->fdata;
   header = im->fi->fdata;

   if (im->fi->fsize > (int)(sizeof(tga_footer)))
     {
        footer = (tga_footer *) (fptr + im->fi->fsize - sizeof(tga_footer));

        /* check the footer to see if we have a v2.0 TGA file */
        footer_present = memcmp(footer->signature, TGA_SIGNATURE,
                                sizeof(footer->signature)) == 0;
     }
   else
     {
        footer_present = 0;
     }

   if ((size_t)im->fi->fsize < sizeof(tga_header) + header->idLength +
       (footer_present ? sizeof(tga_footer) : 0))
      goto quit;

   /* skip over header */
   fptr += sizeof(tga_header);

   /* skip over alphanumeric ID field */
   if (header->idLength)
      fptr += header->idLength;

   /* now parse the header */

   /* this flag indicates right-to-left pixel storage */
   fliph = !!(header->descriptor & TGA_DESC_HORIZONTAL);
   /* this flag indicates bottom-up pixel storage */
   flipv = !(header->descriptor & TGA_DESC_VERTICAL);

   rle = 0;                     /* RLE compressed */
   hasc = 0;                    /* Has color */

   switch (header->imageType)
     {
     default:
        goto quit;

     case TGA_TYPE_MAPPED:
        break;
     case TGA_TYPE_COLOR:
        hasc = 1;
        break;
     case TGA_TYPE_GRAY:
        break;

     case TGA_TYPE_MAPPED_RLE:
        rle = 1;
        break;
     case TGA_TYPE_COLOR_RLE:
        hasc = 1;
        rle = 1;
        break;
     case TGA_TYPE_GRAY_RLE:
        rle = 1;
        break;
     }

   bpp = header->bpp;           /* Bits per pixel */
   hasa = 0;                    /* Has alpha */

   switch (bpp)
     {
     default:
        goto quit;
     case 32:
        if (header->descriptor & TGA_DESC_ABITS)
           hasa = 1;
        break;
     case 24:
        break;
     case 16:
        if (header->descriptor & TGA_DESC_ABITS)
           hasa = 1;
        break;
     case 8:
        break;
     }

   rc = LOAD_BADIMAGE;          /* Format accepted */

   /* endian-safe loading of 16-bit sizes */
   im->w = (header->widthHi << 8) | header->widthLo;
   im->h = (header->heightHi << 8) | header->heightLo;

   D("Image info: type: %d bpp=%d desc=%04x\n",
     header->imageType, header->bpp, header->descriptor);

   if (!IMAGE_DIMENSIONS_OK(im->w, im->h))
      goto quit;

   im->has_alpha = hasa;

   if (!load_data)
      QUIT_WITH_RC(LOAD_SUCCESS);

   /* find out how much data must be read from the file */
   /* (this is NOT simply width*height*4, due to compression) */

   datasize = im->fi->fsize - sizeof(tga_header) - header->idLength -
      (footer_present ? sizeof(tga_footer) : 0);

   palette = NULL;
   if (header->imageType == TGA_TYPE_MAPPED ||
       header->imageType == TGA_TYPE_MAPPED_RLE)
     {
        if (bpp != 8)
           goto quit;
        palette = fptr;
        palcnt = (header->colorMapLengthHi << 8) | header->colorMapLengthLo;
        palbpp = header->colorMapSize / 8;      /* bytes per palette entry */
        if (palbpp < 3 || palbpp > 4)
           goto quit;           /* only supporting 24/32bit palettes */
        int                 palbytes = palcnt * palbpp;

        fptr += palbytes;
        datasize -= palbytes;
     }

   /* buffer is ready for parsing */

   /* bufptr is the next byte to be read from the buffer */
   bufptr = fptr;
   bufend = bufptr + datasize;

   /* Load data */

   /* allocate the destination buffer */
   if (!__imlib_AllocateData(im))
      QUIT_WITH_RC(LOAD_OOM);

   /* dataptr is the next 32-bit pixel to be filled in */
   dataptr = im->data;

   if (!rle)
     {
        int                 x, y;

        /* decode uncompressed BGRA data */
        for (y = 0; y < im->h; y++)     /* for each row */
          {
             /* point dataptr at the beginning of the row */
             if (flipv)
                /* some TGA's are stored upside-down! */
                dataptr = im->data + ((im->h - y - 1) * im->w);
             else
                dataptr = im->data + (y * im->w);

             for (x = 0; (x < im->w); x++)      /* for each pixel in the row */
               {
                  if (bufptr + bpp / 8 > bufend)
                     goto quit;

                  switch (bpp)
                    {
                    case 32:   /* 32-bit BGRA pixels */
                       b = *bufptr++;
                       g = *bufptr++;
                       r = *bufptr++;
                       a = *bufptr++;
                       *dataptr++ = PIXEL_ARGB(a, r, g, b);
                       break;

                    case 24:   /* 24-bit BGR pixels */
                       b = *bufptr++;
                       g = *bufptr++;
                       r = *bufptr++;
                       a = 0xff;
                       *dataptr++ = PIXEL_ARGB(a, r, g, b);
                       break;

                    case 16:
                       b = *bufptr++;
                       a = *bufptr++;
                       if (hasc)
                         {
                            pix16 = b | ((unsigned short)a << 8);
                            r = (pix16 >> 7) & 0xf8;
                            g = (pix16 >> 2) & 0xf8;
                            b = (pix16 << 3) & 0xf8;
                            a = (hasa && !(pix16 & 0x8000)) ? 0x00 : 0xff;
                         }
                       else
                         {
                            r = g = b;
                         }
                       *dataptr++ = PIXEL_ARGB(a, r, g, b);
                       break;

                    case 8:    /* 8-bit grayscale or palette */
                       b = *bufptr++;
                       a = 0xff;
                       if (palette)
                         {
                            if (b >= palcnt)
                               goto quit;
                            r = palette[b * palbpp + 2];
                            g = palette[b * palbpp + 1];
                            b = palette[b * palbpp + 0];
                         }
                       else
                         {
                            r = g = b;
                         }
                       *dataptr++ = PIXEL_ARGB(a, r, g, b);
                       break;
                    }

               }                /* end for (each pixel) */
          }

        if (fliph)
           tgaflip(im->data, im->w, im->h, fliph, 0);
     }
   else
     {
        /* decode RLE compressed data */
        uint32_t           *final_pixel = dataptr + im->w * im->h;

        /* loop until we've got all the pixels or run out of input */
        while ((dataptr < final_pixel))
          {
             int                 i, count;
             unsigned char       curbyte;

             if ((bufptr + 1 + (bpp / 8)) > bufend)
                goto quit;

             curbyte = *bufptr++;
             count = (curbyte & 0x7F) + 1;

             if (curbyte & 0x80)        /* RLE packet */
               {
                  switch (bpp)
                    {
                    case 32:
                       b = *bufptr++;
                       g = *bufptr++;
                       r = *bufptr++;
                       a = *bufptr++;
                       for (i = 0; (i < count) && (dataptr < final_pixel); i++)
                          *dataptr++ = PIXEL_ARGB(a, r, g, b);
                       break;

                    case 24:
                       b = *bufptr++;
                       g = *bufptr++;
                       r = *bufptr++;
                       a = 0xff;
                       for (i = 0; (i < count) && (dataptr < final_pixel); i++)
                          *dataptr++ = PIXEL_ARGB(a, r, g, b);
                       break;

                    case 16:
                       b = *bufptr++;
                       a = *bufptr++;
                       if (hasc)
                         {
                            pix16 = b | ((unsigned short)a << 8);
                            r = (pix16 >> 7) & 0xf8;
                            g = (pix16 >> 2) & 0xf8;
                            b = (pix16 << 3) & 0xf8;
                            a = (hasa && !(pix16 & 0x8000)) ? 0x00 : 0xff;
                         }
                       else
                         {
                            r = g = b;
                         }
                       for (i = 0; (i < count) && (dataptr < final_pixel); i++)
                          *dataptr++ = PIXEL_ARGB(a, r, g, b);
                       break;

                    case 8:
                       b = *bufptr++;
                       a = 0xff;
                       if (palette)
                         {
                            if (b >= palcnt)
                               goto quit;
                            r = palette[b * palbpp + 2];
                            g = palette[b * palbpp + 1];
                            b = palette[b * palbpp + 0];
                         }
                       else
                         {
                            r = g = b;
                         }
                       for (i = 0; (i < count) && (dataptr < final_pixel); i++)
                          *dataptr++ = PIXEL_ARGB(a, r, g, b);
                       break;
                    }
               }                /* end if (RLE packet) */
             else               /* raw packet */
               {
                  for (i = 0; (i < count) && (dataptr < final_pixel); i++)
                    {
                       if ((bufptr + bpp / 8) > bufend)
                          goto quit;

                       switch (bpp)
                         {
                         case 32:      /* 32-bit BGRA pixels */
                            b = *bufptr++;
                            g = *bufptr++;
                            r = *bufptr++;
                            a = *bufptr++;
                            *dataptr++ = PIXEL_ARGB(a, r, g, b);
                            break;

                         case 24:      /* 24-bit BGR pixels */
                            b = *bufptr++;
                            g = *bufptr++;
                            r = *bufptr++;
                            a = 0xff;
                            *dataptr++ = PIXEL_ARGB(a, r, g, b);
                            break;

                         case 16:
                            b = *bufptr++;
                            a = *bufptr++;
                            if (hasc)
                              {
                                 pix16 = b | ((unsigned short)a << 8);
                                 r = (pix16 >> 7) & 0xf8;
                                 g = (pix16 >> 2) & 0xf8;
                                 b = (pix16 << 3) & 0xf8;
                                 a = (hasa && !(pix16 & 0x8000)) ? 0x00 : 0xff;
                              }
                            else
                              {
                                 r = g = b;
                              }
                            *dataptr++ = PIXEL_ARGB(a, r, g, b);
                            break;

                         case 8:       /* 8-bit grayscale or palette */
                            b = *bufptr++;
                            a = 0xff;
                            if (palette)
                              {
                                 if (b >= palcnt)
                                    goto quit;
                                 r = palette[b * palbpp + 2];
                                 g = palette[b * palbpp + 1];
                                 b = palette[b * palbpp + 0];
                              }
                            else
                              {
                                 r = g = b;
                              }
                            *dataptr++ = PIXEL_ARGB(a, r, g, b);
                            break;
                         }
                    }
               }                /* end if (raw packet) */
          }                     /* end for (each packet) */

        if (fliph || flipv)
           tgaflip(im->data, im->w, im->h, fliph, flipv);
     }

   if (im->lc)
      __imlib_LoadProgressRows(im, 0, im->h);

   rc = LOAD_SUCCESS;

 quit:
   return rc;
}

/* flip a uint32_t image block in place */
static void
tgaflip(uint32_t * in, int w, int h, int fliph, int flipv)
{
   uint32_t            tmp;
   int                 x, y, x2, y2, dx, dy, nx, ny;

   dx = fliph ? -1 : 1;
   dy = flipv ? -1 : 1;
   nx = fliph ? w / 2 : w;
   ny = flipv && !fliph ? h / 2 : h;

   y2 = flipv ? h - 1 : 0;
   for (y = 0; y < ny; y++, y2 += dy)
     {
        x2 = fliph ? w - 1 : 0;
        for (x = 0; x < nx; x++, x2 += dx)
          {
             tmp = in[y * w + x];
             in[y * w + x] = in[y2 * w + x2];
             in[y2 * w + x2] = tmp;
          }
     }
}

/*
 * Write an uncompressed RGBA 24- or 32-bit targa to disk
 * (If anyone wants to write a RLE saver, feel free =)
 */

static int
_save(ImlibImage * im)
{
   int                 rc;
   FILE               *f;
   uint32_t           *dataptr;
   unsigned char      *buf, *bufptr;
   int                 y;
   tga_header          header;

   f = fopen(im->fi->name, "wb");
   if (!f)
      return LOAD_FAIL;

   rc = LOAD_FAIL;

   /* assemble the TGA header information */

   /* most entries are zero... */
   memset(&header, 0x0, sizeof(header));

   /* uncompressed RGB Targa identifier */
   header.imageType = TGA_TYPE_COLOR;

   /* image width, low byte  */
   header.widthLo = im->w & 0xFF;
   /* image width, high byte */
   header.widthHi = im->w >> 8;

   /* image height, low byte */
   header.heightLo = im->h & 0xFF;
   /* image height, high byte */
   header.heightHi = im->h >> 8;

   /* total number of bits per pixel */
   header.bpp = im->has_alpha ? 32 : 24;
   /* number of extra (alpha) bits per pixel */
   header.descriptor = im->has_alpha ? 8 : 0;

   /* top-to-bottom storage */
   header.descriptor |= TGA_DESC_VERTICAL;

   /* allocate a buffer to receive the BGRA-swapped pixel values */
   buf = malloc(im->w * im->h * (im->has_alpha ? 4 : 3));
   if (!buf)
      goto quit;

   /* now we have to read from im->data into buf, swapping RGBA to BGRA */
   dataptr = im->data;
   bufptr = buf;

   /* for each row */
   for (y = 0; y < im->h; y++)
     {
        int                 x;

        /* for each pixel in the row */
        for (x = 0; x < im->w; x++)
          {
             uint32_t            pixel = *dataptr++;

             *bufptr++ = PIXEL_B(pixel);
             *bufptr++ = PIXEL_G(pixel);
             *bufptr++ = PIXEL_R(pixel);
             if (im->has_alpha)
                *bufptr++ = PIXEL_A(pixel);
          }                     /* end for (each pixel in row) */

        /* report progress every row */
        if (im->lc && __imlib_LoadProgressRows(im, y, 1))
           QUIT_WITH_RC(LOAD_BREAK);
     }

   /* write the header */
   fwrite(&header, sizeof(header), 1, f);

   /* write the image data */
   fwrite(buf, 1, im->w * im->h * (im->has_alpha ? 4 : 3), f);

   rc = LOAD_SUCCESS;

 quit:
   free(buf);
   fclose(f);

   return rc;
}

IMLIB_LOADER(_formats, _load, _save);
