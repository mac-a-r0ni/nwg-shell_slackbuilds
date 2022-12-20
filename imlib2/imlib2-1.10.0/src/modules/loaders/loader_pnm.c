#include "config.h"
#include "Imlib2_Loader.h"

#include <ctype.h>
#include <stdbool.h>

#define DBG_PFX "LDR-pnm"

static const char  *const _formats[] = { "pnm", "ppm", "pgm", "pbm", "pam" };

static struct {
   const unsigned char *data, *dptr;
   unsigned int        size;
} mdata;

static void
mm_init(const void *src, unsigned int size)
{
   mdata.data = mdata.dptr = src;
   mdata.size = size;
}

static int
mm_read(void *dst, unsigned int len)
{
   if (mdata.dptr + len > mdata.data + mdata.size)
      return 1;                 /* Out of data */

   memcpy(dst, mdata.dptr, len);
   mdata.dptr += len;

   return 0;
}

static int
mm_getc(void)
{
   unsigned char       ch;

   if (mdata.dptr + 1 > mdata.data + mdata.size)
      return -1;                /* Out of data */

   ch = *mdata.dptr++;

   return ch;
}

static int
mm_get01(void)
{
   int                 ch;

   for (;;)
     {
        ch = mm_getc();
        switch (ch)
          {
          case '0':
             return 0;
          case '1':
             return 1;
          case ' ':
          case '\t':
          case '\r':
          case '\n':
             continue;
          default:
             return -1;
          }
     }
}

static int
mm_getu(unsigned int *pui)
{
   int                 ch;
   int                 uval;
   bool                comment;

   /* Strip whitespace and comments */
   for (comment = false;;)
     {
        ch = mm_getc();
        if (ch < 0)
           return ch;
        if (comment)
          {
             if (ch == '\n')
                comment = false;
             continue;
          }
        if (isspace(ch))
           continue;
        if (ch != '#')
           break;
        comment = true;
     }

   if (!isdigit(ch))
      return -1;

   /* Parse number */
   for (uval = 0;;)
     {
        uval = 10 * uval + ch - '0';
        ch = mm_getc();
        if (ch < 0)
           return ch;
        if (!isdigit(ch))
           break;
     }

   *pui = uval;
   return 0;                    /* Ok */
}

