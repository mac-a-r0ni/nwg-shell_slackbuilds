#include "config.h"
#include "Imlib2_Loader.h"

#include <setjmp.h>
#include <tiffio.h>

#define DBG_PFX "LDR-tiff"

static const char  *const _formats[] = { "tiff", "tif" };

#define DD(fmt...)  DC(0x80, fmt)

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

static              tmsize_t
_tiff_read(thandle_t ctx, void *buf, tmsize_t len)
{
   DD("%s: len=%ld\n", __func__, len);

   if (mdata.dptr + len > mdata.data + mdata.size)
      return 0;                 /* Out of data */

   memcpy(buf, mdata.dptr, len);
   mdata.dptr += len;

   return len;
}

static              tmsize_t
_tiff_write(thandle_t ctx, void *buf, tmsize_t len)
{
   DD("%s: len=%ld\n", __func__, len);

   return 0;
}

static              toff_t
_tiff_seek(thandle_t ctx, toff_t offs, int whence)
{
   const unsigned char *dptr;

   DD("%s: offs=%ld, whence=%d\n", __func__, offs, whence);

   switch (whence)
     {
     default:
        return -1;
     case SEEK_SET:
        dptr = mdata.data + offs;
        break;
     case SEEK_CUR:
        dptr = mdata.data += offs;
        break;
     case SEEK_END:
        dptr = mdata.data + mdata.size + offs;
        break;
     }

   if (dptr > mdata.data + mdata.size)
      return -1;                /* Out of data */

   mdata.dptr = dptr;

   return mdata.dptr - mdata.data;
}

static int
_tiff_close(thandle_t ctx)
{
   DD("%s\n", __func__);

   return 0;
}

static              toff_t
_tiff_size(thandle_t ctx)
{
   DD("%s: size=%d\n", __func__, mdata.size);

   return mdata.size;
}

static int
_tiff_map(thandle_t ctx, void **base, toff_t * size)
{
   DD("%s\n", __func__);

   *base = (void *)mdata.data;
   *size = mdata.size;

   return 1;
}
static void
_tiff_unmap(thandle_t ctx, void *base, toff_t size)
{
   DD("%s\n", __func__);
}

/* This is a wrapper data structure for TIFFRGBAImage, so that data can be */
/* passed into the callbacks. More elegent, I think, than a bunch of globals */

typedef struct {
   TIFFRGBAImage       rgba;
   tileContigRoutine   put_contig;
   tileSeparateRoutine put_separate;
   ImlibImage         *image;
} TIFFRGBAImage_Extra;

#define PIM(_x, _y) buffer + ((_x) + image_width * (_y))

