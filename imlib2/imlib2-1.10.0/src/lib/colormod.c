#include "common.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "colormod.h"
#include "image.h"

static uint64_t     mod_count = 0;

ImlibColorModifier *
__imlib_CreateCmod(void)
{
   ImlibColorModifier *cm;
   int                 i;

   cm = malloc(sizeof(ImlibColorModifier));
   if (!cm)
      return NULL;
   cm->modification_count = mod_count;
   for (i = 0; i < 256; i++)
     {
        cm->red_mapping[i] = (uint8_t) i;
        cm->green_mapping[i] = (uint8_t) i;
        cm->blue_mapping[i] = (uint8_t) i;
        cm->alpha_mapping[i] = (uint8_t) i;
     }
   return cm;
}

void
__imlib_FreeCmod(ImlibColorModifier * cm)
{
   free(cm);
}

void
__imlib_CmodChanged(ImlibColorModifier * cm)
{
   mod_count++;
   cm->modification_count = mod_count;
}

void
__imlib_CmodSetTables(ImlibColorModifier * cm,
                      uint8_t * r, uint8_t * g, uint8_t * b, uint8_t * a)
{
   int                 i;

   for (i = 0; i < 256; i++)
     {
        if (r)
           cm->red_mapping[i] = r[i];
        if (g)
           cm->green_mapping[i] = g[i];
        if (b)
           cm->blue_mapping[i] = b[i];
        if (a)
           cm->alpha_mapping[i] = a[i];
     }
   __imlib_CmodChanged(cm);
}

void
__imlib_CmodReset(ImlibColorModifier * cm)
{
   int                 i;

   for (i = 0; i < 256; i++)
     {
        cm->red_mapping[i] = (uint8_t) i;
        cm->green_mapping[i] = (uint8_t) i;
        cm->blue_mapping[i] = (uint8_t) i;
        cm->alpha_mapping[i] = (uint8_t) i;
     }
   __imlib_CmodChanged(cm);
}

void
__imlib_DataCmodApply(uint32_t * data, int w, int h, int jump,
                      bool has_alpha, ImlibColorModifier * cm)
{
   int                 x, y;
   uint32_t           *p;

   /* We might be adding alpha */
   if (!has_alpha)
     {
        p = data;
        for (y = 0; y < h; y++)
          {
             for (x = 0; x < w; x++)
               {
                  R_VAL(p) = R_CMOD(cm, R_VAL(p));
                  G_VAL(p) = G_CMOD(cm, G_VAL(p));
                  B_VAL(p) = B_CMOD(cm, B_VAL(p));
                  p++;
               }
             p += jump;
          }
        return;
     }

   p = data;
   for (y = 0; y < h; y++)
     {
        for (x = 0; x < w; x++)
          {
             R_VAL(p) = R_CMOD(cm, R_VAL(p));
             G_VAL(p) = G_CMOD(cm, G_VAL(p));
             B_VAL(p) = B_CMOD(cm, B_VAL(p));
             A_VAL(p) = A_CMOD(cm, A_VAL(p));
             p++;
          }
        p += jump;
     }
}

void
__imlib_CmodGetTables(ImlibColorModifier * cm, uint8_t * r, uint8_t * g,
                      uint8_t * b, uint8_t * a)
{
   if (r)
      memcpy(r, cm->red_mapping, (256 * sizeof(uint8_t)));
   if (g)
      memcpy(g, cm->green_mapping, (256 * sizeof(uint8_t)));
   if (b)
      memcpy(b, cm->blue_mapping, (256 * sizeof(uint8_t)));
   if (a)
      memcpy(a, cm->alpha_mapping, (256 * sizeof(uint8_t)));
}

void
__imlib_CmodModBrightness(ImlibColorModifier * cm, double v)
{
   int                 i, val, val2;

   val = (int)(v * 255);
   for (i = 0; i < 256; i++)
     {
        val2 = (int)cm->red_mapping[i] + val;
        if (val2 < 0)
           val2 = 0;
        if (val2 > 255)
           val2 = 255;
        cm->red_mapping[i] = (uint8_t) val2;

        val2 = (int)cm->green_mapping[i] + val;
        if (val2 < 0)
           val2 = 0;
        if (val2 > 255)
           val2 = 255;
        cm->green_mapping[i] = (uint8_t) val2;

        val2 = (int)cm->blue_mapping[i] + val;
        if (val2 < 0)
           val2 = 0;
        if (val2 > 255)
           val2 = 255;
        cm->blue_mapping[i] = (uint8_t) val2;

        val2 = (int)cm->alpha_mapping[i] + val;
        if (val2 < 0)
           val2 = 0;
        if (val2 > 255)
           val2 = 255;
        cm->alpha_mapping[i] = (uint8_t) val2;
     }
}

void
__imlib_CmodModContrast(ImlibColorModifier * cm, double v)
{
   int                 i, val2;

   for (i = 0; i < 256; i++)
     {
        val2 = (int)(((double)cm->red_mapping[i] - 127) * v) + 127;
        if (val2 < 0)
           val2 = 0;
        if (val2 > 255)
           val2 = 255;
        cm->red_mapping[i] = (uint8_t) val2;

        val2 = (int)(((double)cm->green_mapping[i] - 127) * v) + 127;
        if (val2 < 0)
           val2 = 0;
        if (val2 > 255)
           val2 = 255;
        cm->green_mapping[i] = (uint8_t) val2;

        val2 = (int)(((double)cm->blue_mapping[i] - 127) * v) + 127;
        if (val2 < 0)
           val2 = 0;
        if (val2 > 255)
           val2 = 255;
        cm->blue_mapping[i] = (uint8_t) val2;

        val2 = (int)(((double)cm->alpha_mapping[i] - 127) * v) + 127;
        if (val2 < 0)
           val2 = 0;
        if (val2 > 255)
           val2 = 255;
        cm->alpha_mapping[i] = (uint8_t) val2;
     }
}

void
__imlib_CmodModGamma(ImlibColorModifier * cm, double v)
{
   int                 i, val2;

   if (v < 0.01)
      v = 0.01;
   for (i = 0; i < 256; i++)
     {
        val2 = (int)(pow(((double)cm->red_mapping[i] / 255), (1 / v)) * 255);
        if (val2 < 0)
           val2 = 0;
        if (val2 > 255)
           val2 = 255;
        cm->red_mapping[i] = (uint8_t) val2;

        val2 = (int)(pow(((double)cm->green_mapping[i] / 255), (1 / v)) * 255);
        if (val2 < 0)
           val2 = 0;
        if (val2 > 255)
           val2 = 255;
        cm->green_mapping[i] = (uint8_t) val2;

        val2 = (int)(pow(((double)cm->blue_mapping[i] / 255), (1 / v)) * 255);
        if (val2 < 0)
           val2 = 0;
        if (val2 > 255)
           val2 = 255;
        cm->blue_mapping[i] = (uint8_t) val2;

        val2 = (int)(pow(((double)cm->alpha_mapping[i] / 255), (1 / v)) * 255);
        if (val2 < 0)
           val2 = 0;
        if (val2 > 255)
           val2 = 255;
        cm->alpha_mapping[i] = (uint8_t) val2;
     }
}

#if 0
void
__imlib_ImageCmodApply(ImlibImage * im, ImlibColorModifier * cm)
{
   __imlib_DataCmodApply(im->data, im->w, im->h, 0, cm);
}
#endif
