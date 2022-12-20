/*
 * XBM loader
 */
#include "config.h"
#include "Imlib2_Loader.h"

#define DBG_PFX "LDR-xbm"

static const char  *const _formats[] = { "xbm" };

static struct {
   const char         *data, *dptr;
   unsigned int        size;
} mdata;

static void
mm_init(const void *src, unsigned int size)
{
   mdata.data = mdata.dptr = src;
   mdata.size = size;
}

static const char  *
mm_gets(char *dst, unsigned int len)
{
   int                 left = mdata.data + mdata.size - mdata.dptr;
   int                 cnt;
   const char         *ptr;

   if (left <= 0)
      return NULL;              /* Out of data */

   ptr = memchr(mdata.dptr, '\n', left);

   cnt = (ptr) ? ptr - mdata.dptr : left;
   if (cnt >= (int)len)
      cnt = len - 1;

   memcpy(dst, mdata.dptr, cnt);
   dst[cnt] = '\0';

   if (ptr)
      cnt += 1;
   mdata.dptr += cnt;

   return dst;
}

static const uint32_t _bitmap_colors[2] = { 0xffffffff, 0xff000000 };

static              uint32_t
_bitmap_color(int bit)
{
   return _bitmap_colors[!!bit];
}

static int
_bitmap_dither(int x, int y, uint32_t pixel)
{
   static const uint8_t _dither_44[4][4] = {
   /**INDENT-OFF**/
      { 0, 32,  8, 40},
      {48, 16, 56, 24},
      {12, 44,  4, 36},
      {60, 28, 52, 20},
   /**INDENT-ON**/
   };
   int                 val, set;

   if (PIXEL_A(pixel) < 0x80)
     {
        set = 0;
     }
   else
     {
        val = (PIXEL_R(pixel) + PIXEL_G(pixel) + PIXEL_B(pixel)) / 12;
        set = val <= _dither_44[x & 0x3][y & 0x3];
     }

   return set;
}

static int
_load(ImlibImage * im, int load_data)
{
   int                 rc;
   char                buf[4096], tok1[1024], tok2[1024];
   uint32_t           *ptr, pixel;
   int                 i, x, y, bit, nl;
   const char         *s;
   int                 header, val, nlen;

   rc = LOAD_FAIL;

   if (im->fi->fsize < 64)
      return rc;                /* Not XBM */

   /* Signature check ("#define") allow longish initial comment */
   s = im->fi->fdata;
   nlen = s[0] == '/' && s[1] == '*' ? 4096 : 256;
   nlen = im->fi->fsize > nlen ? nlen : im->fi->fsize;
   if (!memmem(s, nlen, "#define", 7))
      goto quit;

   mm_init(im->fi->fdata, im->fi->fsize);

   ptr = NULL;
   x = y = 0;

   header = 1;
   for (nl = 0;; nl++)
     {
        s = mm_gets(buf, sizeof(buf));
        if (!s)
           break;

        D(">>>%s\n", buf);

        if (header)
          {
             /* Header */
             tok1[0] = tok2[0] = '\0';
             val = -1;
             sscanf(buf, " %1023s %1023s %d", tok1, tok2, &val);
             D("'%s': '%s': %d\n", tok1, tok2, val);
             if (strcmp(tok1, "#define") == 0)
               {
                  if (tok2[0] == '\0')
                     goto quit;

                  nlen = strlen(tok2);
                  if (nlen > 6 && strcmp(tok2 + nlen - 6, "_width") == 0)
                    {
                       D("'%s' = %d\n", tok2, val);
                       im->w = val;
                    }
                  else if (nlen > 7 && strcmp(tok2 + nlen - 7, "_height") == 0)
                    {
                       D("'%s' = %d\n", tok2, val);
                       im->h = val;
                    }
               }
             else if (strcmp(tok1, "static") == 0 && strstr(buf + 6, "_bits"))
               {
                  if (!IMAGE_DIMENSIONS_OK(im->w, im->h))
                     goto quit;

                  if (!load_data)
                     QUIT_WITH_RC(LOAD_SUCCESS);

                  header = 0;

                  rc = LOAD_BADIMAGE;   /* Format accepted */

                  ptr = __imlib_AllocateData(im);
                  if (!ptr)
                     QUIT_WITH_RC(LOAD_OOM);
               }
             else
               {
                  /* Quit if we don't have the header in N lines */
                  if (nl >= 30)
                     break;
                  continue;
               }
          }
        else
          {
             /* Data */
             for (; *s != '\0';)
               {
                  nlen = -1;
                  sscanf(s, "%i%n", &val, &nlen);
                  D("Data '%s': %02x (%d)\n", s, val, nlen);
                  if (nlen < 0)
                     break;
                  s += nlen;
                  if (*s == ',')
                     s++;

                  for (i = 0; i < 8 && x < im->w; i++, x++)
                    {
                       bit = (val & (1 << i)) != 0;
                       pixel = _bitmap_color(bit);
                       *ptr++ = pixel;
                       D("i, x, y: %2d %2d %2d: %08x\n", i, x, y, pixel);
                    }

                  if (x >= im->w)
                    {
                       if (im->lc && __imlib_LoadProgressRows(im, y, 1))
                          QUIT_WITH_RC(LOAD_BREAK);

                       x = 0;
                       y += 1;
                       if (y >= im->h)
                          goto done;
                    }
               }
          }
     }

 done:
   if (!header)
      rc = LOAD_SUCCESS;

 quit:
   return rc;
}

static int
_save(ImlibImage * im)
{
   FILE               *f;
   int                 rc;
   const char         *s, *name;
   char               *bname;
   int                 i, k, x, y, bits, nval, val;
   uint32_t           *ptr;

   f = fopen(im->fi->name, "wb");
   if (!f)
      return LOAD_FAIL;

   rc = LOAD_SUCCESS;

   name = im->fi->name;
   if ((s = strrchr(name, '/')) != 0)
      name = s + 1;

   bname = strndup(name, strcspn(name, "."));

   fprintf(f, "#define %s_width %d\n", bname, im->w);
   fprintf(f, "#define %s_height %d\n", bname, im->h);
   fprintf(f, "static unsigned char %s_bits[] = {\n", bname);

   free(bname);

   nval = ((im->w + 7) / 8) * im->h;
   ptr = im->data;
   x = k = 0;
   for (y = 0; y < im->h;)
     {
        bits = 0;
        for (i = 0; i < 8 && x < im->w; i++, x++)
          {
             val = _bitmap_dither(x, y, *ptr++);
             if (val)
                bits |= 1 << i;
          }
        if (x >= im->w)
          {
             x = 0;
             y += 1;
          }
        k++;
        D("x, y = %2d,%2d: %d/%d\n", x, y, k, nval);
        fprintf(f, " 0x%02x%s%s", bits, k < nval ? "," : "",
                (k == nval) || ((k % 12) == 0) ? "\n" : "");
     }

   fprintf(f, "};\n");

   fclose(f);

   return rc;
}

IMLIB_LOADER(_formats, _load, _save);
