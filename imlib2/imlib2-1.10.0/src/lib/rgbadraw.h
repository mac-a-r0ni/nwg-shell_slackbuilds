#ifndef __RGBADRAW
#define __RGBADRAW 1

#include "types.h"

#define IN_SEGMENT(x, sx, sw) \
((unsigned)((x) - (sx)) < (unsigned)(sw))

#define IN_RANGE(x, y, w, h) \
( ((unsigned)(x) < (unsigned)(w)) && ((unsigned)(y) < (unsigned)(h)) )

#define IN_RECT(x, y, rx, ry, rw, rh) \
( ((unsigned)((x) - (rx)) < (unsigned)(rw)) && \
  ((unsigned)((y) - (ry)) < (unsigned)(rh)) )

#define DIV_255(a, x, tmp) \
do {                           \
 tmp = (x) + 0x80;             \
 a = (tmp + (tmp >> 8)) >> 8;  \
} while (0)

#define MULT(na, a0, a1, tmp) \
  DIV_255(na, (a0) * (a1), tmp)

typedef struct {
   int                 x, y;
} ImlibPoint;

typedef struct {
   int                 x, y, w, h;
} Imlib_Rectangle;

typedef struct {
   ImlibPoint         *points;
   int                 pointcount;
   int                 lx, rx;
   int                 ty, by;
} ImlibPoly;

/* image related operations: in rgbadraw.c */

void                __imlib_FlipImageHoriz(ImlibImage * im);
void                __imlib_FlipImageVert(ImlibImage * im);
void                __imlib_FlipImageBoth(ImlibImage * im);
void                __imlib_FlipImageDiagonal(ImlibImage * im, int direction);
void                __imlib_BlurImage(ImlibImage * im, int rad);
void                __imlib_SharpenImage(ImlibImage * im, int rad);
void                __imlib_TileImageHoriz(ImlibImage * im);
void                __imlib_TileImageVert(ImlibImage * im);

void                __imlib_copy_alpha_data(ImlibImage * src, ImlibImage * dst,
                                            int x, int y, int w, int h,
                                            int nx, int ny);

void                __imlib_copy_image_data(ImlibImage * im, int x, int y,
                                            int w, int h, int nx, int ny);

/* point and line drawing: in line.c */

ImlibUpdate        *__imlib_Point_DrawToImage(int x, int y, uint32_t color,
                                              ImlibImage * im,
                                              int clx, int cly,
                                              int clw, int clh,
                                              ImlibOp op, char blend,
                                              char make_updates);

ImlibUpdate        *__imlib_Line_DrawToImage(int x0, int y0, int x1, int y1,
                                             uint32_t color, ImlibImage * im,
                                             int clx, int cly, int clw, int clh,
                                             ImlibOp op, char blend,
                                             char anti_alias,
                                             char make_updates);

/* rectangle drawing and filling: in rectangle.c */

void                __imlib_Rectangle_DrawToImage(int xc, int yc, int w, int h,
                                                  uint32_t color,
                                                  ImlibImage * im, int clx,
                                                  int cly, int clw, int clh,
                                                  ImlibOp op, char blend);

void                __imlib_Rectangle_FillToImage(int xc, int yc, int w, int h,
                                                  uint32_t color,
                                                  ImlibImage * im, int clx,
                                                  int cly, int clw, int clh,
                                                  ImlibOp op, char blend);

/* ellipse drawing and filling: in ellipse.c */

void                __imlib_Ellipse_DrawToImage(int xc, int yc, int a, int b,
                                                uint32_t color, ImlibImage * im,
                                                int clx, int cly,
                                                int clw, int clh,
                                                ImlibOp op, char blend,
                                                char anti_alias);

void                __imlib_Ellipse_FillToImage(int xc, int yc, int a, int b,
                                                uint32_t color, ImlibImage * im,
                                                int clx, int cly,
                                                int clw, int clh,
                                                ImlibOp op, char blend,
                                                char anti_alias);

/* polygon handling functions: in polygon.c */

ImlibPoly          *__imlib_polygon_new(void);
void                __imlib_polygon_free(ImlibPoly * poly);
void                __imlib_polygon_add_point(ImlibPoly * poly, int x, int y);
unsigned char       __imlib_polygon_contains_point(ImlibPoly * poly,
                                                   int x, int y);
void                __imlib_polygon_get_bounds(ImlibPoly * poly,
                                               int *px1, int *py1,
                                               int *px2, int *py2);

/* polygon drawing and filling: in polygon.c */

void                __imlib_Polygon_DrawToImage(ImlibPoly * poly, char closed,
                                                uint32_t color, ImlibImage * im,
                                                int clx, int cly,
                                                int clw, int clh,
                                                ImlibOp op, char blend,
                                                char anti_alias);
void                __imlib_Polygon_FillToImage(ImlibPoly * poly,
                                                uint32_t color, ImlibImage * im,
                                                int clx, int cly, int clw,
                                                int clh, ImlibOp op, char blend,
                                                char anti_alias);

#endif
