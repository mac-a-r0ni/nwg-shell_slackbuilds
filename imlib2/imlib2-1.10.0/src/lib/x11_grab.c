#include "common.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "x11_grab.h"
#include "x11_ximage.h"

static char         _x_err = 0;
static uint8_t      rtab[256], gtab[256], btab[256];

static int
Tmp_HandleXError(Display * d, XErrorEvent * ev)
{
   _x_err = 1;
   return 0;
}

void
__imlib_GrabXImageToRGBA(uint32_t * data,
                         int x_dst, int y_dst, int w_dst, int h_dst,
                         Display * d, XImage * xim, XImage * mxim, Visual * v,
                         int depth,
                         int x_src, int y_src, int w_src, int h_src, int grab)
{
   int                 x, y, inx, iny;
   const uint32_t     *src;
   const uint16_t     *s16;
   uint32_t           *ptr;
   int                 pixel;
   uint16_t            p16;
   int                 bgr = 0;

   if (!data)
      return;

   if (grab)
      XGrabServer(d);           /* This may prevent the image to be changed under our feet */

   if (v->blue_mask > v->red_mask)
      bgr = 1;

   if (x_src < 0)
      inx = -x_src;
   else
      inx = x_dst;
   if (y_src < 0)
      iny = -y_src;
   else
      iny = y_dst;

   /* go thru the XImage and convert */

   if ((depth == 24) && (xim->bits_per_pixel == 32))
      depth = 25;               /* fake depth meaning 24 bit in 32 bpp ximage */

   /* data needs swapping */
#ifdef WORDS_BIGENDIAN
   if (xim->bitmap_bit_order == LSBFirst)
#else
   if (xim->bitmap_bit_order == MSBFirst)
#endif
     {
        switch (depth)
          {
          case 0:
          case 1:
          case 2:
          case 3:
          case 4:
          case 5:
          case 6:
          case 7:
          case 8:
             break;
          case 15:
          case 16:
             for (y = 0; y < h_src; y++)
               {
                  unsigned short     *tmp;

                  tmp =
                     (unsigned short *)(xim->data + (xim->bytes_per_line * y));
                  for (x = 0; x < w_src; x++)
                    {
                       *tmp = SWAP16(*tmp);
                       tmp++;
                    }
               }
             break;
          case 24:
          case 25:
          case 30:
          case 32:
             for (y = 0; y < h_src; y++)
               {
                  unsigned int       *tmp;

                  tmp = (unsigned int *)(xim->data + (xim->bytes_per_line * y));
                  for (x = 0; x < w_src; x++)
                    {
                       *tmp = SWAP32(*tmp);
                       tmp++;
                    }
               }
             break;
          default:
             break;
          }
     }

   switch (depth)
     {
     case 0:
     case 1:
     case 2:
     case 3:
     case 4:
     case 5:
     case 6:
     case 7:
     case 8:
        if (mxim)
          {
             for (y = 0; y < h_src; y++)
               {
                  ptr = data + ((y + iny) * w_dst) + inx;
                  for (x = 0; x < w_src; x++)
                    {
                       pixel = XGetPixel(xim, x, y);
                       pixel = (btab[pixel & 0xff]) |
                          (gtab[pixel & 0xff] << 8) |
                          (rtab[pixel & 0xff] << 16);
                       if (XGetPixel(mxim, x, y))
                          pixel |= 0xff000000;
                       *ptr++ = pixel;
                    }
               }
          }
        else
          {
             for (y = 0; y < h_src; y++)
               {
                  ptr = data + ((y + iny) * w_dst) + inx;
                  for (x = 0; x < w_src; x++)
                    {
                       pixel = XGetPixel(xim, x, y);
                       *ptr++ = 0xff000000 |
                          (btab[pixel & 0xff]) |
                          (gtab[pixel & 0xff] << 8) |
                          (rtab[pixel & 0xff] << 16);
                    }
               }
          }
        break;
     case 16:
#undef MP
#undef RMSK
#undef GMSK
#undef BMSK
#undef RSH
#undef GSH
#undef BSH
#define MP(x, y) ((XGetPixel(mxim, (x), (y))) ? 0xff: 0)
#define RMSK  0x1f
#define GMSK  0x3f
#define BMSK  0x1f
#define RSH(p)  ((p) >> 11)
#define GSH(p)  ((p) >>  5)
#define BSH(p)  ((p) >>  0)
#define RVAL(p) (255 * (RSH(p) & RMSK)) / RMSK
#define GVAL(p) (255 * (GSH(p) & GMSK)) / GMSK
#define BVAL(p) (255 * (BSH(p) & BMSK)) / BMSK
        if (mxim)
          {
             for (y = 0; y < h_src; y++)
               {
                  s16 = (uint16_t *) (xim->data + (xim->bytes_per_line * y));
                  ptr = data + ((y + iny) * w_dst) + inx;
                  for (x = 0; x < w_src; x++)
                    {
                       p16 = *s16++;
                       *ptr++ =
                          PIXEL_ARGB(MP(x, y), RVAL(p16), GVAL(p16), BVAL(p16));
                    }
               }
          }
        else
          {
             for (y = 0; y < h_src; y++)
               {
                  s16 = (uint16_t *) (xim->data + (xim->bytes_per_line * y));
                  ptr = data + ((y + iny) * w_dst) + inx;
                  for (x = 0; x < w_src; x++)
                    {
                       p16 = *s16++;
                       *ptr++ =
                          PIXEL_ARGB(0xff, RVAL(p16), GVAL(p16), BVAL(p16));
                    }
               }
          }
        break;
     case 15:
#undef MP
#undef RMSK
#undef GMSK
#undef BMSK
#undef RSH
#undef GSH
#undef BSH
#define MP(x, y) ((XGetPixel(mxim, (x), (y))) ? 0xff: 0)
#define RMSK  0x1f
#define GMSK  0x1f
#define BMSK  0x1f
#define RSH(p)  ((p) >> 10)
#define GSH(p)  ((p) >>  5)
#define BSH(p)  ((p) >>  0)
#define RVAL(p) (255 * (RSH(p) & RMSK)) / RMSK
#define GVAL(p) (255 * (GSH(p) & GMSK)) / GMSK
#define BVAL(p) (255 * (BSH(p) & BMSK)) / BMSK
        if (mxim)
          {
             for (y = 0; y < h_src; y++)
               {
                  s16 = (uint16_t *) (xim->data + (xim->bytes_per_line * y));
                  ptr = data + ((y + iny) * w_dst) + inx;
                  for (x = 0; x < w_src; x++)
                    {
                       p16 = *s16++;
                       *ptr++ =
                          PIXEL_ARGB(MP(x, y), RVAL(p16), GVAL(p16), BVAL(p16));
                    }
               }
          }
        else
          {
             for (y = 0; y < h_src; y++)
               {
                  s16 = (uint16_t *) (xim->data + (xim->bytes_per_line * y));
                  ptr = data + ((y + iny) * w_dst) + inx;
                  for (x = 0; x < w_src; x++)
                    {
                       p16 = *s16++;
                       *ptr++ =
                          PIXEL_ARGB(0xff, RVAL(p16), GVAL(p16), BVAL(p16));
                    }
               }
          }
        break;
     case 24:
        if (bgr)
          {
             if (mxim)
               {
                  for (y = 0; y < h_src; y++)
                    {
                       ptr = data + ((y + iny) * w_dst) + inx;
                       for (x = 0; x < w_src; x++)
                         {
                            pixel = XGetPixel(xim, x, y);
                            pixel = ((pixel << 16) & 0xff0000) |
                               ((pixel) & 0x00ff00) |
                               ((pixel >> 16) & 0x0000ff);
                            if (XGetPixel(mxim, x, y))
                               pixel |= 0xff000000;
                            *ptr++ = pixel;
                         }
                    }
               }
             else
               {
                  for (y = 0; y < h_src; y++)
                    {
                       ptr = data + ((y + iny) * w_dst) + inx;
                       for (x = 0; x < w_src; x++)
                         {
                            pixel = XGetPixel(xim, x, y);
                            *ptr++ = 0xff000000 |
                               ((pixel << 16) & 0xff0000) |
                               ((pixel) & 0x00ff00) |
                               ((pixel >> 16) & 0x0000ff);
                         }
                    }
               }
          }
        else
          {
             if (mxim)
               {
                  for (y = 0; y < h_src; y++)
                    {
                       ptr = data + ((y + iny) * w_dst) + inx;
                       for (x = 0; x < w_src; x++)
                         {
                            pixel = XGetPixel(xim, x, y) & 0x00ffffff;
                            if (XGetPixel(mxim, x, y))
                               pixel |= 0xff000000;
                            *ptr++ = pixel;
                         }
                    }
               }
             else
               {
                  for (y = 0; y < h_src; y++)
                    {
                       ptr = data + ((y + iny) * w_dst) + inx;
                       for (x = 0; x < w_src; x++)
                         {
                            pixel = XGetPixel(xim, x, y);
                            *ptr++ = 0xff000000 | (pixel & 0x00ffffff);
                         }
                    }
               }
          }
        break;
     case 25:
        if (bgr)
          {
             if (mxim)
               {
                  for (y = 0; y < h_src; y++)
                    {
                       src =
                          (uint32_t *) (xim->data + (xim->bytes_per_line * y));
                       ptr = data + ((y + iny) * w_dst) + inx;
                       for (x = 0; x < w_src; x++)
                         {
                            pixel = ((*src << 16) & 0xff0000) |
                               ((*src) & 0x00ff00) | ((*src >> 16) & 0x0000ff);
                            if (XGetPixel(mxim, x, y))
                               pixel |= 0xff000000;
                            *ptr++ = pixel;
                            src++;
                         }
                    }
               }
             else
               {
                  for (y = 0; y < h_src; y++)
                    {
                       src =
                          (uint32_t *) (xim->data + (xim->bytes_per_line * y));
                       ptr = data + ((y + iny) * w_dst) + inx;
                       for (x = 0; x < w_src; x++)
                         {
                            *ptr++ = 0xff000000 |
                               ((*src << 16) & 0xff0000) |
                               ((*src) & 0x00ff00) | ((*src >> 16) & 0x0000ff);
                            src++;
                         }
                    }
               }
          }
        else
          {
             if (mxim)
               {
                  for (y = 0; y < h_src; y++)
                    {
                       src =
                          (uint32_t *) (xim->data + (xim->bytes_per_line * y));
                       ptr = data + ((y + iny) * w_dst) + inx;
                       for (x = 0; x < w_src; x++)
                         {
                            pixel = (*src) & 0x00ffffff;
                            if (XGetPixel(mxim, x, y))
                               pixel |= 0xff000000;
                            *ptr++ = pixel;
                            src++;
                         }
                    }
               }
             else
               {
                  for (y = 0; y < h_src; y++)
                    {
                       src =
                          (uint32_t *) (xim->data + (xim->bytes_per_line * y));
                       ptr = data + ((y + iny) * w_dst) + inx;
                       for (x = 0; x < w_src; x++)
                         {
                            *ptr++ = 0xff000000 | ((*src) & 0x00ffffff);
                            src++;
                         }
                    }
               }
          }
        break;
     case 30:
        if (bgr)
          {
             if (mxim)
               {
                  for (y = 0; y < h_src; y++)
                    {
                       src =
                          (uint32_t *) (xim->data + (xim->bytes_per_line * y));
                       ptr = data + ((y + iny) * w_dst) + inx;
                       for (x = 0; x < w_src; x++)
                         {
                            pixel = (((*src) & 0x000003ff) << 14 & 0x00ff0000) |
                               (((*src) & 0x000ffc00) >> 4 & 0x0000ff00) |
                               (((*src) & 0x3ff00000) >> 22 & 0x000000ff);
                            if (XGetPixel(mxim, x, y))
                               pixel |= 0xff000000;
                            *ptr++ = pixel;
                            src++;
                         }
                    }
               }
             else
               {
                  for (y = 0; y < h_src; y++)
                    {
                       src =
                          (uint32_t *) (xim->data + (xim->bytes_per_line * y));
                       ptr = data + ((y + iny) * w_dst) + inx;
                       for (x = 0; x < w_src; x++)
                         {
                            *ptr++ = 0xff000000 |
                               (((*src) & 0x000003ff) << 14 & 0x00ff0000) |
                               (((*src) & 0x000ffc00) >> 4 & 0x0000ff00) |
                               (((*src) & 0x3ff00000) >> 22 & 0x000000ff);
                            src++;
                         }
                    }
               }
          }
        else
          {
             if (mxim)
               {
                  for (y = 0; y < h_src; y++)
                    {
                       src =
                          (uint32_t *) (xim->data + (xim->bytes_per_line * y));
                       ptr = data + ((y + iny) * w_dst) + inx;
                       for (x = 0; x < w_src; x++)
                         {
                            pixel = (((*src) & 0x3ff00000) >> 6 & 0x00ff0000) |
                               (((*src) & 0x000ffc00) >> 4 & 0x0000ff00) |
                               (((*src) & 0x000003ff) >> 2 & 0x000000ff);
                            if (XGetPixel(mxim, x, y))
                               pixel |= 0xff000000;
                            *ptr++ = pixel;
                            src++;
                         }
                    }
               }
             else
               {
                  for (y = 0; y < h_src; y++)
                    {
                       src =
                          (uint32_t *) (xim->data + (xim->bytes_per_line * y));
                       ptr = data + ((y + iny) * w_dst) + inx;
                       for (x = 0; x < w_src; x++)
                         {
                            *ptr++ = 0xff000000 |
                               (((*src) & 0x3ff00000) >> 6 & 0x00ff0000) |
                               (((*src) & 0x000ffc00) >> 4 & 0x0000ff00) |
                               (((*src) & 0x000003ff) >> 2 & 0x000000ff);
                            src++;
                         }
                    }
               }
          }
        break;
     case 32:
        if (bgr)
          {
             if (mxim)
               {
                  for (y = 0; y < h_src; y++)
                    {
                       src =
                          (uint32_t *) (xim->data + (xim->bytes_per_line * y));
                       ptr = data + ((y + iny) * w_dst) + inx;
                       for (x = 0; x < w_src; x++)
                         {
                            pixel = SWAP32(*src);
                            if (!XGetPixel(mxim, x, y))
                               pixel &= 0x00ffffff;
                            *ptr++ = pixel;
                            src++;
                         }
                    }
               }
             else
               {
                  for (y = 0; y < h_src; y++)
                    {
                       src =
                          (uint32_t *) (xim->data + (xim->bytes_per_line * y));
                       ptr = data + ((y + iny) * w_dst) + inx;
                       for (x = 0; x < w_src; x++)
                         {
                            *ptr++ = SWAP32(*src);
                            src++;
                         }
                    }
               }
          }
        else
          {
             if (mxim)
               {
                  for (y = 0; y < h_src; y++)
                    {
                       src =
                          (uint32_t *) (xim->data + (xim->bytes_per_line * y));
                       ptr = data + ((y + iny) * w_dst) + inx;
                       for (x = 0; x < w_src; x++)
                         {
                            pixel = *src++;
                            if (!XGetPixel(mxim, x, y))
                               pixel &= 0x00ffffff;
                            *ptr++ = pixel;
                         }
                    }
               }
             else
               {
                  for (y = 0; y < h_src; y++)
                    {
                       src =
                          (uint32_t *) (xim->data + (xim->bytes_per_line * y));
                       ptr = data + ((y + iny) * w_dst) + inx;
                       for (x = 0; x < w_src; x++)
                         {
                            *ptr++ = *src++;
                         }
                    }
               }
          }
        break;
     default:
        break;
     }

   if (grab)
      XUngrabServer(d);
}

