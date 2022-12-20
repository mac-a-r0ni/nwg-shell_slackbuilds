#include "common.h"

#include <stdlib.h>

#include "asm_c.h"
#include "blend.h"
#include "colormod.h"
#include "image.h"
#include "scale.h"

#define ADD_COPY(r, g, b, dest) \
                ADD_COLOR(R_VAL(dest), r, R_VAL(dest)); \
                ADD_COLOR(G_VAL(dest), g, G_VAL(dest)); \
                ADD_COLOR(B_VAL(dest), b, B_VAL(dest));

#define SUB_COPY(r, g, b, dest) \
                SUB_COLOR(R_VAL(dest), r, R_VAL(dest)); \
                SUB_COLOR(G_VAL(dest), g, G_VAL(dest)); \
                SUB_COLOR(B_VAL(dest), b, B_VAL(dest));

#define RE_COPY(r, g, b, dest) \
                RESHADE_COLOR(R_VAL(dest), r, R_VAL(dest)); \
                RESHADE_COLOR(G_VAL(dest), g, G_VAL(dest)); \
                RESHADE_COLOR(B_VAL(dest), b, B_VAL(dest));

uint8_t             pow_lut[256][256];

void
__imlib_build_pow_lut(void)
{
   static char         _pow_lut_initialized = 0;
   int                 i, j;

   if (_pow_lut_initialized)
      return;
   _pow_lut_initialized = 1;

   for (i = 0; i < 256; i++)
     {
        for (j = 0; j < 256; j++)
/*	   pow_lut[i][j] = 255 * pow((double)i / 255, (double)j / 255);  */
          {
             int                 divisor;

             divisor = (i + (j * (255 - i)) / 255);
             if (divisor > 0)
                pow_lut[i][j] = (i * 255) / divisor;
             else
                pow_lut[i][j] = 0;
          }
     }
}

/* COPY OPS */

