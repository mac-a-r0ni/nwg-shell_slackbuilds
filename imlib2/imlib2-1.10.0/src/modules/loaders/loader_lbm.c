/*------------------------------------------------------------------------------
 * Reads regular Amiga IFF ILBM files.
 *
 * Supports IMLIB2_LBM_NOMASK environment variable. If this is set to "1", then
 * a transparency mask in an image will be ignored. On the Amiga a mask is often
 * applied only when loading a brush rather than a picture, but this loader has
 * no way to tell when the user wants this behaviour from the picture alone.
 *
 * Author:  John Bickers <jbickers@ihug.co.nz>
 * Since:   2004-08-21
 * Version: 2004-08-28
 *------------------------------------------------------------------------------*/

#include "config.h"
#include "Imlib2_Loader.h"

#define DBG_PFX "LDR-lbm"

static const char  *const _formats[] = { "iff", "ilbm", "lbm" };

#define L2RLONG(a) ((((int)((a)[0]) & 0xff) << 24) + (((int)((a)[1]) & 0xff) << 16) + (((int)((a)[2]) & 0xff) << 8) + ((int)((a)[3]) & 0xff))
#define L2RWORD(a) ((((int)((a)[0]) & 0xff) << 8) + ((int)((a)[1]) & 0xff))

typedef struct {
   int                 size;
   const unsigned char *data;
} CHUNK;

typedef struct {
   CHUNK               bmhd;
   CHUNK               camg;
   CHUNK               cmap;
   CHUNK               ctbl;
   CHUNK               sham;
   CHUNK               body;
   unsigned char      *cmap_alloc;      /* Modified colormap */

   int                 depth;
   int                 mask;
   int                 ham;
   int                 hbrite;

   int                 row;

   int                 offset;
   int                 count;
   int                 rle;
} ILBM;

/*------------------------------------------------------------------------------
 * Frees memory allocated as part of an ILBM structure.
 *------------------------------------------------------------------------------*/
static void
freeilbm(ILBM * ilbm)
{
   free(ilbm->cmap_alloc);
}

#define mm_check(p) ((const unsigned char *)(p) <= fptr + size)

/*------------------------------------------------------------------------------
 * Reads the given chunks out of a file, returns 0 if the file had a problem.
 *
 * Format FORMsizeILBMtag.size....tag.size....tag.size....
 *------------------------------------------------------------------------------*/
static int
loadchunks(const unsigned char *fptr, unsigned int size, ILBM * ilbm, int full)
{
   CHUNK              *c;
   int                 formsize, z;
   int                 ok;
   const unsigned char *buf;

   ok = 0;

   buf = fptr;
   if (!mm_check(buf + 12))
      return ok;

   if (memcmp(buf, "FORM", 4) != 0 || memcmp(buf + 8, "ILBM", 4) != 0)
      return ok;

   formsize = L2RLONG(buf + 4);

   D("%s: %.4s %.4s formsize=%d\n", __func__, buf, buf + 8, formsize);

   buf += 12;

   while (1)
     {
        if (buf >= fptr + formsize + 8)
           break;
        if (!mm_check(buf + 8))
           break;               /* Error or short file. */

        z = L2RLONG(buf + 4);
        if (z < 0)
           break;               /* Corrupt file. */

        D("%s: %.4s %d\n", __func__, buf, z);

        c = NULL;
        if (!memcmp(buf, "BMHD", 4))
           c = &(ilbm->bmhd);
        else if (full)
          {
             if (!memcmp(buf, "CAMG", 4))
                c = &(ilbm->camg);
             else if (!memcmp(buf, "CMAP", 4))
                c = &(ilbm->cmap);
             else if (!memcmp(buf, "CTBL", 4))
                c = &(ilbm->ctbl);
             else if (!memcmp(buf, "SHAM", 4))
                c = &(ilbm->sham);
             else if (!memcmp(buf, "BODY", 4))
                c = &(ilbm->body);
          }

        buf += 8;

        if (c && !c->data)
          {
             c->size = z;
             if (!mm_check(buf + c->size))
                break;
             c->data = buf;

             if (!full)
               {                /* Only BMHD required. */
                  ok = 1;
                  break;
               }
          }

        buf += z;
     }

   /* File may end strangely, especially if body size is uneven, but it's
    * ok if we have the chunks we want. !full check is already done. */
   if (ilbm->bmhd.data && ilbm->body.data)
      ok = 1;

   return ok;
}

