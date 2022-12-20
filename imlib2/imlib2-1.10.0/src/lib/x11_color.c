#include "common.h"

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "x11_color.h"

unsigned short      _max_colors = 256;

int
__imlib_XActualDepth(Display * d, Visual * v)
{
   XVisualInfo         xvi, *xvir;
   int                 depth = 0, num;

   xvi.visual = v;
   xvi.visualid = XVisualIDFromVisual(v);
   xvir = XGetVisualInfo(d, VisualIDMask, &xvi, &num);
   if (xvir)
     {
        depth = xvir[0].depth;
        if ((depth == 16) &&
            ((xvir->red_mask | xvir->green_mask | xvir->blue_mask) == 0x7fff))
           depth = 15;
        XFree(xvir);
     }
   return depth;
}

Visual             *
__imlib_BestVisual(Display * d, int screen, int *depth_return)
{
   XVisualInfo         xvi, *xvir;
   int                 j, i, num, maxd = 0;
   Visual             *v = NULL;

   const int           visprefs[] = {
      PseudoColor, TrueColor, DirectColor, StaticColor, GrayScale, StaticGray
   };

   xvi.screen = screen;
   maxd = 0;
   for (j = 0; j < 6; j++)
     {
        xvi.class = visprefs[j];
        xvir = XGetVisualInfo(d, VisualScreenMask | VisualClassMask,
                              &xvi, &num);
        if (xvir)
          {
             for (i = 0; i < num; i++)
               {
                  if ((xvir[i].depth > 1) &&
                      (xvir[i].depth >= maxd) && (xvi.class == PseudoColor))
                    {
                       maxd = xvir[i].depth;
                       v = xvir[i].visual;
                    }
                  else if ((xvir[i].depth > maxd) && (xvir[i].depth <= 24))
                    {
                       maxd = xvir[i].depth;
                       v = xvir[i].visual;
                    }
               }
             XFree(xvir);
          }
     }
   if (depth_return)
      *depth_return = maxd;
   return v;
}

static void
_free_colors(Display * d, Colormap cmap, uint8_t * lut, int num)
{
   unsigned long       pixels[256];
   int                 i;

   if (num > 0)
     {
        for (i = 0; i < num; i++)
           pixels[i] = (unsigned long)lut[i];
        XFreeColors(d, cmap, pixels, num, 0);
     }

   free(lut);
}

static uint8_t     *
__imlib_AllocColors332(Display * d, Colormap cmap, Visual * v)
{
   int                 r, g, b, i;
   uint8_t            *color_lut;
   int                 sig_mask = 0;

   for (i = 0; i < v->bits_per_rgb; i++)
      sig_mask |= (0x1 << i);
   sig_mask <<= (16 - v->bits_per_rgb);
   i = 0;
   color_lut = malloc(256 * sizeof(uint8_t));
   if (!color_lut)
      return NULL;
   for (r = 0; r < 8; r++)
     {
        for (g = 0; g < 8; g++)
          {
             for (b = 0; b < 4; b++)
               {
                  XColor              xcl;
                  XColor              xcl_in;
                  int                 val;
                  Status              ret;

                  val = (r << 6) | (r << 3) | (r);
                  xcl.red = (unsigned short)((val << 7) | (val >> 2));
                  val = (g << 6) | (g << 3) | (g);
                  xcl.green = (unsigned short)((val << 7) | (val >> 2));
                  val = (b << 6) | (b << 4) | (b << 2) | (b);
                  xcl.blue = (unsigned short)((val << 8) | (val));
                  xcl_in = xcl;
                  ret = XAllocColor(d, cmap, &xcl);
                  if ((ret == 0) ||
                      ((xcl_in.red & sig_mask) != (xcl.red & sig_mask)) ||
                      ((xcl_in.green & sig_mask) != (xcl.green & sig_mask)) ||
                      ((xcl_in.blue & sig_mask) != (xcl.blue & sig_mask)))
                    {
                       _free_colors(d, cmap, color_lut, i);
                       return NULL;
                    }
                  color_lut[i] = xcl.pixel;
                  i++;
               }
          }
     }
   return color_lut;
}

