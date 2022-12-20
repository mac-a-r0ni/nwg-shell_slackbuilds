#include "common.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "font.h"

FT_Library          ft_lib;

void
__imlib_font_init(void)
{
   static int          initialised = 0;
   int                 error;

   if (initialised)
      return;
   error = FT_Init_FreeType(&ft_lib);
   if (error)
      return;
   initialised = 1;
}

int
__imlib_font_ascent_get(ImlibFont * fn)
{
   int                 val;
   int                 ret;

   val = (int)fn->ft.face->ascender;
   fn->ft.face->units_per_EM = 2048;    /* nasy hack - need to have correct 
                                         * val */
   ret =
      (val * fn->ft.face->size->metrics.y_scale) /
      (fn->ft.face->units_per_EM * fn->ft.face->units_per_EM);
   return ret;
}

int
__imlib_font_descent_get(ImlibFont * fn)
{
   int                 val;
   int                 ret;

   val = -(int)fn->ft.face->descender;
   fn->ft.face->units_per_EM = 2048;    /* nasy hack - need to have correct 
                                         * val */
   ret =
      (val * fn->ft.face->size->metrics.y_scale) /
      (fn->ft.face->units_per_EM * fn->ft.face->units_per_EM);
   return ret;
}

int
__imlib_font_max_ascent_get(ImlibFont * fn)
{
   int                 val;
   int                 ret;

   val = (int)fn->ft.face->bbox.yMax;
   fn->ft.face->units_per_EM = 2048;    /* nasy hack - need to have correct 
                                         * val */
   ret =
      (val * fn->ft.face->size->metrics.y_scale) /
      (fn->ft.face->units_per_EM * fn->ft.face->units_per_EM);
   return ret;
}

int
__imlib_font_max_descent_get(ImlibFont * fn)
{
   int                 val;
   int                 ret;

   val = (int)fn->ft.face->bbox.yMin;
   fn->ft.face->units_per_EM = 2048;    /* nasy hack - need to have correct 
                                         * val */
   ret =
      (val * fn->ft.face->size->metrics.y_scale) /
      (fn->ft.face->units_per_EM * fn->ft.face->units_per_EM);
   return ret;
}

int
__imlib_font_get_line_advance(ImlibFont * fn)
{
   int                 val;
   int                 ret;

   val = (int)fn->ft.face->height;
   fn->ft.face->units_per_EM = 2048;    /* nasy hack - need to have correct 
                                         * val */
   ret =
      (val * fn->ft.face->size->metrics.y_scale) /
      (fn->ft.face->units_per_EM * fn->ft.face->units_per_EM);
   return ret;
}

/*
 * Reads UTF8 bytes from @text, starting at *@index and returns the code
 * point of the next valid code point. @index is updated ready for the
 * next call.
 *
 * Returns 0 to indicate an error (e.g. invalid UTF8)
 */
static int
_utf8_get_next(const char *text, int *iindex)
{
   const unsigned char *buf = (const unsigned char *)text;
   int                 index = *iindex, r;
   unsigned char       d = buf[index++], d2, d3, d4;

   if (!d)
      return 0;
   if (d < 0x80)
     {
        *iindex = index;
        return d;
     }
   if ((d & 0xe0) == 0xc0)
     {
        /* 2 byte */
        d2 = buf[index++];
        if ((d2 & 0xc0) != 0x80)
           return 0;
        r = d & 0x1f;           /* copy lower 5 */
        r <<= 6;
        r |= (d2 & 0x3f);       /* copy lower 6 */
     }
   else if ((d & 0xf0) == 0xe0)
     {
        /* 3 byte */
        d2 = buf[index++];
        d3 = buf[index++];
        if ((d2 & 0xc0) != 0x80 || (d3 & 0xc0) != 0x80)
           return 0;
        r = d & 0x0f;           /* copy lower 4 */
        r <<= 6;
        r |= (d2 & 0x3f);
        r <<= 6;
        r |= (d3 & 0x3f);
     }
   else
     {
        /* 4 byte */
        d2 = buf[index++];
        d3 = buf[index++];
        d4 = buf[index++];
        if ((d2 & 0xc0) != 0x80 || (d3 & 0xc0) != 0x80 || (d4 & 0xc0) != 0x80)
           return 0;
        r = d & 0x0f;           /* copy lower 4 */
        r <<= 6;
        r |= (d2 & 0x3f);
        r <<= 6;
        r |= (d3 & 0x3f);
        r <<= 6;
        r |= (d4 & 0x3f);

     }
   *iindex = index;
   return r;
}

/*
 * This function returns the first font in the fallback chain to contain
 * the requested glyph.
 * The glyph index is returned in ret_index
 * If the glyph is not found, then the given font pointer is returned and
 * ret_index will be set to 0
 */
static ImlibFont   *
__imlib_font_find_glyph(ImlibFont * first_fn, int gl, unsigned int *ret_index)
{
   int                 index;
   ImlibFont          *fn = first_fn;

   for (; fn; fn = fn->fallback_next)
     {
        index = FT_Get_Char_Index(fn->ft.face, gl);
        if (index <= 0)
           continue;
        *ret_index = index;
        return fn;
     }

   *ret_index = 0;
   return first_fn;
}

Imlib_Font_Glyph   *
__imlib_font_get_next_glyph(ImlibFont * fn, const char *utf8, int *cindx,
                            FT_UInt * pindex, int *pkern)
{
   FT_UInt             index;
   Imlib_Font_Glyph   *fg;
   ImlibFont          *fn_in_chain;
   int                 gl, kern;

   gl = _utf8_get_next(utf8, cindx);
   if (gl == 0)
      return NULL;

   fn_in_chain = __imlib_font_find_glyph(fn, gl, &index);

   kern = 0;
   if (FT_HAS_KERNING(fn->ft.face) && *pindex && index)
     {
        FT_Vector           delta;

        FT_Get_Kerning(fn_in_chain->ft.face, *pindex, index,
                       ft_kerning_default, &delta);
        kern = delta.x << 2;
     }
   if (pkern)
      *pkern = kern;

   fg = __imlib_font_cache_glyph_get(fn_in_chain, index);
   if (!fg)
      return IMLIB_GLYPH_NONE;

   *pindex = index;

   return fg;
}