static int
_load(ImlibImage * im, int load_data)
{
   int                 rc;
   int                 c, p;
   int                 w, h, v, numbers, count;
   uint8_t            *data = NULL;     /* for the binary versions */
   uint8_t            *ptr = NULL;
   int                *idata = NULL;    /* for the ASCII versions */
   uint32_t           *ptr2, rval, gval, bval;
   int                 i, j, x, y;

   rc = LOAD_FAIL;

   mm_init(im->fi->fdata, im->fi->fsize);

   /* read the header info */

   c = mm_getc();
   if (c != 'P')
      goto quit;

   numbers = 3;
   p = mm_getc();
   if (p == '1' || p == '4')
      numbers = 2;              /* bitimages don't have max value */

   if ((p < '1') || (p > '8'))
      goto quit;

   /* read numbers */
   w = h = 0;
   v = 255;
   for (count = i = 0; count < numbers; i++)
     {
        if (mm_getu(&gval))
           goto quit;

        if (p == '7' && i == 0)
          {
             if (gval != 332)
                goto quit;
             else
                continue;
          }

        count++;
        switch (count)
          {
          case 1:              /* width */
             w = gval;
             break;
          case 2:              /* height */
             h = gval;
             break;
          case 3:              /* max value, only for color and greyscale */
             v = gval;
             break;
          }
     }
   if ((v < 0) || (v > 255))
      goto quit;

   D("P%c: WxH=%dx%d V=%d\n", p, w, h, v);

   rc = LOAD_BADIMAGE;          /* Format accepted */

   im->w = w;
   im->h = h;
   if (!IMAGE_DIMENSIONS_OK(w, h))
      goto quit;

   im->has_alpha = p == '8';

   if (!load_data)
      QUIT_WITH_RC(LOAD_SUCCESS);

   /* Load data */

   ptr2 = __imlib_AllocateData(im);
   if (!ptr2)
      QUIT_WITH_RC(LOAD_OOM);

   /* start reading the data */
   switch (p)
     {
     case '1':                 /* ASCII monochrome */
        for (y = 0; y < h; y++)
          {
             for (x = 0; x < w; x++)
               {
                  i = mm_get01();
                  if (i < 0)
                     goto quit;

                  *ptr2++ = i ? 0xff000000 : 0xffffffff;
               }

             if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                goto quit_progress;
          }
        break;
     case '2':                 /* ASCII greyscale */
        for (y = 0; y < h; y++)
          {
             for (x = 0; x < w; x++)
               {
                  if (mm_getu(&gval))
                     goto quit;

                  if (v == 0 || v == 255)
                    {
                       *ptr2++ = 0xff000000 | (gval << 16) | (gval << 8) | gval;
                    }
                  else
                    {
                       *ptr2++ =
                          0xff000000 | (((gval * 255) / v) << 16) |
                          (((gval * 255) / v) << 8) | ((gval * 255) / v);
                    }
               }

             if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                goto quit_progress;
          }
        break;
     case '3':                 /* ASCII RGB */
        for (y = 0; y < h; y++)
          {
             for (x = 0; x < w; x++)
               {
                  if (mm_getu(&rval))
                     goto quit;
                  if (mm_getu(&gval))
                     goto quit;
                  if (mm_getu(&bval))
                     goto quit;

                  if (v == 0 || v == 255)
                    {
                       *ptr2++ = 0xff000000 | (rval << 16) | (gval << 8) | bval;
                    }
                  else
                    {
                       *ptr2++ =
                          0xff000000 |
                          (((rval * 255) / v) << 16) |
                          (((gval * 255) / v) << 8) | ((bval * 255) / v);
                    }
               }

             if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                goto quit_progress;
          }
        break;
     case '4':                 /* binary 1bit monochrome */
        data = malloc((w + 7) / 8 * sizeof(uint8_t));
        if (!data)
           QUIT_WITH_RC(LOAD_OOM);

        ptr2 = im->data;
        for (y = 0; y < h; y++)
          {
             if (mm_read(data, (w + 7) / 8))
                goto quit;

             ptr = data;
             for (x = 0; x < w; x += 8)
               {
                  j = (w - x >= 8) ? 8 : w - x;
                  for (i = 0; i < j; i++)
                    {
                       if (ptr[0] & (0x80 >> i))
                          *ptr2 = 0xff000000;
                       else
                          *ptr2 = 0xffffffff;
                       ptr2++;
                    }
                  ptr++;
               }

             if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                goto quit_progress;
          }
        break;
     case '5':                 /* binary 8bit grayscale GGGGGGGG */
        data = malloc(1 * sizeof(uint8_t) * w);
        if (!data)
           QUIT_WITH_RC(LOAD_OOM);

        ptr2 = im->data;
        for (y = 0; y < h; y++)
          {
             if (mm_read(data, w * 1))
                goto quit;

             ptr = data;
             if (v == 0 || v == 255)
               {
                  for (x = 0; x < w; x++)
                    {
                       *ptr2 =
                          0xff000000 | (ptr[0] << 16) | (ptr[0] << 8) | ptr[0];
                       ptr2++;
                       ptr++;
                    }
               }
             else
               {
                  for (x = 0; x < w; x++)
                    {
                       *ptr2 =
                          0xff000000 |
                          (((ptr[0] * 255) / v) << 16) |
                          (((ptr[0] * 255) / v) << 8) | ((ptr[0] * 255) / v);
                       ptr2++;
                       ptr++;
                    }
               }

             if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                goto quit_progress;
          }
        break;
     case '6':                 /* 24bit binary RGBRGBRGB */
        data = malloc(3 * sizeof(uint8_t) * w);
        if (!data)
           QUIT_WITH_RC(LOAD_OOM);

        ptr2 = im->data;
        for (y = 0; y < h; y++)
          {
             if (mm_read(data, w * 3))
                goto quit;

             ptr = data;
             if (v == 0 || v == 255)
               {
                  for (x = 0; x < w; x++)
                    {
                       *ptr2 =
                          0xff000000 | (ptr[0] << 16) | (ptr[1] << 8) | ptr[2];
                       ptr2++;
                       ptr += 3;
                    }
               }
             else
               {
                  for (x = 0; x < w; x++)
                    {
                       *ptr2 =
                          0xff000000 |
                          (((ptr[0] * 255) / v) << 16) |
                          (((ptr[1] * 255) / v) << 8) | ((ptr[2] * 255) / v);
                       ptr2++;
                       ptr += 3;
                    }
               }

             if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                goto quit_progress;
          }
        break;
     case '7':                 /* XV's 8bit 332 format */
        data = malloc(1 * sizeof(uint8_t) * w);
        if (!data)
           QUIT_WITH_RC(LOAD_OOM);

        ptr2 = im->data;
        for (y = 0; y < h; y++)
          {
             if (mm_read(data, w * 1))
                goto quit;

             ptr = data;
             for (x = 0; x < w; x++)
               {
                  int                 r, g, b;

                  r = (*ptr >> 5) & 0x7;
                  g = (*ptr >> 2) & 0x7;
                  b = (*ptr) & 0x3;
                  *ptr2 =
                     0xff000000 |
                     (((r << 21) | (r << 18) | (r << 15)) & 0xff0000) |
                     (((g << 13) | (g << 10) | (g << 7)) & 0xff00) |
                     ((b << 6) | (b << 4) | (b << 2) | (b << 0));
                  ptr2++;
                  ptr++;
               }

             if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                goto quit_progress;
          }
        break;
     case '8':                 /* 24bit binary RGBARGBARGBA */
        data = malloc(4 * sizeof(uint8_t) * w);
        if (!data)
           QUIT_WITH_RC(LOAD_OOM);

        ptr2 = im->data;
        for (y = 0; y < h; y++)
          {
             if (mm_read(data, w * 4))
                goto quit;

             ptr = data;
             if (v == 0 || v == 255)
               {
                  for (x = 0; x < w; x++)
                    {
                       *ptr2 = PIXEL_ARGB(ptr[3], ptr[0], ptr[1], ptr[2]);
                       ptr2++;
                       ptr += 4;
                    }
               }
             else
               {
                  for (x = 0; x < w; x++)
                    {
                       *ptr2 =
                          PIXEL_ARGB((ptr[3] * 255) / v,
                                     (ptr[0] * 255) / v,
                                     (ptr[1] * 255) / v, (ptr[2] * 255) / v);
                       ptr2++;
                       ptr += 4;
                    }
               }

             if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                goto quit_progress;
          }
        break;
     default:
        goto quit;
      quit_progress:
        rc = LOAD_BREAK;
        goto quit;
     }

   rc = LOAD_SUCCESS;

 quit:
   free(idata);
   free(data);

   return rc;
}