static uint8_t     *
__imlib_AllocColors666(Display * d, Colormap cmap, Visual * v)
{
   int                 r, g, b, i;
   uint8_t            *color_lut;
   int                 sig_mask = 0;

   for (i = 0; i < v->bits_per_rgb; i++)
      sig_mask |= (0x1 << i);
   sig_mask <<= (16 - v->bits_per_rgb);
   i = 0;
   color_lut = malloc(256 * sizeof(uint8_t));
   if (!color_lut)
      return NULL;
   for (r = 0; r < 6; r++)
     {
        for (g = 0; g < 6; g++)
          {
             for (b = 0; b < 6; b++)
               {
                  XColor              xcl;
                  XColor              xcl_in;
                  int                 val;
                  Status              ret;

                  val = (int)((((double)r) / 5.0) * 65535);
                  xcl.red = (unsigned short)(val);
                  val = (int)((((double)g) / 5.0) * 65535);
                  xcl.green = (unsigned short)(val);
                  val = (int)((((double)b) / 5.0) * 65535);
                  xcl.blue = (unsigned short)(val);
                  xcl_in = xcl;
                  ret = XAllocColor(d, cmap, &xcl);
                  if ((ret == 0) ||
                      ((xcl_in.red & sig_mask) != (xcl.red & sig_mask)) ||
                      ((xcl_in.green & sig_mask) != (xcl.green & sig_mask)) ||
                      ((xcl_in.blue & sig_mask) != (xcl.blue & sig_mask)))
                    {
                       _free_colors(d, cmap, color_lut, i);
                       return NULL;
                    }
                  color_lut[i] = xcl.pixel;
                  i++;
               }
          }
     }
   return color_lut;
}

static uint8_t     *
__imlib_AllocColors232(Display * d, Colormap cmap, Visual * v)
{
   int                 r, g, b, i;
   uint8_t            *color_lut;
   int                 sig_mask = 0;

   for (i = 0; i < v->bits_per_rgb; i++)
      sig_mask |= (0x1 << i);
   sig_mask <<= (16 - v->bits_per_rgb);
   i = 0;
   color_lut = malloc(128 * sizeof(uint8_t));
   if (!color_lut)
      return NULL;
   for (r = 0; r < 4; r++)
     {
        for (g = 0; g < 8; g++)
          {
             for (b = 0; b < 4; b++)
               {
                  XColor              xcl;
                  XColor              xcl_in;
                  int                 val;
                  Status              ret;

                  val = (r << 6) | (r << 4) | (r << 2) | (r);
                  xcl.red = (unsigned short)((val << 8) | (val));
                  val = (g << 6) | (g << 3) | (g);
                  xcl.green = (unsigned short)((val << 7) | (val >> 2));
                  val = (b << 6) | (b << 4) | (b << 2) | (b);
                  xcl.blue = (unsigned short)((val << 8) | (val));
                  xcl_in = xcl;
                  ret = XAllocColor(d, cmap, &xcl);
                  if ((ret == 0) ||
                      ((xcl_in.red & sig_mask) != (xcl.red & sig_mask)) ||
                      ((xcl_in.green & sig_mask) != (xcl.green & sig_mask)) ||
                      ((xcl_in.blue & sig_mask) != (xcl.blue & sig_mask)))
                    {
                       _free_colors(d, cmap, color_lut, i);
                       return NULL;
                    }
                  color_lut[i] = xcl.pixel;
                  i++;
               }
          }
     }
   return color_lut;
}

static uint8_t     *
__imlib_AllocColors222(Display * d, Colormap cmap, Visual * v)
{
   int                 r, g, b, i;
   uint8_t            *color_lut;
   int                 sig_mask = 0;

   for (i = 0; i < v->bits_per_rgb; i++)
      sig_mask |= (0x1 << i);
   sig_mask <<= (16 - v->bits_per_rgb);
   i = 0;
   color_lut = malloc(64 * sizeof(uint8_t));
   if (!color_lut)
      return NULL;
   for (r = 0; r < 4; r++)
     {
        for (g = 0; g < 4; g++)
          {
             for (b = 0; b < 4; b++)
               {
                  XColor              xcl;
                  XColor              xcl_in;
                  int                 val;
                  Status              ret;

                  val = (r << 6) | (r << 4) | (r << 2) | (r);
                  xcl.red = (unsigned short)((val << 8) | (val));
                  val = (g << 6) | (g << 4) | (g << 2) | (g);
                  xcl.green = (unsigned short)((val << 8) | (val));
                  val = (b << 6) | (b << 4) | (b << 2) | (b);
                  xcl.blue = (unsigned short)((val << 8) | (val));
                  xcl_in = xcl;
                  ret = XAllocColor(d, cmap, &xcl);
                  if ((ret == 0) ||
                      ((xcl_in.red & sig_mask) != (xcl.red & sig_mask)) ||
                      ((xcl_in.green & sig_mask) != (xcl.green & sig_mask)) ||
                      ((xcl_in.blue & sig_mask) != (xcl.blue & sig_mask)))
                    {
                       _free_colors(d, cmap, color_lut, i);
                       return NULL;
                    }
                  color_lut[i] = xcl.pixel;
                  i++;
               }
          }
     }
   return color_lut;
}

