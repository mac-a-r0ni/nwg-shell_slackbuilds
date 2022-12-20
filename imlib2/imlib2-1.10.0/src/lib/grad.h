#ifndef __GRAD
#define __GRAD 1

#include "types.h"

typedef struct _ImlibRangeColor {
   uint8_t             red, green, blue, alpha;
   int                 distance;
   struct _ImlibRangeColor *next;
} ImlibRangeColor;

typedef struct {
   ImlibRangeColor    *color;
} ImlibRange;

ImlibRange         *__imlib_CreateRange(void);
void                __imlib_FreeRange(ImlibRange * rg);
void                __imlib_AddRangeColor(ImlibRange * rg, uint8_t r, uint8_t g,
                                          uint8_t b, uint8_t a, int dist);
void                __imlib_DrawGradient(ImlibImage * im,
                                         int x, int y, int w, int h,
                                         ImlibRange * rg, double angle,
                                         ImlibOp op,
                                         int clx, int cly, int clw, int clh);
void                __imlib_DrawHsvaGradient(ImlibImage * im,
                                             int x, int y, int w, int h,
                                             ImlibRange * rg, double angle,
                                             ImlibOp op,
                                             int clx, int cly,
                                             int clw, int clh);

#endif