/*------------------------------------------------------------------------------
 * Unpacks a row of possibly RLE data at a time.
 *
 * RLE compression depends on a count byte, followed by data bytes.
 *
 * 0x80 means skip.
 * 0xff to 0x81 means repeat one data byte (256 - count) + 1 times.
 * 0x00 to 0x7f means copy count + 1 data bytes.
 *
 * In theory RLE compression is not supposed to create runs across scanlines.
 *------------------------------------------------------------------------------*/
static void
bodyrow(unsigned char *p, int z, ILBM * ilbm)
{
   int                 i, x, w;
   unsigned char       b;

   if (ilbm->offset >= ilbm->body.size)
     {
        memset(p, 0, z);
        return;
     }

   if (!ilbm->rle)
     {
        w = ilbm->body.size - ilbm->offset;
        if (w > z)
           w = z;
        memcpy(p, ilbm->body.data + ilbm->offset, w);
        if (w < z)
           memset(p + w, 0, z - w);
        ilbm->offset += w;
        return;
     }

#ifdef __clang_analyzer__
   memset(p, 0, z);
#endif

   for (i = 0; i < z; i += w)
     {
        if (ilbm->offset < ilbm->body.size)
          {
             b = ilbm->body.data[ilbm->offset++];
             while (b == 0x80 && ilbm->offset < ilbm->body.size)
                b = ilbm->body.data[ilbm->offset++];
          }

        if (ilbm->offset >= ilbm->body.size)
          {
             w = z - i;
             memset(p + i, 0, w);
          }
        else if (b & 0x80)
          {
             w = (0x100 - b) + 1;
             if (w > z - i)
                w = z - i;

             b = ilbm->body.data[ilbm->offset++];
             memset(p + i, b, w);
          }
        else
          {
             w = (b & 0x7f) + 1;
             if (w > ilbm->body.size - ilbm->offset)
                w = ilbm->body.size - ilbm->offset;
             x = (w <= z - i) ? w : z - i;
             memcpy(p + i, ilbm->body.data + ilbm->offset, x);
             ilbm->offset += w;
             w = x;
          }
     }
}

/*------------------------------------------------------------------------------
 * Shifts a value to produce an 8-bit colour gun, and fills in the lower bits
 * from the high bits of the value so that, for example, 4-bit 0x0f scales to
 * 0xff, or 1-bit 0x01 scales to 0xff.
 *------------------------------------------------------------------------------*/
static unsigned char
scalegun(unsigned char v, int sl)
{
   int                 sr;

   switch (sl)
     {
     case 1:
     case 2:
     case 3:
        sr = 8 - sl;
        return (v << sl) | (v >> sr);

     case 4:
        return (v << 4) | v;

     case 5:
        return v * 0x24;

     case 6:
        return v * 0x55;

     case 7:
        return v * 0xff;
     }
   return v;
}

/*------------------------------------------------------------------------------
 * Scales the colours in a CMAP chunk if they all look like 4-bit colour, so
 * that they use all 8-bits. This is done by copying the high nybble into the
 * low nybble, so for example 0xf0 becomes 0xff.
 *------------------------------------------------------------------------------*/
static void
scalecmap(ILBM * ilbm)
{
   int                 i;

   if (!ilbm->cmap.data || ilbm->cmap.size <= 0)
      return;

   for (i = 0; i < ilbm->cmap.size; i++)
      if (ilbm->cmap.data[i] & 0x0f)
         return;

   ilbm->cmap_alloc = malloc(ilbm->cmap.size);

   for (i = 0; i < ilbm->cmap.size; i++)
      ilbm->cmap_alloc[i] = ilbm->cmap.data[i] | ilbm->cmap.data[i] >> 4;

   ilbm->cmap.data = ilbm->cmap_alloc;
}

/*------------------------------------------------------------------------------
 * Deplanes and converts an array of bitplanes to a single scanline of uint32_t
 * (unsigned int) values. uint32_t is ARGB.
 *------------------------------------------------------------------------------*/