static uint8_t     *
__imlib_AllocColors221(Display * d, Colormap cmap, Visual * v)
{
   int                 r, g, b, i;
   uint8_t            *color_lut;
   int                 sig_mask = 0;

   for (i = 0; i < v->bits_per_rgb; i++)
      sig_mask |= (0x1 << i);
   sig_mask <<= (16 - v->bits_per_rgb);
   i = 0;
   color_lut = malloc(32 * sizeof(uint8_t));
   if (!color_lut)
      return NULL;
   for (r = 0; r < 4; r++)
     {
        for (g = 0; g < 4; g++)
          {
             for (b = 0; b < 2; b++)
               {
                  XColor              xcl;
                  XColor              xcl_in;
                  int                 val;
                  Status              ret;

                  val = (r << 6) | (r << 4) | (r << 2) | (r);
                  xcl.red = (unsigned short)((val << 8) | (val));
                  val = (g << 6) | (g << 4) | (g << 2) | (g);
                  xcl.green = (unsigned short)((val << 8) | (val));
                  val = (b << 7) | (b << 6) | (b << 5) | (b << 4) |
                     (b << 3) | (b << 2) | (b << 1) | (b);
                  xcl.blue = (unsigned short)((val << 8) | (val));
                  xcl_in = xcl;
                  ret = XAllocColor(d, cmap, &xcl);
                  if ((ret == 0) ||
                      ((xcl_in.red & sig_mask) != (xcl.red & sig_mask)) ||
                      ((xcl_in.green & sig_mask) != (xcl.green & sig_mask)) ||
                      ((xcl_in.blue & sig_mask) != (xcl.blue & sig_mask)))
                    {
                       _free_colors(d, cmap, color_lut, i);
                       return NULL;
                    }
                  color_lut[i] = xcl.pixel;
                  i++;
               }
          }
     }
   return color_lut;
}

static uint8_t     *
__imlib_AllocColors121(Display * d, Colormap cmap, Visual * v)
{
   int                 r, g, b, i;
   uint8_t            *color_lut;
   int                 sig_mask = 0;

   for (i = 0; i < v->bits_per_rgb; i++)
      sig_mask |= (0x1 << i);
   sig_mask <<= (16 - v->bits_per_rgb);
   i = 0;
   color_lut = malloc(16 * sizeof(uint8_t));
   if (!color_lut)
      return NULL;
   for (r = 0; r < 2; r++)
     {
        for (g = 0; g < 4; g++)
          {
             for (b = 0; b < 2; b++)
               {
                  XColor              xcl;
                  XColor              xcl_in;
                  int                 val;
                  Status              ret;

                  val = (r << 7) | (r << 6) | (r << 5) | (r << 4) |
                     (r << 3) | (r << 2) | (r << 1) | (r);
                  xcl.red = (unsigned short)((val << 8) | (val));
                  val = (g << 6) | (g << 4) | (g << 2) | (g);
                  xcl.green = (unsigned short)((val << 8) | (val));
                  val = (b << 7) | (b << 6) | (b << 5) | (b << 4) |
                     (b << 3) | (b << 2) | (b << 1) | (b);
                  xcl.blue = (unsigned short)((val << 8) | (val));
                  xcl_in = xcl;
                  ret = XAllocColor(d, cmap, &xcl);
                  if ((ret == 0) ||
                      ((xcl_in.red & sig_mask) != (xcl.red & sig_mask)) ||
                      ((xcl_in.green & sig_mask) != (xcl.green & sig_mask)) ||
                      ((xcl_in.blue & sig_mask) != (xcl.blue & sig_mask)))
                    {
                       _free_colors(d, cmap, color_lut, i);
                       return NULL;
                    }
                  color_lut[i] = xcl.pixel;
                  i++;
               }
          }
     }
   return color_lut;
}