static void
raster(TIFFRGBAImage_Extra * img, uint32_t * rast,
       uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
   uint32_t            image_width, image_height;
   uint32_t           *pixel, pixel_value;
   uint32_t            i, j, k;
   uint32_t           *buffer_pixel, *buffer = img->image->data;
   int                 alpha_premult;
   int                 a, r, g, b;

   image_width = img->image->w;
   image_height = img->image->h;

#if 0
   printf("%s: x,y=%d,%d wxh=%dx%d (image %dx%d)\n", __func__,
          x, y, w, h, image_width, image_height);
#endif

   /* rast seems to point to the beginning of the last strip processed */
   /* so you need use negative offsets. Bizzare. Someone please check this */
   /* I don't understand why, but that seems to be what's going on. */
   /* libtiff needs better docs! */

   alpha_premult = img->rgba.alpha == EXTRASAMPLE_UNASSALPHA;

   switch (img->rgba.orientation)
     {
     default:
     case ORIENTATION_TOPLEFT:
     case ORIENTATION_TOPRIGHT:
        for (j = 0; j < h; j++)
          {
             pixel = rast - j * image_width;

             for (i = 0; i < w; i++)
               {
                  pixel_value = *pixel++;
                  a = TIFFGetA(pixel_value);
                  r = TIFFGetR(pixel_value);
                  g = TIFFGetG(pixel_value);
                  b = TIFFGetB(pixel_value);
                  if ((a > 0) && (a < 255) && (alpha_premult))
                    {
                       r = (r * 255) / a;
                       g = (g * 255) / a;
                       b = (b * 255) / a;
                    }
                  k = x + i;
                  if (img->rgba.orientation == ORIENTATION_TOPRIGHT)
                     k = image_width - 1 - k;
                  buffer_pixel = PIM(k, image_height - 1 - (y - j));
                  *buffer_pixel = PIXEL_ARGB(a, r, g, b);
               }
          }
        break;
     case ORIENTATION_BOTRIGHT:
     case ORIENTATION_BOTLEFT:
        for (j = 0; j < h; j++)
          {
             pixel = rast + j * image_width;

             for (i = 0; i < w; i++)
               {
                  pixel_value = *pixel++;
                  a = TIFFGetA(pixel_value);
                  r = TIFFGetR(pixel_value);
                  g = TIFFGetG(pixel_value);
                  b = TIFFGetB(pixel_value);
                  if ((a > 0) && (a < 255) && (alpha_premult))
                    {
                       r = (r * 255) / a;
                       g = (g * 255) / a;
                       b = (b * 255) / a;
                    }
                  k = x + i;
                  if (img->rgba.orientation == ORIENTATION_BOTRIGHT)
                     k = image_width - 1 - k;
                  buffer_pixel = PIM(k, image_height - 1 - (y + j));
                  *buffer_pixel = PIXEL_ARGB(a, r, g, b);
               }
          }
        break;

     case ORIENTATION_LEFTTOP:
     case ORIENTATION_RIGHTTOP:
        for (i = 0; i < h; i++)
          {
             pixel = rast - i * image_height;

             for (j = 0; j < w; j++)
               {
                  pixel_value = *pixel++;
                  a = TIFFGetA(pixel_value);
                  r = TIFFGetR(pixel_value);
                  g = TIFFGetG(pixel_value);
                  b = TIFFGetB(pixel_value);
                  if ((a > 0) && (a < 255) && (alpha_premult))
                    {
                       r = (r * 255) / a;
                       g = (g * 255) / a;
                       b = (b * 255) / a;
                    }
                  k = y - i;
                  if (img->rgba.orientation == ORIENTATION_LEFTTOP)
                     k = image_width - 1 - k;
                  buffer_pixel = PIM(k, x + j);
                  *buffer_pixel = PIXEL_ARGB(a, r, g, b);
               }
          }
        break;
     case ORIENTATION_RIGHTBOT:
     case ORIENTATION_LEFTBOT:
        for (i = 0; i < h; i++)
          {
             pixel = rast + i * image_height;

             for (j = 0; j < w; j++)
               {
                  pixel_value = *pixel++;
                  a = TIFFGetA(pixel_value);
                  r = TIFFGetR(pixel_value);
                  g = TIFFGetG(pixel_value);
                  b = TIFFGetB(pixel_value);
                  if ((a > 0) && (a < 255) && (alpha_premult))
                    {
                       r = (r * 255) / a;
                       g = (g * 255) / a;
                       b = (b * 255) / a;
                    }
                  k = y + i;
                  if (img->rgba.orientation == ORIENTATION_RIGHTBOT)
                     k = image_width - 1 - k;
                  buffer_pixel = PIM(k, image_height - 1 - (x + j));
                  *buffer_pixel = PIXEL_ARGB(a, r, g, b);
               }
          }
        break;
     }

   if (img->image->lc)
     {
        /* for tile based images, we just progress each tile because */
        /* of laziness. Couldn't think of a good way to do this */

        switch (img->rgba.orientation)
          {
          default:
          case ORIENTATION_TOPLEFT:
             if (w >= image_width)
               {
                  __imlib_LoadProgressRows(img->image, image_height - y - 1, h);
               }
             else
               {
                  /* for tile based images, we just progress each tile because */
                  /* of laziness. Couldn't think of a good way to do this */
                  y = image_height - 1 - y;
                  goto progress_a;
               }
             break;
          case ORIENTATION_TOPRIGHT:
             y = image_height - 1 - y;
             goto progress_a;
          case ORIENTATION_BOTRIGHT:
             y = image_height - y - h;
             goto progress_a;
          case ORIENTATION_BOTLEFT:
             y = image_height - y - h;
             goto progress_a;
           progress_a:
             __imlib_LoadProgress(img->image, x, y, w, h);
             break;

          case ORIENTATION_LEFTTOP:
             y = image_width - 1 - y;
             goto progress_b;
          case ORIENTATION_RIGHTTOP:
             y = y + 1 - h;
             goto progress_b;
          case ORIENTATION_RIGHTBOT:
             y = image_width - y - h;
             goto progress_b;
          case ORIENTATION_LEFTBOT:
             goto progress_b;
           progress_b:
             __imlib_LoadProgress(img->image, y, x, h, w);
             break;
          }
     }
}