static void
__imlib_BlendRGBAToRGB(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                       int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;
             uint8_t             a;

             a = A_VAL(src);
             switch (a)
               {
               case 0:
                  break;
               case 255:
                  *dst = (*dst & 0xff000000) | (*src & 0x00ffffff);
                  break;
               default:
                  BLEND(R_VAL(src), G_VAL(src), B_VAL(src), a, dst);
                  break;
               }
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_BlendRGBAToRGBA(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                        int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;
             uint8_t             a, aa;

             aa = A_VAL(src);
             switch (aa)
               {
               case 0:
                  break;
               case 255:
                  *dst = *src;
                  break;
               default:
                  a = pow_lut[aa][A_VAL(dst)];
                  BLEND_COLOR(aa, A_VAL(dst), 255, A_VAL(dst));
                  BLEND(R_VAL(src), G_VAL(src), B_VAL(src), a, dst);
                  break;
               }
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_CopyRGBAToRGB(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                      int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;

   while (h--)
     {
        while (w--)
          {
             *dst = (*dst & 0xff000000) | (*src & 0x00ffffff);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_CopyRGBToRGBA(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                      int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;

   while (h--)
     {
        while (w--)
          {
             *dst = 0xff000000 | (*src & 0x00ffffff);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_CopyRGBAToRGBA(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                       int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;

   while (h--)
     {
        while (w--)
          {
             *dst = *src;
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

/* ADD OPS */

static void
__imlib_AddBlendRGBAToRGB(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                          int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;
             uint8_t             a;

             a = A_VAL(src);
             switch (a)
               {
               case 0:
                  break;
               case 255:
                  ADD_COPY(R_VAL(src), G_VAL(src), B_VAL(src), dst);
                  break;
               default:
                  BLEND_ADD(R_VAL(src), G_VAL(src), B_VAL(src), a, dst);
                  break;
               }
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_AddBlendRGBAToRGBA(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                           int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;
             uint8_t             a, aa;

             aa = A_VAL(src);
             switch (aa)
               {
               case 0:
                  break;
               case 255:
                  A_VAL(dst) = 0xff;
                  ADD_COPY(R_VAL(src), G_VAL(src), B_VAL(src), dst);
                  break;
               default:
                  a = pow_lut[aa][A_VAL(dst)];
                  BLEND_COLOR(aa, A_VAL(dst), 255, A_VAL(dst));
                  BLEND_ADD(R_VAL(src), G_VAL(src), B_VAL(src), a, dst);
                  break;
               }
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_AddCopyRGBAToRGB(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                         int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             ADD_COPY(R_VAL(src), G_VAL(src), B_VAL(src), dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_AddCopyRGBAToRGBA(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                          int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             A_VAL(dst) = A_VAL(src);
             ADD_COPY(R_VAL(src), G_VAL(src), B_VAL(src), dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_AddCopyRGBToRGBA(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                         int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             A_VAL(dst) = 0xff;
             ADD_COPY(R_VAL(src), G_VAL(src), B_VAL(src), dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

/* SUBTRACT OPS */

static void
__imlib_SubBlendRGBAToRGB(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                          int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;
             uint8_t             a;

             a = A_VAL(src);
             switch (a)
               {
               case 0:
                  break;
               case 255:
                  SUB_COPY(R_VAL(src), G_VAL(src), B_VAL(src), dst);
                  break;
               default:
                  BLEND_SUB(R_VAL(src), G_VAL(src), B_VAL(src), a, dst);
                  break;
               }
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_SubBlendRGBAToRGBA(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                           int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;
             uint8_t             a, aa;

             aa = A_VAL(src);
             switch (aa)
               {
               case 0:
                  break;
               case 255:
                  A_VAL(dst) = 0xff;
                  SUB_COPY(R_VAL(src), G_VAL(src), B_VAL(src), dst);
                  break;
               default:
                  a = pow_lut[aa][A_VAL(dst)];
                  BLEND_COLOR(aa, A_VAL(dst), 255, A_VAL(dst));
                  BLEND_SUB(R_VAL(src), G_VAL(src), B_VAL(src), a, dst);
                  break;
               }
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_SubCopyRGBAToRGB(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                         int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             SUB_COPY(R_VAL(src), G_VAL(src), B_VAL(src), dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_SubCopyRGBAToRGBA(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                          int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             A_VAL(dst) = A_VAL(src);
             SUB_COPY(R_VAL(src), G_VAL(src), B_VAL(src), dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_SubCopyRGBToRGBA(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                         int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             A_VAL(dst) = 0xff;
             SUB_COPY(R_VAL(src), G_VAL(src), B_VAL(src), dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

/* RESHADE OPS */

static void
__imlib_ReBlendRGBAToRGB(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                         int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;
             uint8_t             a;

             a = A_VAL(src);
             switch (a)
               {
               case 0:
                  break;
               case 255:
                  RE_COPY(R_VAL(src), G_VAL(src), B_VAL(src), dst);
                  break;
               default:
                  BLEND_RE(R_VAL(src), G_VAL(src), B_VAL(src), a, dst);
                  break;
               }
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_ReBlendRGBAToRGBA(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                          int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;
             uint8_t             a, aa;

             aa = A_VAL(src);
             switch (aa)
               {
               case 0:
                  break;
               case 255:
                  A_VAL(dst) = 0xff;
                  RE_COPY(R_VAL(src), G_VAL(src), B_VAL(src), dst);
                  break;
               default:
                  a = pow_lut[aa][A_VAL(dst)];
                  BLEND_COLOR(aa, A_VAL(dst), 255, A_VAL(dst));
                  BLEND_RE(R_VAL(src), G_VAL(src), B_VAL(src), a, dst);
                  break;
               }
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_ReCopyRGBAToRGB(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                        int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             RE_COPY(R_VAL(src), G_VAL(src), B_VAL(src), dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_ReCopyRGBAToRGBA(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                         int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             A_VAL(dst) = A_VAL(src);
             RE_COPY(R_VAL(src), G_VAL(src), B_VAL(src), dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_ReCopyRGBToRGBA(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                        int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             A_VAL(dst) = 0xff;
             RE_COPY(R_VAL(src), G_VAL(src), B_VAL(src), dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

/* WITH COLOMOD */
/* COPY OPS */

static void
__imlib_BlendRGBAToRGBCmod(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                           int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;
             uint8_t             a;

             a = amod[A_VAL(src)];
             switch (a)
               {
               case 0:
                  break;
               case 255:
                  R_VAL(dst) = rmod[R_VAL(src)];
                  G_VAL(dst) = gmod[G_VAL(src)];
                  B_VAL(dst) = bmod[B_VAL(src)];
                  break;
               default:
                  BLEND(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)], a,
                        dst);
                  break;
               }
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_BlendRGBAToRGBACmod(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                            int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;
             uint8_t             a, aa;

             aa = amod[A_VAL(src)];
             switch (aa)
               {
               case 0:
                  break;
               case 255:
                  A_VAL(dst) = 0xff;
                  R_VAL(dst) = rmod[R_VAL(src)];
                  G_VAL(dst) = gmod[G_VAL(src)];
                  B_VAL(dst) = bmod[B_VAL(src)];
                  break;
               default:
                  a = pow_lut[aa][A_VAL(dst)];
                  BLEND_COLOR(aa, A_VAL(dst), 255, A_VAL(dst));
                  BLEND(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)], a,
                        dst);
                  break;
               }
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_BlendRGBToRGBACmod(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                           int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;
   uint8_t             am = amod[255];

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;
             uint8_t             a;

             a = pow_lut[am][A_VAL(dst)];
             BLEND_COLOR(am, A_VAL(dst), 255, A_VAL(dst))
                BLEND(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)], a,
                      dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_BlendRGBToRGBCmod(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                          int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;
   uint8_t             am = amod[255];

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             BLEND(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)], am,
                   dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_CopyRGBAToRGBCmod(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                          int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;

   while (h--)
     {
        while (w--)
          {
             R_VAL(dst) = rmod[R_VAL(src)];
             G_VAL(dst) = gmod[G_VAL(src)];
             B_VAL(dst) = bmod[B_VAL(src)];
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_CopyRGBToRGBACmod(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                          int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;
   uint8_t             am = amod[255];

   while (h--)
     {
        while (w--)
          {
             A_VAL(dst) = am;
             R_VAL(dst) = rmod[R_VAL(src)];
             G_VAL(dst) = gmod[G_VAL(src)];
             B_VAL(dst) = bmod[B_VAL(src)];
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_CopyRGBAToRGBACmod(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                           int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;

   while (h--)
     {
        while (w--)
          {
             A_VAL(dst) = amod[A_VAL(src)];
             R_VAL(dst) = rmod[R_VAL(src)];
             G_VAL(dst) = gmod[G_VAL(src)];
             B_VAL(dst) = bmod[B_VAL(src)];
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

/* ADD OPS */

static void
__imlib_AddBlendRGBAToRGBCmod(uint32_t * src, int srcw, uint32_t * dst,
                              int dstw, int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;
             uint8_t             a;

             a = amod[A_VAL(src)];
             switch (a)
               {
               case 0:
                  break;
               case 255:
                  ADD_COPY(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)],
                           dst);
                  break;
               default:
                  BLEND_ADD(rmod[R_VAL(src)], gmod[G_VAL(src)],
                            bmod[B_VAL(src)], a, dst);
                  break;
               }
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_AddBlendRGBAToRGBACmod(uint32_t * src, int srcw, uint32_t * dst,
                               int dstw, int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;
             uint8_t             a, aa;

             aa = amod[A_VAL(src)];
             switch (aa)
               {
               case 0:
                  break;
               case 255:
                  A_VAL(dst) = 0xff;
                  ADD_COPY(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)],
                           dst);
                  break;
               default:
                  a = pow_lut[aa][A_VAL(dst)];
                  BLEND_COLOR(aa, A_VAL(dst), 255, A_VAL(dst));
                  BLEND_ADD(rmod[R_VAL(src)], gmod[G_VAL(src)],
                            bmod[B_VAL(src)], a, dst);
                  break;
               }
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_AddBlendRGBToRGBCmod(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                             int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;
   uint8_t             am = amod[255];

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             BLEND_ADD(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)], am,
                       dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_AddBlendRGBToRGBACmod(uint32_t * src, int srcw, uint32_t * dst,
                              int dstw, int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;
   uint8_t             am = amod[255];

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;
             uint8_t             a;

             a = pow_lut[am][A_VAL(dst)];
             BLEND_COLOR(am, A_VAL(dst), 255, A_VAL(dst));
             BLEND_ADD(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)], a,
                       dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_AddCopyRGBAToRGBCmod(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                             int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             ADD_COPY(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)],
                      dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_AddCopyRGBAToRGBACmod(uint32_t * src, int srcw, uint32_t * dst,
                              int dstw, int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             A_VAL(dst) = amod[A_VAL(src)];
             ADD_COPY(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)],
                      dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_AddCopyRGBToRGBACmod(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                             int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;
   uint8_t             am = amod[255];

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             A_VAL(dst) = am;
             ADD_COPY(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)],
                      dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

/* SUBTRACT OPS */

static void
__imlib_SubBlendRGBAToRGBCmod(uint32_t * src, int srcw, uint32_t * dst,
                              int dstw, int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;
             uint8_t             a;

             a = amod[A_VAL(src)];
             switch (a)
               {
               case 0:
                  break;
               case 255:
                  SUB_COPY(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)],
                           dst);
                  break;
               default:
                  BLEND_SUB(rmod[R_VAL(src)], gmod[G_VAL(src)],
                            bmod[B_VAL(src)], a, dst);
                  break;
               }
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_SubBlendRGBAToRGBACmod(uint32_t * src, int srcw, uint32_t * dst,
                               int dstw, int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;
             uint8_t             a, aa;

             aa = amod[A_VAL(src)];
             switch (aa)
               {
               case 0:
                  break;
               case 255:
                  A_VAL(dst) = 0xff;
                  SUB_COPY(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)],
                           dst);
                  break;
               default:
                  a = pow_lut[aa][A_VAL(dst)];
                  BLEND_COLOR(aa, A_VAL(dst), 255, A_VAL(dst));
                  BLEND_SUB(rmod[R_VAL(src)], gmod[G_VAL(src)],
                            bmod[B_VAL(src)], a, dst);
                  break;
               }
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_SubBlendRGBToRGBCmod(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                             int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;
   uint8_t             am = amod[255];

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             BLEND_SUB(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)], am,
                       dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_SubBlendRGBToRGBACmod(uint32_t * src, int srcw, uint32_t * dst,
                              int dstw, int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;
   uint8_t             am = amod[255];

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;
             uint8_t             a;

             a = pow_lut[am][A_VAL(dst)];
             BLEND_COLOR(am, A_VAL(dst), 255, A_VAL(dst));
             BLEND_SUB(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)], a,
                       dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_SubCopyRGBAToRGBCmod(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                             int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             SUB_COPY(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)],
                      dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_SubCopyRGBAToRGBACmod(uint32_t * src, int srcw, uint32_t * dst,
                              int dstw, int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             A_VAL(dst) = amod[A_VAL(src)];
             SUB_COPY(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)],
                      dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_SubCopyRGBToRGBACmod(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                             int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;
   uint8_t             am = amod[255];

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             A_VAL(dst) = am;
             SUB_COPY(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)],
                      dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

/* RESHADE OPS */

static void
__imlib_ReBlendRGBAToRGBCmod(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                             int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;
             uint8_t             a;

             a = amod[A_VAL(src)];
             switch (a)
               {
               case 0:
                  break;
               case 255:
                  RE_COPY(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)],
                          dst);
                  break;
               default:
                  BLEND_RE(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)],
                           a, dst);
                  break;
               }
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_ReBlendRGBAToRGBACmod(uint32_t * src, int srcw, uint32_t * dst,
                              int dstw, int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;
             uint8_t             a, aa;

             aa = amod[A_VAL(src)];
             switch (aa)
               {
               case 0:
                  break;
               case 255:
                  A_VAL(dst) = 0xff;
                  RE_COPY(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)],
                          dst);
                  break;
               default:
                  a = pow_lut[aa][A_VAL(dst)];
                  BLEND_COLOR(aa, A_VAL(dst), 255, A_VAL(dst));
                  BLEND_RE(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)],
                           a, dst);
               }
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_ReBlendRGBToRGBCmod(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                            int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;
   uint8_t             am = amod[255];

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             BLEND_RE(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)], am,
                      dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_ReBlendRGBToRGBACmod(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                             int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;
   uint8_t             am = amod[255];

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;
             uint8_t             a;

             a = pow_lut[am][A_VAL(dst)];
             BLEND_COLOR(am, A_VAL(dst), 255, A_VAL(dst));
             BLEND_RE(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)], a,
                      dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_ReCopyRGBAToRGBCmod(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                            int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             RE_COPY(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)], dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_ReCopyRGBAToRGBACmod(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                             int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             A_VAL(dst) = amod[A_VAL(src)];
             RE_COPY(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)], dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

static void
__imlib_ReCopyRGBToRGBACmod(uint32_t * src, int srcw, uint32_t * dst, int dstw,
                            int w, int h, ImlibColorModifier * cm)
{
   int                 src_step = (srcw - w), dst_step = (dstw - w), ww = w;
   uint8_t            *amod = cm->alpha_mapping, *rmod = cm->red_mapping,
      *gmod = cm->green_mapping, *bmod = cm->blue_mapping;
   uint8_t             am = amod[255];

   while (h--)
     {
        while (w--)
          {
             uint32_t            tmp;

             A_VAL(dst) = am;
             RE_COPY(rmod[R_VAL(src)], gmod[G_VAL(src)], bmod[B_VAL(src)], dst);
             src++;
             dst++;
          }
        src += src_step;
        dst += dst_step;
        w = ww;
     }
}

/*\ Equivalent functions \*/

#define __imlib_CopyRGBToRGB			__imlib_CopyRGBToRGBA
#define __imlib_BlendRGBToRGB			__imlib_CopyRGBToRGB
#define __imlib_BlendRGBToRGBA			__imlib_CopyRGBToRGBA
#define __imlib_mmx_copy_rgb_to_rgb		__imlib_mmx_copy_rgb_to_rgba
#define __imlib_mmx_blend_rgb_to_rgb		__imlib_mmx_copy_rgb_to_rgb
#define __imlib_mmx_blend_rgb_to_rgba		__imlib_mmx_copy_rgb_to_rgba
#define __imlib_amd64_copy_rgb_to_rgb		__imlib_amd64_copy_rgb_to_rgba
#define __imlib_amd64_blend_rgb_to_rgb		__imlib_amd64_copy_rgb_to_rgb
#define __imlib_amd64_blend_rgb_to_rgba		__imlib_amd64_copy_rgb_to_rgba
#define __imlib_CopyRGBToRGBCmod		__imlib_CopyRGBAToRGBCmod
#define __imlib_mmx_copy_rgb_to_rgb_cmod	__imlib_mmx_copy_rgba_to_rgb_cmod
#define __imlib_amd64_copy_rgb_to_rgb_cmod	__imlib_amd64_copy_rgba_to_rgb_cmod

#define __imlib_AddCopyRGBToRGB			__imlib_AddCopyRGBAToRGB
#define __imlib_AddBlendRGBToRGB		__imlib_AddCopyRGBToRGB
#define __imlib_AddBlendRGBToRGBA		__imlib_AddCopyRGBToRGBA
#define __imlib_mmx_add_copy_rgb_to_rgb		__imlib_mmx_add_copy_rgba_to_rgb
#define __imlib_mmx_add_blend_rgb_to_rgb	__imlib_mmx_add_copy_rgb_to_rgb
#define __imlib_mmx_add_blend_rgb_to_rgba	__imlib_mmx_add_copy_rgb_to_rgba
#define __imlib_amd64_add_copy_rgb_to_rgb      	__imlib_amd64_add_copy_rgba_to_rgb
#define __imlib_amd64_add_blend_rgb_to_rgb	__imlib_amd64_add_copy_rgb_to_rgb
#define __imlib_amd64_add_blend_rgb_to_rgba	__imlib_amd64_add_copy_rgb_to_rgba
#define __imlib_AddCopyRGBToRGBCmod		__imlib_AddCopyRGBAToRGBCmod
#define __imlib_mmx_add_copy_rgb_to_rgb_cmod	__imlib_mmx_add_copy_rgb_to_rgba_cmod
#define __imlib_amd64_add_copy_rgb_to_rgb_cmod	__imlib_amd64_add_copy_rgb_to_rgba_cmod

#define __imlib_SubCopyRGBToRGB			__imlib_SubCopyRGBAToRGB
#define __imlib_SubBlendRGBToRGB		__imlib_SubCopyRGBToRGB
#define __imlib_SubBlendRGBToRGBA		__imlib_SubCopyRGBToRGBA
#define __imlib_mmx_subtract_copy_rgb_to_rgba	__imlib_mmx_subtract_copy_rgba_to_rgba
#define __imlib_mmx_subtract_copy_rgb_to_rgb	__imlib_mmx_subtract_copy_rgba_to_rgb
#define __imlib_mmx_subtract_blend_rgb_to_rgb	__imlib_mmx_subtract_copy_rgb_to_rgb
#define __imlib_mmx_subtract_blend_rgb_to_rgba	__imlib_mmx_subtract_copy_rgb_to_rgba
#define __imlib_amd64_subtract_copy_rgb_to_rgb	__imlib_amd64_subtract_copy_rgba_to_rgb
#define __imlib_amd64_subtract_blend_rgb_to_rgb	__imlib_amd64_subtract_copy_rgb_to_rgb
#define __imlib_amd64_subtract_blend_rgb_to_rgba	__imlib_amd64_subtract_copy_rgb_to_rgba
#define __imlib_SubCopyRGBToRGBCmod		__imlib_SubCopyRGBAToRGBCmod
#define __imlib_mmx_subtract_copy_rgb_to_rgb_cmod	__imlib_mmx_subtract_copy_rgb_to_rgba_cmod
#define __imlib_amd64_subtract_copy_rgb_to_rgb_cmod	__imlib_amd64_subtract_copy_rgb_to_rgba_cmod

#define __imlib_ReCopyRGBToRGB			__imlib_ReCopyRGBAToRGB
#define __imlib_ReBlendRGBToRGB			__imlib_ReCopyRGBToRGB
#define __imlib_ReBlendRGBToRGBA		__imlib_ReCopyRGBToRGBA
#define __imlib_mmx_reshade_copy_rgb_to_rgba	__imlib_mmx_reshade_copy_rgba_to_rgba
#define __imlib_mmx_reshade_copy_rgb_to_rgb	__imlib_mmx_reshade_copy_rgba_to_rgb
#define __imlib_mmx_reshade_blend_rgb_to_rgb	__imlib_mmx_reshade_copy_rgb_to_rgb
#define __imlib_mmx_reshade_blend_rgb_to_rgba	__imlib_mmx_reshade_copy_rgb_to_rgba
#define __imlib_amd64_reshade_copy_rgb_to_rgb	__imlib_amd64_reshade_copy_rgba_to_rgb
#define __imlib_amd64_reshade_blend_rgb_to_rgb	__imlib_amd64_reshade_copy_rgb_to_rgb
#define __imlib_amd64_reshade_blend_rgb_to_rgba	__imlib_amd64_reshade_copy_rgb_to_rgba
#define __imlib_ReCopyRGBToRGBCmod		__imlib_ReCopyRGBAToRGBCmod
#define __imlib_mmx_reshade_copy_rgb_to_rgb_cmod	__imlib_mmx_reshade_copy_rgb_to_rgba_cmod
#define __imlib_amd64_reshade_copy_rgb_to_rgb_cmod	__imlib_amd64_reshade_copy_rgb_to_rgba_cmod

ImlibBlendFunction
__imlib_GetBlendFunction(ImlibOp op, char blend, char merge_alpha, char rgb_src,
                         ImlibColorModifier * cm)
{
   /*\ [ mmx ][ operation ][ cmod ][ merge_alpha ][ rgb_src ][ blend ] \ */
   static const ImlibBlendFunction ibfuncs[][4][2][2][2][2] = {
      /*\ OP_COPY \ */
      {{{{{__imlib_CopyRGBAToRGB, __imlib_BlendRGBAToRGB},
          {__imlib_CopyRGBToRGB, __imlib_BlendRGBToRGB}},
         {{__imlib_CopyRGBAToRGBA, __imlib_BlendRGBAToRGBA},
          {__imlib_CopyRGBToRGBA, __imlib_BlendRGBToRGBA}}},

        {{{__imlib_CopyRGBAToRGBCmod, __imlib_BlendRGBAToRGBCmod},
          {__imlib_CopyRGBToRGBCmod, __imlib_BlendRGBToRGBCmod}},
         {{__imlib_CopyRGBAToRGBACmod, __imlib_BlendRGBAToRGBACmod},
          {__imlib_CopyRGBToRGBACmod, __imlib_BlendRGBToRGBACmod}}}},
       /*\ OP_ADD \ */
       {{{{__imlib_AddCopyRGBAToRGB, __imlib_AddBlendRGBAToRGB},
          {__imlib_AddCopyRGBToRGB, __imlib_AddBlendRGBToRGB}},
         {{__imlib_AddCopyRGBAToRGBA, __imlib_AddBlendRGBAToRGBA},
          {__imlib_AddCopyRGBToRGBA, __imlib_AddBlendRGBToRGBA}}},

        {{{__imlib_AddCopyRGBAToRGBCmod, __imlib_AddBlendRGBAToRGBCmod},
          {__imlib_AddCopyRGBToRGBCmod, __imlib_AddBlendRGBToRGBCmod}},
         {{__imlib_AddCopyRGBAToRGBACmod, __imlib_AddBlendRGBAToRGBACmod},
          {__imlib_AddCopyRGBToRGBACmod, __imlib_AddBlendRGBToRGBACmod}}}},
       /*\ OP_SUBTRACT \ */
       {{{{__imlib_SubCopyRGBAToRGB, __imlib_SubBlendRGBAToRGB},
          {__imlib_SubCopyRGBToRGB, __imlib_SubBlendRGBToRGB}},
         {{__imlib_SubCopyRGBAToRGBA, __imlib_SubBlendRGBAToRGBA},
          {__imlib_SubCopyRGBToRGBA, __imlib_SubBlendRGBToRGBA}}},

        {{{__imlib_SubCopyRGBAToRGBCmod, __imlib_SubBlendRGBAToRGBCmod},
          {__imlib_SubCopyRGBToRGBCmod, __imlib_SubBlendRGBToRGBCmod}},
         {{__imlib_SubCopyRGBAToRGBACmod, __imlib_SubBlendRGBAToRGBACmod},
          {__imlib_SubCopyRGBToRGBACmod, __imlib_SubBlendRGBToRGBACmod}}}},
       /*\ OP_RESHADE \ */
       {{{{__imlib_ReCopyRGBAToRGB, __imlib_ReBlendRGBAToRGB},
          {__imlib_ReCopyRGBToRGB, __imlib_ReBlendRGBToRGB}},
         {{__imlib_ReCopyRGBAToRGBA, __imlib_ReBlendRGBAToRGBA},
          {__imlib_ReCopyRGBToRGBA, __imlib_ReBlendRGBToRGBA}}},

        {{{__imlib_ReCopyRGBAToRGBCmod, __imlib_ReBlendRGBAToRGBCmod},
          {__imlib_ReCopyRGBToRGBCmod, __imlib_ReBlendRGBToRGBCmod}},
         {{__imlib_ReCopyRGBAToRGBACmod, __imlib_ReBlendRGBAToRGBACmod},
          {__imlib_ReCopyRGBToRGBACmod, __imlib_ReBlendRGBToRGBACmod}}}}},

#ifdef DO_MMX_ASM
      /*\ OP_COPY \ */
      {{{{{__imlib_mmx_copy_rgba_to_rgb, __imlib_mmx_blend_rgba_to_rgb},
          {__imlib_mmx_copy_rgb_to_rgb, __imlib_mmx_blend_rgb_to_rgb}},
         {{__imlib_mmx_copy_rgba_to_rgba,
           __imlib_BlendRGBAToRGBA /*__imlib_mmx_blend_rgba_to_rgba*/ },
          {__imlib_mmx_copy_rgb_to_rgba, __imlib_mmx_blend_rgb_to_rgba}}},

        {{{__imlib_mmx_copy_rgba_to_rgb_cmod,
           __imlib_mmx_blend_rgba_to_rgb_cmod},
          {__imlib_mmx_copy_rgb_to_rgb_cmod,
           __imlib_mmx_blend_rgb_to_rgb_cmod}},
         {{__imlib_mmx_copy_rgba_to_rgba_cmod,
           __imlib_BlendRGBAToRGBACmod /*__imlib_mmx_blend_rgba_to_rgba_cmod*/
           },
          {__imlib_mmx_copy_rgb_to_rgba_cmod,
           __imlib_BlendRGBToRGBACmod /*__imlib_mmx_blend_rgb_to_rgba_cmod*/
           }}}},
       /*\ OP_ADD \ */
       {{{{__imlib_mmx_add_copy_rgba_to_rgb, __imlib_mmx_add_blend_rgba_to_rgb},
          {__imlib_mmx_add_copy_rgb_to_rgb, __imlib_mmx_add_blend_rgb_to_rgb}},
         {{__imlib_AddCopyRGBAToRGBA /*__imlib_mmx_add_copy_rgba_to_rgba*/ ,
           __imlib_AddBlendRGBAToRGBA /*__imlib_mmx_add_blend_rgba_to_rgba*/ },
          {__imlib_mmx_add_copy_rgb_to_rgba,
           __imlib_mmx_add_blend_rgb_to_rgba}}},

        {{{__imlib_mmx_add_copy_rgba_to_rgb_cmod,
           __imlib_mmx_add_blend_rgba_to_rgb_cmod},
          {__imlib_mmx_add_copy_rgb_to_rgb_cmod,
           __imlib_mmx_add_blend_rgb_to_rgb_cmod}},
         {{__imlib_AddCopyRGBAToRGBACmod
           /*__imlib_mmx_add_copy_rgba_to_rgba_cmod*/ ,
           __imlib_AddBlendRGBAToRGBACmod
           /*__imlib_mmx_add_blend_rgba_to_rgba_cmod*/ },
          {__imlib_AddCopyRGBToRGBACmod
           /*__imlib_mmx_add_copy_rgb_to_rgba_cmod*/ ,
           __imlib_AddBlendRGBToRGBACmod
           /*__imlib_mmx_add_blend_rgb_to_rgba_cmod*/ }}}},
       /*\ OP_SUBTRACT \ */
       {{{{__imlib_mmx_subtract_copy_rgba_to_rgb,
           __imlib_mmx_subtract_blend_rgba_to_rgb},
          {__imlib_mmx_subtract_copy_rgb_to_rgb,
           __imlib_mmx_subtract_blend_rgb_to_rgb}},
         {{__imlib_SubCopyRGBAToRGBA /*__imlib_mmx_subtract_copy_rgba_to_rgba*/
           ,
           __imlib_SubBlendRGBAToRGBA
           /*__imlib_mmx_subtract_blend_rgba_to_rgba*/ },
          {__imlib_mmx_subtract_copy_rgb_to_rgba,
           __imlib_mmx_subtract_blend_rgb_to_rgba}}},

        {{{__imlib_mmx_subtract_copy_rgba_to_rgb_cmod,
           __imlib_mmx_subtract_blend_rgba_to_rgb_cmod},
          {__imlib_mmx_subtract_copy_rgb_to_rgb_cmod,
           __imlib_mmx_subtract_blend_rgb_to_rgb_cmod}},
         {{__imlib_SubCopyRGBAToRGBACmod
           /*__imlib_mmx_subtract_copy_rgba_to_rgba_cmod*/ ,
           __imlib_SubBlendRGBAToRGBACmod
           /*__imlib_mmx_subtract_blend_rgba_to_rgba_cmod*/ },
          {__imlib_SubCopyRGBToRGBACmod
           /*__imlib_mmx_subtract_copy_rgb_to_rgba_cmod*/ ,
           __imlib_SubBlendRGBToRGBACmod
           /*__imlib_mmx_subtract_blend_rgb_to_rgba_cmod*/ }}}},
       /*\ OP_RESHADE \ */
       {{{{__imlib_mmx_reshade_copy_rgba_to_rgb,
           __imlib_mmx_reshade_blend_rgba_to_rgb},
          {__imlib_mmx_reshade_copy_rgb_to_rgb,
           __imlib_mmx_reshade_blend_rgb_to_rgb}},
         {{__imlib_ReCopyRGBAToRGBA /*__imlib_mmx_reshade_copy_rgba_to_rgba*/ ,
           __imlib_ReBlendRGBAToRGBA /*__imlib_mmx_reshade_blend_rgba_to_rgba*/
           },
          {__imlib_mmx_reshade_copy_rgb_to_rgba,
           __imlib_mmx_reshade_blend_rgb_to_rgba}}},

        {{{__imlib_mmx_reshade_copy_rgba_to_rgb_cmod,
           __imlib_mmx_reshade_blend_rgba_to_rgb_cmod},
          {__imlib_mmx_reshade_copy_rgb_to_rgb_cmod,
           __imlib_mmx_reshade_blend_rgb_to_rgb_cmod}},
         {{__imlib_ReCopyRGBAToRGBACmod
           /*__imlib_mmx_reshade_copy_rgba_to_rgba_cmod*/ ,
           __imlib_ReBlendRGBAToRGBACmod
           /*__imlib_mmx_reshade_blend_rgba_to_rgba_cmod*/ },
          {__imlib_ReCopyRGBToRGBACmod
           /*__imlib_mmx_reshade_copy_rgb_to_rgba_cmod*/ ,
           __imlib_ReBlendRGBToRGBACmod
           /*__imlib_mmx_reshade_blend_rgb_to_rgba_cmod*/ }}}}},
#elif DO_AMD64_ASM
      /*\ OP_COPY \ */
      {{{{{__imlib_amd64_copy_rgba_to_rgb, __imlib_amd64_blend_rgba_to_rgb},
          {__imlib_amd64_copy_rgb_to_rgb, __imlib_amd64_blend_rgb_to_rgb}},
         {{__imlib_amd64_copy_rgba_to_rgba, __imlib_amd64_blend_rgba_to_rgba},
          {__imlib_amd64_copy_rgb_to_rgba, __imlib_amd64_blend_rgb_to_rgba}}},

        {{{__imlib_amd64_copy_rgba_to_rgb_cmod,
           __imlib_amd64_blend_rgba_to_rgb_cmod},
          {__imlib_amd64_copy_rgb_to_rgb_cmod,
           __imlib_amd64_blend_rgb_to_rgb_cmod}},
         {{__imlib_amd64_copy_rgba_to_rgba_cmod,
           __imlib_amd64_blend_rgba_to_rgba_cmod},
          {__imlib_amd64_copy_rgb_to_rgba_cmod,
           __imlib_amd64_blend_rgb_to_rgba_cmod}}}},
       /*\ OP_ADD \ */
       {{{{__imlib_amd64_add_copy_rgba_to_rgb,
           __imlib_amd64_add_blend_rgba_to_rgb},
          {__imlib_amd64_add_copy_rgb_to_rgb,
           __imlib_amd64_add_blend_rgb_to_rgb}},
         {{__imlib_amd64_add_copy_rgba_to_rgba,
           __imlib_amd64_add_blend_rgba_to_rgba},
          {__imlib_amd64_add_copy_rgb_to_rgba,
           __imlib_amd64_add_blend_rgb_to_rgba}}},

        {{{__imlib_amd64_add_copy_rgba_to_rgb_cmod,
           __imlib_amd64_add_blend_rgba_to_rgb_cmod},
          {__imlib_amd64_add_copy_rgb_to_rgb_cmod,
           __imlib_amd64_add_blend_rgb_to_rgb_cmod}},
         {{__imlib_amd64_add_copy_rgba_to_rgba_cmod,
           __imlib_amd64_add_blend_rgba_to_rgba_cmod},
          {__imlib_amd64_add_copy_rgb_to_rgba_cmod,
           __imlib_amd64_add_blend_rgb_to_rgba_cmod}}}},
       /*\ OP_SUBTRACT \ */
       {{{{__imlib_amd64_subtract_copy_rgba_to_rgb,
           __imlib_amd64_subtract_blend_rgba_to_rgb},
          {__imlib_amd64_subtract_copy_rgb_to_rgb,
           __imlib_amd64_subtract_blend_rgb_to_rgb}},
         {{__imlib_amd64_subtract_copy_rgba_to_rgba,
           __imlib_amd64_subtract_blend_rgba_to_rgba},
          {__imlib_amd64_subtract_copy_rgb_to_rgba,
           __imlib_amd64_subtract_blend_rgb_to_rgba}}},

        {{{__imlib_amd64_subtract_copy_rgba_to_rgb_cmod,
           __imlib_amd64_subtract_blend_rgba_to_rgb_cmod},
          {__imlib_amd64_subtract_copy_rgb_to_rgb_cmod,
           __imlib_amd64_subtract_blend_rgb_to_rgb_cmod}},
         {{__imlib_amd64_subtract_copy_rgba_to_rgba_cmod,
           __imlib_amd64_subtract_blend_rgba_to_rgba_cmod},
          {__imlib_amd64_subtract_copy_rgb_to_rgba_cmod,
           __imlib_amd64_subtract_blend_rgb_to_rgba_cmod}}}},
       /*\ OP_RESHADE \ */
       {{{{__imlib_amd64_reshade_copy_rgba_to_rgb,
           __imlib_amd64_reshade_blend_rgba_to_rgb},
          {__imlib_amd64_reshade_copy_rgb_to_rgb,
           __imlib_amd64_reshade_blend_rgb_to_rgb}},
         {{__imlib_amd64_reshade_copy_rgba_to_rgba,
           __imlib_amd64_reshade_blend_rgba_to_rgba},
          {__imlib_amd64_reshade_copy_rgb_to_rgba,
           __imlib_amd64_reshade_blend_rgb_to_rgba}}},

        {{{__imlib_amd64_reshade_copy_rgba_to_rgb_cmod,
           __imlib_amd64_reshade_blend_rgba_to_rgb_cmod},
          {__imlib_amd64_reshade_copy_rgb_to_rgb_cmod,
           __imlib_amd64_reshade_blend_rgb_to_rgb_cmod}},
         {{__imlib_amd64_reshade_copy_rgba_to_rgba_cmod,
           __imlib_amd64_reshade_blend_rgba_to_rgba_cmod},
          {__imlib_amd64_reshade_copy_rgb_to_rgba_cmod,
           __imlib_amd64_reshade_blend_rgb_to_rgba_cmod}}}}},
#endif
   };

   ImlibBlendFunction  bfun;
   int                 do_mmx = 0;

   if (op < OP_COPY || op > OP_RESHADE)
      return NULL;

#if defined(DO_MMX_ASM) || defined(DO_AMD64_ASM)
   do_mmx = __imlib_do_asm();
#endif

   if (cm && rgb_src && (A_CMOD(cm, 0xff) == 0xff))
      blend = 0;
   if (blend && cm && rgb_src && (A_CMOD(cm, 0xff) == 0))
      return NULL;

   bfun = ibfuncs[!!do_mmx][op][!!cm][!!merge_alpha][!!rgb_src][!!blend];

   return bfun;
}

void
__imlib_BlendRGBAToData(uint32_t * src, int src_w, int src_h, uint32_t * dst,
                        int dst_w, int dst_h, int sx, int sy, int dx, int dy,
                        int w, int h, char blend, char merge_alpha,
                        ImlibColorModifier * cm, ImlibOp op, char rgb_src)
{
   ImlibBlendFunction  blender;

   if (sx < 0)
     {
        w += sx;
        dx -= sx;
        sx = 0;
     }
   if (sy < 0)
     {
        h += sy;
        dy -= sy;
        sy = 0;
     }
   if (dx < 0)
     {
        w += dx;
        sx -= dx;
        dx = 0;
     }
   if (dy < 0)
     {
        h += dy;
        sy -= dy;
        dy = 0;
     }
   if ((w <= 0) || (h <= 0))
      return;
   if ((sx + w) > src_w)
      w = src_w - sx;
   if ((sy + h) > src_h)
      h = src_h - sy;
   if ((dx + w) > dst_w)
      w = dst_w - dx;
   if ((dy + h) > dst_h)
      h = dst_h - dy;
   if ((w <= 0) || (h <= 0))
      return;

   __imlib_build_pow_lut();
   blender = __imlib_GetBlendFunction(op, blend, merge_alpha, rgb_src, cm);
   if (blender)
      blender(src + (sy * src_w) + sx, src_w,
              dst + (dy * dst_w) + dx, dst_w, w, h, cm);
}

#define LINESIZE 16

void
__imlib_BlendImageToImage(ImlibImage * im_src, ImlibImage * im_dst,
                          char aa, char blend, char merge_alpha,
                          int ssx, int ssy, int ssw, int ssh,
                          int ddx, int ddy, int ddw, int ddh,
                          ImlibColorModifier * cm, ImlibOp op,
                          int clx, int cly, int clw, int clh)
{
   char                rgb_src = 0;

   if (__imlib_LoadImageData(im_src))
      return;
   if (__imlib_LoadImageData(im_dst))
      return;

   /* don't do anything if we have a 0 width or height image to render */
   /* if the input rect size < 0 don't render either */
   if (ssw <= 0 || ssh <= 0 || ddw == 0 || ddh == 0)
      return;

   if ((ssw == ddw) && (ssh == ddh))
     {
        if (!im_dst->has_alpha)
           merge_alpha = 0;
        if (!im_src->has_alpha)
          {
             rgb_src = 1;
             if (merge_alpha)
                blend = 1;
          }
        if (clw)
          {
             int                 px, py;

             px = ddx;
             py = ddy;
             CLIP(ddx, ddy, ddw, ddh, clx, cly, clw, clh);
             if (ddw <= 0 || ddh <= 0)
                return;
             px = ddx - px;
             py = ddy - py;
             ssx += px;
             ssy += py;
          }

        __imlib_BlendRGBAToData(im_src->data, im_src->w, im_src->h,
                                im_dst->data, im_dst->w, im_dst->h,
                                ssx, ssy,
                                ddx, ddy,
                                ddw, ddh, blend, merge_alpha, cm, op, rgb_src);
     }
   else
     {
        ImlibScaleInfo     *scaleinfo = NULL;
        uint32_t           *buf = NULL;
        int                 sx, sy, sw, sh, dx, dy, dw, dh, dxx, dyy, y2, x2;
        int                 psx, psy, psw, psh;
        int                 y, h, hh;

        sx = ssx;
        sy = ssy;
        sw = ssw;
        sh = ssh;
        dx = ddx;
        dy = ddy;
        dw = abs(ddw);
        dh = abs(ddh);

        /* clip the source rect to be within the actual image */
        psx = sx;
        psy = sy;
        psw = sw;
        psh = sh;
        CLIP(sx, sy, sw, sh, 0, 0, im_src->w, im_src->h);
        if (sw <= 0 || sh <= 0)
           return;
        if (psx != sx)
           dx += ((sx - psx) * abs(ddw)) / ssw;
        if (psy != sy)
           dy += ((sy - psy) * abs(ddh)) / ssh;
        if (psw != sw)
           dw = (dw * sw) / psw;
        if (psh != sh)
           dh = (dh * sh) / psh;
        if (dw <= 0 || dh <= 0)
           return;

        /* clip output coords to clipped input coords */
        psx = dx;
        psy = dy;
        psw = dw;
        psh = dh;
        x2 = sx;
        y2 = sy;
        CLIP(dx, dy, dw, dh, 0, 0, im_dst->w, im_dst->h);
        if (dw <= 0 || dh <= 0)
           return;
        if (clw)
          {
             CLIP(dx, dy, dw, dh, clx, cly, clw, clh);
             if (dw <= 0 || dh <= 0)
                return;
          }
        if (psw != dw)
           sw = (sw * dw) / psw;
        if (psh != dh)
           sh = (sh * dh) / psh;
        dxx = dx - psx;
        dyy = dy - psy;
        dxx += (x2 * abs(ddw)) / ssw;
        dyy += (y2 * abs(ddh)) / ssh;

        if (sw == 0)
           sw = 1;
        if (sh == 0)
           sh = 1;
        /* do a second check to see if we now have invalid coords */
        /* don't do anything if we have a 0 width or height image to render */
        /* if the input rect size < 0 don't render either */
        if (sw <= 0 || sh <= 0)
           return;

        scaleinfo = __imlib_CalcScaleInfo(im_src, ssw, ssh, ddw, ddh, aa);
        if (!scaleinfo)
           return;
        /* if we are scaling the image at all make a scaling buffer */
        /* allocate a buffer to render scaled RGBA data into */
        buf = malloc(dw * LINESIZE * sizeof(uint32_t));
        if (!buf)
          {
             __imlib_FreeScaleInfo(scaleinfo);
             return;
          }

        /* setup h */
        h = dh;
        if (!im_dst->has_alpha)
           merge_alpha = 0;
        if (!im_src->has_alpha)
          {
             rgb_src = 1;
             if (merge_alpha)
                blend = 1;
          }

        /* scale in LINESIZE Y chunks and convert to depth */
        for (y = 0; y < dh; y += LINESIZE)
          {
             hh = LINESIZE;
             if (h < LINESIZE)
                hh = h;
             /* scale the imagedata for this LINESIZE lines chunk of image */
             if (aa)
               {
                  if (im_src->has_alpha)
                     __imlib_ScaleAARGBA(scaleinfo, buf, dxx, dyy + y,
                                         0, 0, dw, hh, dw, im_src->w);
                  else
                     __imlib_ScaleAARGB(scaleinfo, buf, dxx, dyy + y,
                                        0, 0, dw, hh, dw, im_src->w);
               }
             else
                __imlib_ScaleSampleRGBA(scaleinfo, buf, dxx, dyy + y,
                                        0, 0, dw, hh, dw);
             __imlib_BlendRGBAToData(buf, dw, hh,
                                     im_dst->data, im_dst->w,
                                     im_dst->h,
                                     0, 0, dx, dy + y, dw, dh,
                                     blend, merge_alpha, cm, op, rgb_src);
             h -= LINESIZE;
          }
        /* free up our buffers and point tables */
        free(buf);
        __imlib_FreeScaleInfo(scaleinfo);
     }
}
