#include "common.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "font.h"

extern FT_Library   ft_lib;

/* string extents */
void
__imlib_font_query_size(ImlibFont * fn, const char *text, int *w, int *h)
{
   int                 pen_x /*, pen_y */ ;
   int                 start_x, end_x;
   int                 chr;

   start_x = 0;
   end_x = 0;
   pen_x = 0;
/* pen_y = 0; */
   for (chr = 0; text[chr];)
     {
        FT_UInt             index;
        Imlib_Font_Glyph   *fg;
        int                 chr_x, /*chr_y, */ chr_w, kern;

        fg = __imlib_font_get_next_glyph(fn, text, &chr, &index, &kern);
        if (!fg)
           break;
        pen_x += kern;
        if (fg == IMLIB_GLYPH_NONE)
           continue;

        chr_x = (pen_x >> 8) + fg->glyph_out->left;
/*      chr_y = (pen_y >> 8) + fg->glyph_out->top; */
        chr_w = fg->glyph_out->bitmap.width;

        if (pen_x == 0)
           start_x = chr_x;
        if ((chr_x + chr_w) > end_x)
           end_x = chr_x + chr_w;

        pen_x += fg->glyph->advance.x >> 8;
     }
   if (w)
      *w = (pen_x >> 8) - start_x;
   if (h)
      *h = __imlib_font_max_ascent_get(fn) - __imlib_font_max_descent_get(fn);  /* TODO: compute this inside the loop since we now may be dealing with multiple fonts */
}

/* text x inset */
int
__imlib_font_query_inset(ImlibFont * fn, const char *text)
{
   FT_UInt             index;
   Imlib_Font_Glyph   *fg;
   int                 chr;

   chr = 0;
   if (!text[0])
      return 0;

   fg = __imlib_font_get_next_glyph(fn, text, &chr, &index, NULL);
   if (!fg || fg == IMLIB_GLYPH_NONE)
      return 0;

   return -fg->glyph_out->left;
}

/* h & v advance */
void
__imlib_font_query_advance(ImlibFont * fn, const char *text, int *h_adv,
                           int *v_adv)
{
   int                 pen_x;
   int                 start_x;
   int                 chr;

   start_x = 0;
   pen_x = 0;
   for (chr = 0; text[chr];)
     {
        FT_UInt             index;
        Imlib_Font_Glyph   *fg;
        int                 kern;

        fg = __imlib_font_get_next_glyph(fn, text, &chr, &index, &kern);
        if (!fg)
           break;
        pen_x += kern;
        if (fg == IMLIB_GLYPH_NONE)
           continue;

        pen_x += fg->glyph->advance.x >> 8;
     }
   if (v_adv)
      *v_adv = __imlib_font_get_line_advance(fn);       /* TODO: compute this in the loop since we may be dealing with multiple fonts */
   if (h_adv)
      *h_adv = (pen_x >> 8) - start_x;
}

/* x y w h for char at char pos */
int
__imlib_font_query_char_coords(ImlibFont * fn, const char *text, int pos,
                               int *cx, int *cy, int *cw, int *ch)
{
   int                 pen_x;
   int                 prev_chr_end;
   int                 chr;
   int                 asc, desc;

   pen_x = 0;
   prev_chr_end = 0;
   asc = __imlib_font_max_ascent_get(fn);
   desc = __imlib_font_max_descent_get(fn);
   for (chr = 0; text[chr];)
     {
        int                 pchr;
        FT_UInt             index;
        Imlib_Font_Glyph   *fg;
        int                 chr_x, chr_w, kern;

        pchr = chr;
        fg = __imlib_font_get_next_glyph(fn, text, &chr, &index, &kern);
        if (!fg)
           break;
        pen_x += kern;
        if (fg == IMLIB_GLYPH_NONE)
           continue;

        if (kern < 0)
           kern = 0;
        chr_x = ((pen_x - kern) >> 8) + fg->glyph_out->left;
        chr_w = fg->glyph_out->bitmap.width + (kern >> 8);
        if (text[chr])
          {
             int                 advw;

             advw = ((fg->glyph->advance.x + (kern << 8)) >> 16);
             if (chr_w < advw)
                chr_w = advw;
          }
        if (chr_x > prev_chr_end)
          {
             chr_w += (chr_x - prev_chr_end);
             chr_x = prev_chr_end;
          }
        if (pchr == pos)
          {
             if (cx)
                *cx = chr_x;
             if (cy)
                *cy = -asc;
             if (cw)
                *cw = chr_w;
             if (ch)
                *ch = asc + desc;
             return 1;
          }
        prev_chr_end = chr_x + chr_w;
        pen_x += fg->glyph->advance.x >> 8;
     }
   return 0;
}

/* char pos of text at xy pos */
int
__imlib_font_query_text_at_pos(ImlibFont * fn, const char *text, int x, int y,
                               int *cx, int *cy, int *cw, int *ch)
{
   int                 pen_x;
   int                 prev_chr_end;
   int                 chr;
   int                 asc, desc;

   pen_x = 0;
   prev_chr_end = 0;
   asc = __imlib_font_max_ascent_get(fn);
   desc = __imlib_font_max_descent_get(fn);
   for (chr = 0; text[chr];)
     {
        int                 pchr;
        FT_UInt             index;
        Imlib_Font_Glyph   *fg;
        int                 chr_x, chr_w, kern;

        pchr = chr;
        fg = __imlib_font_get_next_glyph(fn, text, &chr, &index, &kern);
        if (!fg)
           break;
        pen_x += kern;
        if (fg == IMLIB_GLYPH_NONE)
           continue;

        if (kern < 0)
           kern = 0;
        chr_x = ((pen_x - kern) >> 8) + fg->glyph_out->left;
        chr_w = fg->glyph_out->bitmap.width + (kern >> 8);
        if (text[chr])
          {
             int                 advw;

             advw = ((fg->glyph->advance.x + (kern << 8)) >> 16);
             if (chr_w < advw)
                chr_w = advw;
          }
        if (chr_x > prev_chr_end)
          {
             chr_w += (chr_x - prev_chr_end);
             chr_x = prev_chr_end;
          }
        if ((x >= chr_x) && (x <= (chr_x + chr_w)) && (y > -asc) && (y < desc))
          {
             if (cx)
                *cx = chr_x;
             if (cy)
                *cy = -asc;
             if (cw)
                *cw = chr_w;
             if (ch)
                *ch = asc + desc;
             return pchr;
          }
        prev_chr_end = chr_x + chr_w;
        pen_x += fg->glyph->advance.x >> 8;
     }
   return -1;
}