static void
put_contig_and_raster(TIFFRGBAImage * img, uint32_t * rast,
                      uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                      int32_t fromskew, int32_t toskew, unsigned char *cp)
{
   ((TIFFRGBAImage_Extra *) img)->put_contig(img, rast, x, y, w, h,
                                             fromskew, toskew, cp);
   raster((TIFFRGBAImage_Extra *) img, rast, x, y, w, h);
}

static void
put_separate_and_raster(TIFFRGBAImage * img, uint32_t * rast,
                        uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                        int32_t fromskew, int32_t toskew,
                        unsigned char *r, unsigned char *g, unsigned char *b,
                        unsigned char *a)
{
   ((TIFFRGBAImage_Extra *) img)->put_separate(img, rast, x, y, w, h,
                                               fromskew, toskew, r, g, b, a);
   raster((TIFFRGBAImage_Extra *) img, rast, x, y, w, h);
}

static int
_load(ImlibImage * im, int load_data)
{
   int                 rc;
   TIFF               *tif = NULL;
   uint16_t            magic_number;
   TIFFRGBAImage_Extra rgba_image;
   uint32_t           *rast = NULL;
   char                txt[1024];

   rc = LOAD_FAIL;
   rgba_image.image = NULL;

   /* Do initial signature check */
#define TIFF_BYTES_TO_CHECK sizeof(magic_number)

   if (im->fi->fsize < (int)TIFF_BYTES_TO_CHECK)
      return rc;

   magic_number = *(const uint16_t *)im->fi->fdata;

   if (magic_number != TIFF_BIGENDIAN && magic_number != TIFF_LITTLEENDIAN)
      return rc;

   mm_init(im->fi->fdata, im->fi->fsize);

   tif = TIFFClientOpen(im->fi->name, "r", NULL, _tiff_read, _tiff_write,
                        _tiff_seek, _tiff_close, _tiff_size,
                        _tiff_map, _tiff_unmap);
   if (!tif)
      goto quit;

   strcpy(txt, "Cannot be processed by libtiff");
   if (!TIFFRGBAImageOK(tif, txt))
      goto quit;

   rc = LOAD_BADIMAGE;          /* Format accepted */

   strcpy(txt, "Cannot begin reading tiff");
   if (!TIFFRGBAImageBegin((TIFFRGBAImage *) & rgba_image, tif, 1, txt))
      goto quit;

   rgba_image.image = im;

   if (!rgba_image.rgba.put.any)
     {
        fprintf(stderr, "imlib2-tiffloader: No put function");
        goto quit;
     }

   switch (rgba_image.rgba.orientation)
     {
     default:
     case ORIENTATION_TOPLEFT:
     case ORIENTATION_TOPRIGHT:
     case ORIENTATION_BOTRIGHT:
     case ORIENTATION_BOTLEFT:
        im->w = rgba_image.rgba.width;
        im->h = rgba_image.rgba.height;
        break;
     case ORIENTATION_LEFTTOP:
     case ORIENTATION_RIGHTTOP:
     case ORIENTATION_RIGHTBOT:
     case ORIENTATION_LEFTBOT:
        im->w = rgba_image.rgba.height;
        im->h = rgba_image.rgba.width;
        break;
     }
   if (!IMAGE_DIMENSIONS_OK(im->w, im->h))
      goto quit;

   im->has_alpha = rgba_image.rgba.alpha != EXTRASAMPLE_UNSPECIFIED;

   if (!load_data)
      QUIT_WITH_RC(LOAD_SUCCESS);

   /* Load data */

   if (!__imlib_AllocateData(im))
      QUIT_WITH_RC(LOAD_OOM);

   rast = _TIFFmalloc(sizeof(uint32_t) * im->w * im->h);
   if (!rast)
     {
        fprintf(stderr, "imlib2-tiffloader: Out of memory\n");
        QUIT_WITH_RC(LOAD_OOM);
     }

   if (rgba_image.rgba.isContig)
     {
        rgba_image.put_contig = rgba_image.rgba.put.contig;
        rgba_image.rgba.put.contig = put_contig_and_raster;
     }
   else
     {
        rgba_image.put_separate = rgba_image.rgba.put.separate;
        rgba_image.rgba.put.separate = put_separate_and_raster;
     }

   if (!TIFFRGBAImageGet((TIFFRGBAImage *) & rgba_image, rast,
                         rgba_image.rgba.width, rgba_image.rgba.height))
      goto quit;

   rc = LOAD_SUCCESS;

 quit:
   if (rast)
      _TIFFfree(rast);
   if (rgba_image.image)
      TIFFRGBAImageEnd((TIFFRGBAImage *) & rgba_image);
   if (tif)
      TIFFClose(tif);

   return rc;
}

