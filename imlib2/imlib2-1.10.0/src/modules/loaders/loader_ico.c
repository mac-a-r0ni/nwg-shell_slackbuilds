/*
 * ICO loader
 *
 * ICO(/BMP) file format:
 * https://en.wikipedia.org/wiki/ICO_(file_format)
 * https://en.wikipedia.org/wiki/BMP_file_format
 */
#include "config.h"
#include "Imlib2_Loader.h"

#include <limits.h>

#define DBG_PFX "LDR-ico"

static const char  *const _formats[] = { "ico" };

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

static void
mm_seek(unsigned int offs)
{
   mdata.dptr = mdata.data + offs;
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

/* The ICONDIR */
typedef struct {
   uint16_t            rsvd;
   uint16_t            type;
   uint16_t            icons;
} idir_t;

/* The ICONDIRENTRY */
typedef struct {
   uint8_t             width;
   uint8_t             height;
   uint8_t             colors;
   uint8_t             rsvd;
   uint16_t            planes;
   uint16_t            bpp;
   uint32_t            size;
   uint32_t            offs;
} ide_t;

/* The BITMAPINFOHEADER */
typedef struct {
   uint32_t            header_size;
   uint32_t            width;
   uint32_t            height;
   uint16_t            planes;
   uint16_t            bpp;
   uint32_t            compression;
   uint32_t            size;
   uint32_t            res_hor;
   uint32_t            res_ver;
   uint32_t            colors;
   uint32_t            colors_important;
} bih_t;

typedef struct {
   ide_t               ide;     /* ICONDIRENTRY     */
   bih_t               bih;     /* BITMAPINFOHEADER */

   unsigned short      w;
   unsigned short      h;

   uint32_t           *cmap;    /* Colormap (bpp <= 8) */
   uint8_t            *pxls;    /* Pixel data */
   uint8_t            *mask;    /* Bitmask    */
} ie_t;

typedef struct {
   idir_t              idir;    /* ICONDIR */
   ie_t               *ie;      /* Icon entries */
} ico_t;

static void
ico_delete(ico_t * ico)
{
   int                 i;

   if (ico->ie)
     {
        for (i = 0; i < ico->idir.icons; i++)
          {
             free(ico->ie[i].cmap);
             free(ico->ie[i].pxls);
             free(ico->ie[i].mask);
          }
        free(ico->ie);
     }
}

static void
ico_read_idir(ico_t * ico, int ino)
{
   ie_t               *ie;

   ie = &ico->ie[ino];

   mm_seek(sizeof(idir_t) + ino * sizeof(ide_t));
   if (mm_read(&ie->ide, sizeof(ie->ide)))
      return;

   ie->w = (ie->ide.width > 0) ? ie->ide.width : 256;
   ie->h = (ie->ide.height > 0) ? ie->ide.height : 256;

   SWAP_LE_16_INPLACE(ie->ide.planes);
   SWAP_LE_16_INPLACE(ie->ide.bpp);

   SWAP_LE_32_INPLACE(ie->ide.size);
   SWAP_LE_32_INPLACE(ie->ide.offs);

   DL("Entry %2d: Idir: WxHxD = %dx%dx%d, colors = %d\n",
      ino, ie->w, ie->h, ie->ide.bpp, ie->ide.colors);
}

static void
ico_read_icon(ico_t * ico, int ino)
{
   ie_t               *ie;
   unsigned int        size;

#ifdef WORDS_BIGENDIAN
   unsigned int        nr;
#endif

   ie = &ico->ie[ino];

   mm_seek(ie->ide.offs);
   if (mm_read(&ie->bih, sizeof(ie->bih)))
      goto bail;

   SWAP_LE_32_INPLACE(ie->bih.header_size);
   SWAP_LE_32_INPLACE(ie->bih.width);
   SWAP_LE_32_INPLACE(ie->bih.height);

   SWAP_LE_32_INPLACE(ie->bih.planes);
   SWAP_LE_32_INPLACE(ie->bih.bpp);

   SWAP_LE_32_INPLACE(ie->bih.compression);
   SWAP_LE_32_INPLACE(ie->bih.size);
   SWAP_LE_32_INPLACE(ie->bih.res_hor);
   SWAP_LE_32_INPLACE(ie->bih.res_ver);
   SWAP_LE_32_INPLACE(ie->bih.colors);
   SWAP_LE_32_INPLACE(ie->bih.colors_important);

   if (ie->bih.header_size != 40)
     {
        D("Entry %2d: Skipping entry with unknown format\n", ino);
        goto bail;
     }

   DL("Entry %2d: Icon: WxHxD = %dx%dx%d, colors = %d\n",
      ino, ie->w, ie->h, ie->bih.bpp, ie->bih.colors);

   if (ie->bih.width != ie->w || ie->bih.height != 2 * ie->h)
     {
        D("Entry %2d: Skipping entry with unexpected content (WxH = %dx%d/2)\n",
          ino, ie->bih.width, ie->bih.height);
        goto bail;
     }

   if (ie->bih.colors == 0 && ie->bih.bpp < 32)
      ie->bih.colors = 1U << ie->bih.bpp;

   switch (ie->bih.bpp)
     {
     case 1:
     case 4:
     case 8:
        DL("Allocating a %d slot colormap\n", ie->bih.colors);
        if (UINT_MAX / sizeof(uint32_t) < ie->bih.colors)
           goto bail;
        size = ie->bih.colors * sizeof(uint32_t);
        ie->cmap = malloc(size);
        if (ie->cmap == NULL)
           goto bail;
        if (mm_read(ie->cmap, size))
           goto bail;
#ifdef WORDS_BIGENDIAN
        for (nr = 0; nr < ie->bih.colors; nr++)
           SWAP_LE_32_INPLACE(ie->cmap[nr]);
#endif
        break;
     default:
        break;
     }

   if (!IMAGE_DIMENSIONS_OK(ie->w, ie->h) || ie->bih.bpp == 0 ||
       UINT_MAX / ie->bih.bpp < ie->w * ie->h)
      goto bail;

   size = ((ie->bih.bpp * ie->w + 31) / 32 * 4) * ie->h;
   ie->pxls = malloc(size);
   if (ie->pxls == NULL)
      goto bail;
   if (mm_read(ie->pxls, size))
      goto bail;
   DL("Pixel data size: %u\n", size);

   size = ((ie->w + 31) / 32 * 4) * ie->h;
   ie->mask = malloc(size);
   if (ie->mask == NULL)
      goto bail;
   if (mm_read(ie->mask, size))
      goto bail;
   DL("Mask  data size: %u\n", size);

   return;

 bail:
   ie->w = ie->h = 0;           /* Mark invalid */
}

static int
ico_data_get_bit(uint8_t * data, int w, int x, int y)
{
   int                 w32, res;

   w32 = (w + 31) / 32 * 4;     /* Line length in bytes */
   res = data[y * w32 + x / 8]; /* Byte containing bit */
   res >>= 7 - (x & 7);
   res &= 0x01;

   return res;
}

static int
ico_data_get_nibble(uint8_t * data, int w, int x, int y)
{
   int                 w32, res;

   w32 = (4 * w + 31) / 32 * 4; /* Line length in bytes */
   res = data[y * w32 + x / 2]; /* Byte containing nibble */
   res >>= 4 * (1 - (x & 1));
   res &= 0x0f;

   return res;
}

static int
_load(ImlibImage * im, int load_data)
{
   int                 rc;
   ico_t               ico;
   unsigned int        i;
   int                 ic, x, y, w, h, d, frame;
   uint32_t           *cmap;
   uint8_t            *pxls, *mask, *psrc;
   ie_t               *ie;
   uint32_t           *pdst;
   uint32_t            pixel;
   ImlibImageFrame    *pf;

   rc = LOAD_FAIL;

   mm_init(im->fi->fdata, im->fi->fsize);

   ico.ie = NULL;
   if (mm_read(&ico.idir, sizeof(ico.idir)))
      goto quit;

   SWAP_LE_16_INPLACE(ico.idir.rsvd);
   SWAP_LE_16_INPLACE(ico.idir.type);
   SWAP_LE_16_INPLACE(ico.idir.icons);

   if (ico.idir.rsvd != 0 ||
       (ico.idir.type != 1 && ico.idir.type != 2) || ico.idir.icons <= 0)
      goto quit;

   ico.ie = calloc(ico.idir.icons, sizeof(ie_t));
   if (!ico.ie)
      QUIT_WITH_RC(LOAD_OOM);

   D("Loading '%s' Nicons = %d\n", im->fi->name, ico.idir.icons);

   for (i = 0; i < ico.idir.icons; i++)
     {
        ico_read_idir(&ico, i);
        ico_read_icon(&ico, i);
     }

   rc = LOAD_BADIMAGE;          /* Format accepted */

   frame = im->frame;
   if (frame > 0)
     {
        if (frame > 1 && frame > ico.idir.icons)
           QUIT_WITH_RC(LOAD_BADFRAME);

        pf = __imlib_GetFrame(im);
        if (!pf)
           QUIT_WITH_RC(LOAD_OOM);

        pf->frame_count = ico.idir.icons;
     }

   ic = frame - 1;
   if (ic < 0)
     {
        /* Select default: Find icon with largest size and depth */
        ic = y = d = 0;
        for (x = 0; x < ico.idir.icons; x++)
          {
             ie = &ico.ie[x];
             w = ie->w;
             h = ie->h;
             if (w * h < y)
                continue;
             if (w * h == y && ie->bih.bpp < d)
                continue;
             ic = x;
             y = w * h;
             d = ie->bih.bpp;
          }
     }

   ie = &ico.ie[ic];

   w = ie->w;
   h = ie->h;
   if (!IMAGE_DIMENSIONS_OK(w, h))
      goto quit;

   im->w = w;
   im->h = h;

   im->has_alpha = 1;

   if (!load_data)
      QUIT_WITH_RC(LOAD_SUCCESS);

   if (!__imlib_AllocateData(im))
      QUIT_WITH_RC(LOAD_OOM);

   D("Loading icon %d: WxHxD=%dx%dx%d\n", ic, w, h, ie->bih.bpp);

   cmap = ie->cmap;
   pxls = ie->pxls;
   mask = ie->mask;

   pdst = im->data + (h - 1) * w;       /* Start in lower left corner */

   switch (ie->bih.bpp)
     {
     case 1:
        for (y = 0; y < h; y++, pdst -= 2 * w)
          {
             for (x = 0; x < w; x++)
               {
                  pixel = cmap[ico_data_get_bit(pxls, w, x, y)];
                  if (ico_data_get_bit(mask, w, x, y) == 0)
                     pixel |= 0xff000000;

                  *pdst++ = pixel;
               }
          }
        break;

     case 4:
        for (y = 0; y < h; y++, pdst -= 2 * w)
          {
             for (x = 0; x < w; x++)
               {
                  pixel = cmap[ico_data_get_nibble(pxls, w, x, y)];
                  if (ico_data_get_bit(mask, w, x, y) == 0)
                     pixel |= 0xff000000;

                  *pdst++ = pixel;
               }
          }
        break;

     case 8:
        for (y = 0; y < h; y++, pdst -= 2 * w)
          {
             for (x = 0; x < w; x++)
               {
                  pixel = cmap[pxls[y * w + x]];
                  if (ico_data_get_bit(mask, w, x, y) == 0)
                     pixel |= 0xff000000;

                  *pdst++ = pixel;
               }
          }
        break;

     default:
        for (y = 0; y < h; y++, pdst -= 2 * w)
          {
             for (x = 0; x < w; x++)
               {
                  psrc = &pxls[(y * w + x) * ie->bih.bpp / 8];

                  pixel = PIXEL_ARGB(0, psrc[2], psrc[1], psrc[0]);
                  if (ie->bih.bpp == 32)
                     pixel |= psrc[3] << 24;
                  else if (ico_data_get_bit(mask, w, x, y) == 0)
                     pixel |= 0xff000000;

                  *pdst++ = pixel;
               }
          }
        break;
     }

   rc = LOAD_SUCCESS;

   if (im->lc)
      __imlib_LoadProgressRows(im, 0, im->h);

 quit:
   ico_delete(&ico);

   return rc;
}

IMLIB_LOADER(_formats, _load, NULL);