static int
_save(ImlibImage * im)
{
   int                 rc;
   FILE               *f;
   uint8_t            *buf, *bptr;
   uint32_t           *ptr;
   int                 x, y;

   f = fopen(im->fi->name, "wb");
   if (!f)
      return LOAD_FAIL;

   rc = LOAD_FAIL;

   /* allocate a small buffer to convert image data */
   buf = malloc(im->w * 4 * sizeof(uint8_t));
   if (!buf)
      goto quit;

   ptr = im->data;

   /* if the image has a useful alpha channel */
   if (im->has_alpha)
     {
        fprintf(f, "P8\n" "# PNM File written by Imlib2\n" "%i %i\n" "255\n",
                im->w, im->h);
        for (y = 0; y < im->h; y++)
          {
             bptr = buf;
             for (x = 0; x < im->w; x++)
               {
                  uint32_t            pixel = *ptr++;

                  bptr[0] = PIXEL_R(pixel);
                  bptr[1] = PIXEL_G(pixel);
                  bptr[2] = PIXEL_B(pixel);
                  bptr[3] = PIXEL_A(pixel);
                  bptr += 4;
               }
             fwrite(buf, im->w * 4, 1, f);

             if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                goto quit_progress;
          }
     }
   else
     {
        fprintf(f, "P6\n" "# PNM File written by Imlib2\n" "%i %i\n" "255\n",
                im->w, im->h);
        for (y = 0; y < im->h; y++)
          {
             bptr = buf;
             for (x = 0; x < im->w; x++)
               {
                  uint32_t            pixel = *ptr++;

                  bptr[0] = PIXEL_R(pixel);
                  bptr[1] = PIXEL_G(pixel);
                  bptr[2] = PIXEL_B(pixel);
                  bptr += 3;
               }
             fwrite(buf, im->w * 3, 1, f);

             if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                goto quit_progress;
          }
     }

   rc = LOAD_SUCCESS;

 quit:
   /* finish off */
   free(buf);
   fclose(f);

   return rc;

 quit_progress:
   rc = LOAD_BREAK;
   goto quit;
}

IMLIB_LOADER(_formats, _load, _save);