/* this seems to work, except the magic number isn't written. I'm guessing */
/* this is a problem in libtiff */

static int
_save(ImlibImage * im)
{
   int                 rc;
   TIFF               *tif = NULL;
   uint8_t            *buf = NULL;
   uint32_t            pixel, *data = im->data;
   double              alpha_factor;
   int                 x, y;
   uint8_t             r, g, b, a = 0;
   int                 has_alpha = im->has_alpha;
   int                 compression_type;
   int                 i;
   ImlibImageTag      *tag;

   tif = TIFFOpen(im->fi->name, "w");
   if (!tif)
      return LOAD_FAIL;

   rc = LOAD_FAIL;

   /* None of the TIFFSetFields are checked for errors, but since they */
   /* shouldn't fail, this shouldn't be a problem */

   TIFFSetField(tif, TIFFTAG_IMAGELENGTH, im->h);
   TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, im->w);
   TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
   TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
   TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
   TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);

   /* look for tags attached to image to get extra parameters like quality */
   /* settings etc. - this is the "api" to hint for extra information for */
   /* saver modules */

   /* compression */
   compression_type = COMPRESSION_ADOBE_DEFLATE;
   tag = __imlib_GetTag(im, "compression_type");
   if (tag)
     {
        switch (tag->val)
          {
          default:
             break;
          case COMPRESSION_NONE:
          case COMPRESSION_CCITTRLE:
          case COMPRESSION_CCITTFAX3:
          case COMPRESSION_CCITTFAX4:
          case COMPRESSION_LZW:
          case COMPRESSION_OJPEG:
          case COMPRESSION_JPEG:
          case COMPRESSION_NEXT:
          case COMPRESSION_CCITTRLEW:
          case COMPRESSION_PACKBITS:
          case COMPRESSION_THUNDERSCAN:
          case COMPRESSION_IT8CTPAD:
          case COMPRESSION_IT8LW:
          case COMPRESSION_IT8MP:
          case COMPRESSION_IT8BL:
          case COMPRESSION_PIXARFILM:
          case COMPRESSION_PIXARLOG:
          case COMPRESSION_DEFLATE:
          case COMPRESSION_ADOBE_DEFLATE:
          case COMPRESSION_DCS:
          case COMPRESSION_JBIG:
          case COMPRESSION_SGILOG:
          case COMPRESSION_SGILOG24:
             compression_type = tag->val;
             break;
          }
     }
   TIFFSetField(tif, TIFFTAG_COMPRESSION, compression_type);

   if (has_alpha)
     {
        uint16_t            extras[] = { EXTRASAMPLE_ASSOCALPHA };
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 4);
        TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, 1, extras);
     }
   else
     {
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
     }
   TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
   TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, 0));

   buf = _TIFFmalloc(TIFFScanlineSize(tif));
   if (!buf)
      goto quit;

   for (y = 0; y < im->h; y++)
     {
        i = 0;
        for (x = 0; x < im->w; x++)
          {
             pixel = data[(y * im->w) + x];

             r = PIXEL_R(pixel);
             g = PIXEL_G(pixel);
             b = PIXEL_B(pixel);
             if (has_alpha)
               {
                  /* TIFF makes you pre-mutiply the rgb components by alpha */
                  a = PIXEL_A(pixel);
                  alpha_factor = ((double)a / 255.0);
                  r *= alpha_factor;
                  g *= alpha_factor;
                  b *= alpha_factor;
               }

             /* This might be endian dependent */
             buf[i++] = r;
             buf[i++] = g;
             buf[i++] = b;
             if (has_alpha)
                buf[i++] = a;
          }

        if (!TIFFWriteScanline(tif, buf, y, 0))
           goto quit;

        if (im->lc && __imlib_LoadProgressRows(im, y, 1))
           QUIT_WITH_RC(LOAD_BREAK);
     }

   rc = LOAD_SUCCESS;

 quit:
   if (buf)
      _TIFFfree(buf);
   if (tif)
      TIFFClose(tif);

   return rc;
}

IMLIB_LOADER(_formats, _load, _save);