static              Pixmap
_WindowGetShapeMask(Display * d, Window p,
                    int x, int y, int w, int h, int ww, int wh)
{
   Pixmap              mask;
   XRectangle         *rect;
   int                 rect_num, rect_ord, i;
   XGCValues           gcv;
   GC                  mgc;

   mask = None;

   rect = XShapeGetRectangles(d, p, ShapeBounding, &rect_num, &rect_ord);
   if (!rect)
      return mask;

   if (rect_num == 1 &&
       rect[0].x == 0 && rect[0].y == 0 &&
       rect[0].width == ww && rect[0].height == wh)
      goto done;

   mask = XCreatePixmap(d, p, w, h, 1);

   gcv.foreground = 0;
   gcv.graphics_exposures = False;
   mgc = XCreateGC(d, mask, GCForeground | GCGraphicsExposures, &gcv);

   XFillRectangle(d, mask, mgc, 0, 0, w, h);

   XSetForeground(d, mgc, 1);
   for (i = 0; i < rect_num; i++)
      XFillRectangle(d, mask, mgc, rect[i].x - x, rect[i].y - y,
                     rect[i].width, rect[i].height);

   if (mgc)
      XFreeGC(d, mgc);

 done:
   XFree(rect);

   return mask;
}

int
__imlib_GrabDrawableToRGBA(uint32_t * data, int x_dst, int y_dst, int w_dst,
                           int h_dst, Display * d, Drawable p, Pixmap m_,
                           Visual * v, Colormap cm, int depth, int x_src,
                           int y_src, int w_src, int h_src, char *pdomask,
                           int grab)
{
   XErrorHandler       prev_erh = NULL;
   XWindowAttributes   xatt, ratt;
   char                is_pixmap = 0, is_shm = 0, is_mshm = 0;
   char                domask;
   int                 i;
   int                 src_x, src_y, src_w, src_h;
   int                 width, height, clipx, clipy;
   Pixmap              m = m_;
   XShmSegmentInfo     shminfo, mshminfo;
   XImage             *xim, *mxim;
   XColor              cols[256];

   domask = (pdomask) ? *pdomask : 0;

   h_dst = 0;                   /* h_dst is not used */

   if (grab)
      XGrabServer(d);
   XSync(d, False);
   prev_erh = XSetErrorHandler(Tmp_HandleXError);
   _x_err = 0;

   /* lets see if its a pixmap or not */
   XGetWindowAttributes(d, p, &xatt);
   XSync(d, False);
   if (_x_err)
      is_pixmap = 1;
   /* reset our error handler */
   XSetErrorHandler((XErrorHandler) prev_erh);

   if (is_pixmap)
     {
        Window              dw;

        XGetGeometry(d, p, &dw, &src_x, &src_y,
                     (unsigned int *)&src_w, (unsigned int *)&src_h,
                     (unsigned int *)&src_x, (unsigned int *)&xatt.depth);
        src_x = 0;
        src_y = 0;
     }
   else
     {
        Window              dw;

        XGetWindowAttributes(d, xatt.root, &ratt);
        XTranslateCoordinates(d, p, xatt.root, 0, 0, &src_x, &src_y, &dw);
        src_w = xatt.width;
        src_h = xatt.height;
        if ((xatt.map_state != IsViewable) && (xatt.backing_store == NotUseful))
          {
             if (grab)
                XUngrabServer(d);
             return 0;
          }
     }

   /* clip to the drawable tree and screen */
   clipx = 0;
   clipy = 0;
   width = src_w - x_src;
   height = src_h - y_src;
   if (width > w_src)
      width = w_src;
   if (height > h_src)
      height = h_src;

   if (!is_pixmap)
     {
        if ((src_x + x_src + width) > ratt.width)
           width = ratt.width - (src_x + x_src);
        if ((src_y + y_src + height) > ratt.height)
           height = ratt.height - (src_y + y_src);
     }

   if (x_src < 0)
     {
        clipx = -x_src;
        width += x_src;
        x_src = 0;
     }

   if (y_src < 0)
     {
        clipy = -y_src;
        height += y_src;
        y_src = 0;
     }

   if (!is_pixmap)
     {
        if ((src_x + x_src) < 0)
          {
             clipx -= (src_x + x_src);
             width += (src_x + x_src);
             x_src = -src_x;
          }
        if ((src_y + y_src) < 0)
          {
             clipy -= (src_y + y_src);
             height += (src_y + y_src);
             y_src = -src_y;
          }
     }

   if ((width <= 0) || (height <= 0))
     {
        if (grab)
           XUngrabServer(d);
        return 0;
     }

   w_src = width;
   h_src = height;

   if ((!is_pixmap) && (domask) && (m == None))
      m = _WindowGetShapeMask(d, p, x_src, y_src, w_src, h_src,
                              xatt.width, xatt.height);

   /* Create an Ximage (shared or not) */
   xim = __imlib_ShmGetXImage(d, v, p, xatt.depth, x_src, y_src, w_src, h_src,
                              &shminfo);
   is_shm = !!xim;

   if (!xim)
      xim = XGetImage(d, p, x_src, y_src, w_src, h_src, 0xffffffff, ZPixmap);
   if (!xim)
     {
        if (grab)
           XUngrabServer(d);
        return 0;
     }

   mxim = NULL;
   if ((m) && (domask))
     {
        mxim = __imlib_ShmGetXImage(d, v, m, 1, 0, 0, w_src, h_src, &mshminfo);
        is_mshm = !!mxim;
        if (!mxim)
           mxim = XGetImage(d, m, 0, 0, w_src, h_src, 0xffffffff, ZPixmap);
     }

   if ((is_shm) || (is_mshm))
     {
        XSync(d, False);
        if (grab)
           XUngrabServer(d);
        XSync(d, False);
     }
   else if (grab)
      XUngrabServer(d);

   if ((xatt.depth == 1) && (!cm) && (is_pixmap))
     {
        rtab[0] = 255;
        gtab[0] = 255;
        btab[0] = 255;
        rtab[1] = 0;
        gtab[1] = 0;
        btab[1] = 0;
     }
   else if (xatt.depth <= 8)
     {
        if (!cm)
          {
             if (is_pixmap)
               {
                  cm = DefaultColormap(d, DefaultScreen(d));
               }
             else
               {
                  cm = xatt.colormap;
                  if (cm == None)
                     cm = ratt.colormap;
               }
          }

        for (i = 0; i < (1 << xatt.depth); i++)
          {
             cols[i].pixel = i;
             cols[i].flags = DoRed | DoGreen | DoBlue;
          }
        XQueryColors(d, cm, cols, 1 << xatt.depth);
        for (i = 0; i < (1 << xatt.depth); i++)
          {
             rtab[i] = cols[i].red >> 8;
             gtab[i] = cols[i].green >> 8;
             btab[i] = cols[i].blue >> 8;
          }
     }

   __imlib_GrabXImageToRGBA(data, x_dst + clipx, y_dst + clipy, w_dst, h_dst,
                            d, xim, mxim, v, xatt.depth, x_src, y_src, w_src,
                            h_src, 0);

   /* destroy the Ximage */
   if (is_shm)
      __imlib_ShmDestroyXImage(d, xim, &shminfo);
   else
      XDestroyImage(xim);

   if (mxim)
     {
        if (is_mshm)
           __imlib_ShmDestroyXImage(d, mxim, &mshminfo);
        else
           XDestroyImage(mxim);
     }

   if (m != None && m != m_)
      XFreePixmap(d, m);

   if (pdomask)
     {
        /* Set domask according to whether or not we have useful alpha data */
        if (xatt.depth == 32)
           *pdomask = 1;
        else if (m == None)
           *pdomask = 0;
     }

   return 1;
}