static void
deplane(uint32_t * row, int w, ILBM * ilbm, unsigned char *plane[])
{
   unsigned int        l, r, g, b, a;
   int                 i, o, x;
   unsigned char       bit, v, h;
   const unsigned char *pal;

   pal = NULL;
   if (ilbm->sham.data && ilbm->sham.size >= 2 + (ilbm->row + 1) * 2 * 16)
      pal = ilbm->sham.data + 2 + ilbm->row * 2 * 16;
   if (ilbm->ctbl.data && ilbm->ctbl.size >= (ilbm->row + 1) * 2 * 16)
      pal = ilbm->ctbl.data + ilbm->row * 2 * 16;

   if (ilbm->ham)
      r = g = b = 0;

   bit = 0x80;
   o = 0;
   for (x = 0; x < w; x++)
     {
        l = 0;
        for (i = ilbm->depth - 1; i >= 0; i--)
          {
             l = l << 1;
             if (plane[i][o] & bit)
                l = l | 1;
          }
        a = (ilbm->mask == 0
             || (ilbm->mask == 1 && (plane[ilbm->depth][o] & bit))
             || ilbm->mask == 2) ? 0xff : 0x00;

        if (ilbm->depth == 32)
          {
             a = (l >> 24) & 0xff;
             b = (l >> 16) & 0xff;
             g = (l >> 8) & 0xff;
             r = l & 0xff;
          }
        else if (ilbm->depth == 24)
          {
             b = (l >> 16) & 0xff;
             g = (l >> 8) & 0xff;
             r = l & 0xff;
          }
        else if (ilbm->ham)
          {
             v = l & ((1 << (ilbm->depth - 2)) - 1);
             h = (l & ~v) >> (ilbm->depth - 2);

             if (h == 0x00)
               {
                  if (!pal)
                    {
                       if ((v + 1) * 3 <= ilbm->cmap.size)
                         {
                            r = ilbm->cmap.data[v * 3];
                            g = ilbm->cmap.data[v * 3 + 1];
                            b = ilbm->cmap.data[v * 3 + 2];
                         }
                       else
                          r = g = b = 0;
                    }
                  else
                    {
                       r = scalegun(pal[v * 2] & 0x0f, 4);
                       g = scalegun((pal[v * 2 + 1] & 0xf0) >> 4, 4);
                       b = scalegun((pal[v * 2 + 1] & 0x0f), 4);
                    }
               }
             else if (h == 0x01)
                b = scalegun(v, 8 - (ilbm->depth - 2));
             else if (h == 0x02)
                r = scalegun(v, 8 - (ilbm->depth - 2));
             else
                g = scalegun(v, 8 - (ilbm->depth - 2));
          }
        else if (ilbm->hbrite)
          {
             v = l & ((1 << (ilbm->depth - 1)) - 1);
             h = (l & ~v) >> (ilbm->depth - 1);

             if (!pal)
               {
                  if ((v + 1) * 3 <= ilbm->cmap.size)
                    {
                       r = ilbm->cmap.data[v * 3];
                       g = ilbm->cmap.data[v * 3 + 1];
                       b = ilbm->cmap.data[v * 3 + 2];
                    }
                  else
                     r = g = b = 0;
               }
             else
               {
                  r = scalegun(pal[v * 2] & 0x0f, 4);
                  g = scalegun((pal[v * 2 + 1] & 0xf0) >> 4, 4);
                  b = scalegun((pal[v * 2 + 1] & 0x0f), 4);
               }

             if (h)
               {
                  r = r >> 1;
                  g = g >> 1;
                  b = b >> 1;
               }

             if (ilbm->mask == 2 && v == L2RWORD(ilbm->bmhd.data + 12))
                a = 0x00;
          }
        else if (ilbm->cmap.size == 0 && !pal)
          {
             v = l & ((1 << ilbm->depth) - 1);
             r = scalegun(v, ilbm->depth);
             g = r;
             b = r;
          }
        else
          {
             v = l & 0xff;
             if (!pal)
               {
                  if ((v + 1) * 3 <= ilbm->cmap.size)
                    {
                       r = ilbm->cmap.data[v * 3];
                       g = ilbm->cmap.data[v * 3 + 1];
                       b = ilbm->cmap.data[v * 3 + 2];
                    }
                  else
                     r = g = b = 0;
               }
             else
               {
                  r = scalegun(pal[v * 2] & 0x0f, 4);
                  g = scalegun((pal[v * 2 + 1] & 0xf0) >> 4, 4);
                  b = scalegun((pal[v * 2 + 1] & 0x0f), 4);
               }

             if (ilbm->mask == 2 && v == L2RWORD(ilbm->bmhd.data + 12))
                a = 0x00;
          }

        row[x] = PIXEL_ARGB(a, r, g, b);

        bit = bit >> 1;
        if (bit == 0)
          {
             o++;
             bit = 0x80;
          }
     }
}