static uint8_t     *
__imlib_AllocColors111(Display * d, Colormap cmap, Visual * v)
{
   int                 r, g, b, i;
   uint8_t            *color_lut;
   int                 sig_mask = 0;

   for (i = 0; i < v->bits_per_rgb; i++)
      sig_mask |= (0x1 << i);
   sig_mask <<= (16 - v->bits_per_rgb);
   i = 0;
   color_lut = malloc(8 * sizeof(uint8_t));
   if (!color_lut)
      return NULL;
   for (r = 0; r < 2; r++)
     {
        for (g = 0; g < 2; g++)
          {
             for (b = 0; b < 2; b++)
               {
                  XColor              xcl;
                  XColor              xcl_in;
                  int                 val;
                  Status              ret;

                  val = (r << 7) | (r << 6) | (r << 5) | (r << 4) |
                     (r << 3) | (r << 2) | (r << 1) | (r);
                  xcl.red = (unsigned short)((val << 8) | (val));
                  val = (g << 7) | (g << 6) | (g << 5) | (g << 4) |
                     (g << 3) | (g << 2) | (g << 1) | (g);
                  xcl.green = (unsigned short)((val << 8) | (val));
                  val = (b << 7) | (b << 6) | (b << 5) | (b << 4) |
                     (b << 3) | (b << 2) | (b << 1) | (b);
                  xcl.blue = (unsigned short)((val << 8) | (val));
                  xcl_in = xcl;
                  ret = XAllocColor(d, cmap, &xcl);
                  if ((ret == 0) ||
                      ((xcl_in.red & sig_mask) != (xcl.red & sig_mask)) ||
                      ((xcl_in.green & sig_mask) != (xcl.green & sig_mask)) ||
                      ((xcl_in.blue & sig_mask) != (xcl.blue & sig_mask)))
                    {
                       _free_colors(d, cmap, color_lut, i);
                       return NULL;
                    }
                  color_lut[i] = xcl.pixel;
                  i++;
               }
          }
     }
   return color_lut;
}

static uint8_t     *
__imlib_AllocColors1(Display * d, Colormap cmap, Visual * v)
{
   XColor              xcl;
   uint8_t            *color_lut;
   int                 i;

   color_lut = malloc(2 * sizeof(uint8_t));
   if (!color_lut)
      return NULL;

   i = 0;
   xcl.red = (unsigned short)(0x0000);
   xcl.green = (unsigned short)(0x0000);
   xcl.blue = (unsigned short)(0x0000);
   if (!XAllocColor(d, cmap, &xcl))
      goto bail;
   color_lut[i++] = xcl.pixel;

   xcl.red = (unsigned short)(0xffff);
   xcl.green = (unsigned short)(0xffff);
   xcl.blue = (unsigned short)(0xffff);
   if (!XAllocColor(d, cmap, &xcl))
      goto bail;
   color_lut[i] = xcl.pixel;

   return color_lut;

 bail:
   _free_colors(d, cmap, color_lut, i);
   return NULL;
}

uint8_t            *
__imlib_AllocColorTable(Display * d, Colormap cmap,
                        unsigned char *type_return, Visual * v)
{
   uint8_t            *color_lut = NULL;

   if (v->bits_per_rgb > 1)
     {
        if ((_max_colors >= 256)
            && (color_lut = __imlib_AllocColors332(d, cmap, v)))
          {
             *type_return = PAL_TYPE_332;
             return color_lut;
          }
        if ((_max_colors >= 216)
            && (color_lut = __imlib_AllocColors666(d, cmap, v)))
          {
             *type_return = PAL_TYPE_666;
             return color_lut;
          }
        if ((_max_colors >= 128)
            && (color_lut = __imlib_AllocColors232(d, cmap, v)))
          {
             *type_return = PAL_TYPE_232;
             return color_lut;
          }
        if ((_max_colors >= 64)
            && (color_lut = __imlib_AllocColors222(d, cmap, v)))
          {
             *type_return = PAL_TYPE_222;
             return color_lut;
          }
        if ((_max_colors >= 32)
            && (color_lut = __imlib_AllocColors221(d, cmap, v)))
          {
             *type_return = PAL_TYPE_221;
             return color_lut;
          }
        if ((_max_colors >= 16)
            && (color_lut = __imlib_AllocColors121(d, cmap, v)))
          {
             *type_return = PAL_TYPE_121;
             return color_lut;
          }
     }

   if ((_max_colors >= 8) && (color_lut = __imlib_AllocColors111(d, cmap, v)))
     {
        *type_return = PAL_TYPE_111;
        return color_lut;
     }

   color_lut = __imlib_AllocColors1(d, cmap, v);
   *type_return = PAL_TYPE_1;
   return color_lut;
}