int
__imlib_GrabDrawableScaledToRGBA(uint32_t * data, int nu_x_dst, int nu_y_dst,
                                 int w_dst, int h_dst,
                                 Display * d, Drawable p, Pixmap m_,
                                 Visual * v, Colormap cm, int depth,
                                 int x_src, int y_src, int w_src, int h_src,
                                 char *pdomask, int grab)
{
   int                 rc;
   int                 h_tmp, i, xx;
   XGCValues           gcv;
   GC                  gc, mgc = NULL;
   Pixmap              m = m_;
   Pixmap              psc, msc;

   h_tmp = h_dst > h_src ? h_dst : h_src;

   gcv.foreground = 0;
   gcv.subwindow_mode = IncludeInferiors;
   gcv.graphics_exposures = False;
   gc = XCreateGC(d, p, GCSubwindowMode | GCGraphicsExposures, &gcv);

   if (*pdomask && m == None)
     {
        m = _WindowGetShapeMask(d, p, 0, 0, w_src, h_src, w_src, h_src);
        if (m == None)
           *pdomask = 0;
     }

   if (w_dst == w_src && h_dst == h_src)
     {
        if (x_src == 0 && y_src == 0)
          {
             psc = p;
          }
        else
          {
             psc = XCreatePixmap(d, p, w_src, h_tmp, depth);
             XCopyArea(d, p, psc, gc, x_src, y_src, w_src, h_src, 0, 0);
          }
        msc = m;
     }
   else
     {
        psc = XCreatePixmap(d, p, w_dst, h_tmp, depth);

        if (*pdomask)
          {
             msc = XCreatePixmap(d, p, w_dst, h_tmp, 1);
             mgc = XCreateGC(d, msc, GCForeground | GCGraphicsExposures, &gcv);
          }
        else
           msc = None;

        for (i = 0; i < w_dst; i++)
          {
             xx = (w_src * i) / w_dst;
             XCopyArea(d, p, psc, gc, x_src + xx, y_src, 1, h_src, i, 0);
             if (msc != None)
                XCopyArea(d, m, msc, mgc, xx, 0, 1, h_src, i, 0);
          }
        if (h_dst > h_src)
          {
             for (i = h_dst - 1; i > 0; i--)
               {
                  xx = (h_src * i) / h_dst;
                  if (xx == i)
                     continue;  /* Don't copy onto self */
                  XCopyArea(d, psc, psc, gc, 0, xx, w_dst, 1, 0, i);
                  if (msc != None)
                     XCopyArea(d, msc, msc, mgc, 0, xx, w_dst, 1, 0, i);
               }
          }
        else
          {
             for (i = 0; i < h_dst; i++)
               {
                  xx = (h_src * i) / h_dst;
                  if (xx == i)
                     continue;  /* Don't copy onto self */
                  XCopyArea(d, psc, psc, gc, 0, xx, w_dst, 1, 0, i);
                  if (msc != None)
                     XCopyArea(d, msc, msc, mgc, 0, xx, w_dst, 1, 0, i);
               }
          }
     }

   rc = __imlib_GrabDrawableToRGBA(data, 0, 0, w_dst, h_dst, d, psc, msc,
                                   v, cm, depth, 0, 0, w_dst, h_dst,
                                   pdomask, grab);

   if (mgc)
      XFreeGC(d, mgc);
   if (msc != None && msc != m)
      XFreePixmap(d, msc);
   if (m != None && m != m_)
      XFreePixmap(d, m);
   XFreeGC(d, gc);
   if (psc != p)
      XFreePixmap(d, psc);

   return rc;
}