/*------------------------------------------------------------------------------
 * Loads an image. If load_data is non-zero then the file is fully loaded,
 * otherwise only the width and height are read.
 *
 * Imlib2 doesn't support reading comment chunks like ANNO.
 *------------------------------------------------------------------------------*/
static int
_load(ImlibImage * im, int load_data)
{
   int                 rc;
   char               *env;
   int                 i, n, y, z;
   unsigned char      *plane[40];
   ILBM                ilbm;

   rc = LOAD_FAIL;

   plane[0] = NULL;
   memset(&ilbm, 0, sizeof(ilbm));

  /*----------
   * Load the chunk(s) we're interested in. If load_data is not true, then we only
   * want the image size and format.
   *----------*/
   if (!loadchunks(im->fi->fdata, im->fi->fsize, &ilbm, load_data))
      goto quit;

   rc = LOAD_BADIMAGE;          /* Format accepted */

  /*----------
   * Use and check header.
   *----------*/

   if (ilbm.bmhd.size < 20)
      goto quit;

   im->w = L2RWORD(ilbm.bmhd.data);
   im->h = L2RWORD(ilbm.bmhd.data + 2);
   if (!IMAGE_DIMENSIONS_OK(im->w, im->h))
      goto quit;

   ilbm.depth = ilbm.bmhd.data[8];
   if (ilbm.depth < 1
       || (ilbm.depth > 8 && ilbm.depth != 24 && ilbm.depth != 32))
      goto quit;                /* Only 1 to 8, 24, or 32 planes. */

   ilbm.rle = ilbm.bmhd.data[10];
   if (ilbm.rle < 0 || ilbm.rle > 1)
      goto quit;                /* Only NONE or RLE compression. */

   ilbm.mask = ilbm.bmhd.data[9];

   im->has_alpha = ilbm.mask != 0 || ilbm.depth == 32;

   env = getenv("IMLIB2_LBM_NOMASK");
   if (env
       && (!strcmp(env, "true") || !strcmp(env, "1") || !strcmp(env, "yes")
           || !strcmp(env, "on")))
      im->has_alpha = 0;

   if (!load_data)
      QUIT_WITH_RC(LOAD_SUCCESS);

   ilbm.ham = 0;
   ilbm.hbrite = 0;
   if (ilbm.depth <= 8)
     {
        if (ilbm.camg.size == 4)
          {
             if (ilbm.camg.data[2] & 0x08)
                ilbm.ham = 1;
             if (ilbm.camg.data[3] & 0x80)
                ilbm.hbrite = 1;
          }
        else
          {                     /* Only guess at ham and hbrite if CMAP is present. */
             if (ilbm.depth == 6 && ilbm.cmap.size >= 3 * 16)
                ilbm.ham = 1;
             if (!ilbm.ham && ilbm.depth > 1
                 && ilbm.cmap.size == 3 * (1 << (ilbm.depth - 1)))
                ilbm.hbrite = 1;
          }
     }

  /*----------
   * The source data is planar. Each plane is an even number of bytes wide. If
   * masking type is 1, there is an extra plane that defines the mask. Scanlines
   * from each plane are interleaved, from top to bottom. The first plane is the
   * 0 bit.
   *----------*/

   if (!__imlib_AllocateData(im))
      QUIT_WITH_RC(LOAD_OOM);

   n = ilbm.depth;
   if (ilbm.mask == 1)
      n++;
   plane[0] = malloc(((im->w + 15) / 16) * 2 * n);
   if (!plane[0])
      QUIT_WITH_RC(LOAD_OOM);

   for (i = 1; i < n; i++)
      plane[i] = plane[i - 1] + ((im->w + 15) / 16) * 2;

   z = ((im->w + 15) / 16) * 2 * n;

   scalecmap(&ilbm);

   for (y = 0; y < im->h; y++)
     {
        bodyrow(plane[0], z, &ilbm);

        deplane(im->data + im->w * y, im->w, &ilbm, plane);
        ilbm.row++;

        if (im->lc && __imlib_LoadProgressRows(im, y, 1))
           QUIT_WITH_RC(LOAD_BREAK);
     }

   rc = LOAD_SUCCESS;

 quit:
  /*----------
   * We either had a successful decode, the user cancelled, or we couldn't get
   * the memory for im->data or plane[0].
   *----------*/
   free(plane[0]);

   freeilbm(&ilbm);

   return rc;
}

IMLIB_LOADER(_formats, _load, NULL);
