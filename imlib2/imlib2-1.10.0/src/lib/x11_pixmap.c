#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>

#include "blend.h"
#include "colormod.h"
#include "image.h"
#include "x11_pixmap.h"
#include "x11_rend.h"

typedef struct _ImlibImagePixmap {
   int                 w, h;
   Pixmap              pixmap, mask;
   Display            *display;
   Visual             *visual;
   int                 depth;
   int                 source_x, source_y, source_w, source_h;
   Colormap            colormap;
   char                antialias, hi_quality, dither_mask;
   ImlibBorder         border;
   ImlibImage         *image;
   char               *file;
   char                dirty;
   int                 references;
   uint64_t            modification_count;
   struct _ImlibImagePixmap *next;
} ImlibImagePixmap;

static ImlibImagePixmap *pixmaps = NULL;

/* create a pixmap cache data struct */
static ImlibImagePixmap *
__imlib_ProduceImagePixmap(void)
{
   ImlibImagePixmap   *ip;

   ip = calloc(1, sizeof(ImlibImagePixmap));

   return ip;
}

/* free a pixmap cache data struct and the pixmaps in it */
static void
__imlib_ConsumeImagePixmap(ImlibImagePixmap * ip)
{
#ifdef DEBUG_CACHE
   fprintf(stderr,
           "[Imlib2]  Deleting pixmap.  Reference count is %d, pixmap 0x%08lx, mask 0x%08lx\n",
           ip->references, ip->pixmap, ip->mask);
#endif
   if (ip->pixmap)
      XFreePixmap(ip->display, ip->pixmap);
   if (ip->mask)
      XFreePixmap(ip->display, ip->mask);
   free(ip->file);
   free(ip);
}

static ImlibImagePixmap *
__imlib_FindCachedImagePixmap(ImlibImage * im, int w, int h, Display * d,
                              Visual * v, int depth, int sx, int sy, int sw,
                              int sh, Colormap cm, char aa, char hiq,
                              char dmask, uint64_t modification_count)
{
   ImlibImagePixmap   *ip, *ip_prev;

   for (ip = pixmaps, ip_prev = NULL; ip; ip_prev = ip, ip = ip->next)
     {
        /* if all the pixmap attributes match */
        if ((ip->w == w) && (ip->h == h) && (ip->depth == depth) && (!ip->dirty)
            && (ip->visual == v) && (ip->display == d)
            && (ip->source_x == sx) && (ip->source_x == sy)
            && (ip->source_w == sw) && (ip->source_h == sh)
            && (ip->colormap == cm) && (ip->antialias == aa)
            && (ip->modification_count == modification_count)
            && (ip->dither_mask == dmask)
            && (ip->border.left == im->border.left)
            && (ip->border.right == im->border.right)
            && (ip->border.top == im->border.top)
            && (ip->border.bottom == im->border.bottom) &&
            (((im->file) && (ip->file) && !strcmp(im->file, ip->file)) ||
             ((!im->file) && (!ip->file) && (im == ip->image))))
          {
             /* move the pixmap to the head of the pixmap list */
             if (ip_prev)
               {
                  ip_prev->next = ip->next;
                  ip->next = pixmaps;
                  pixmaps = ip;
               }
             return ip;
          }
     }
   return NULL;
}

/* add a pixmap cahce struct to the pixmap cache (at the start of course */
static ImlibImagePixmap *
__imlib_AddImagePixmapToCache(ImlibImage * im, Pixmap pmap, Pixmap mask,
                              int w, int h,
                              Display * d, Visual * v, int depth,
                              int sx, int sy, int sw, int sh,
                              Colormap cm, char aa, char hiq, char dmask,
                              uint64_t modification_count)
{
   ImlibImagePixmap   *ip;

   ip = __imlib_ProduceImagePixmap();
   ip->visual = v;
   ip->depth = depth;
   ip->image = im;
   if (im->file)
      ip->file = strdup(im->file);
   ip->border.left = im->border.left;
   ip->border.right = im->border.right;
   ip->border.top = im->border.top;
   ip->border.bottom = im->border.bottom;
   ip->colormap = cm;
   ip->display = d;
   ip->w = w;
   ip->h = h;
   ip->source_x = sx;
   ip->source_y = sy;
   ip->source_w = sw;
   ip->source_h = sh;
   ip->antialias = aa;
   ip->modification_count = modification_count;
   ip->dither_mask = dmask;
   ip->hi_quality = hiq;
   ip->references = 1;
   ip->pixmap = pmap;
   ip->mask = mask;

   ip->next = pixmaps;
   pixmaps = ip;

   return ip;
}

void
__imlib_PixmapUnrefImage(const ImlibImage * im)
{
   ImlibImagePixmap   *ip;

   for (ip = pixmaps; ip; ip = ip->next)
     {
        if (ip->image == im)
          {
             ip->image = NULL;
             ip->dirty = 1;
          }
     }
}

/* remove a pixmap cache struct from the pixmap cache */
static void
__imlib_RemoveImagePixmapFromCache(ImlibImagePixmap * ip_del)
{
   ImlibImagePixmap   *ip, *ip_prev;

   for (ip = pixmaps, ip_prev = NULL; ip; ip_prev = ip, ip = ip->next)
     {
        if (ip == ip_del)
          {
             if (ip_prev)
                ip_prev->next = ip->next;
             else
                pixmaps = ip->next;
             return;
          }
     }
}

/* clean out 0 reference count & dirty pixmaps from the cache */
void
__imlib_CleanupImagePixmapCache(void)
{
   ImlibImagePixmap   *ip, *ip_next, *ip_del;
   int                 current_cache;

   current_cache = __imlib_CurrentCacheSize();

   for (ip = pixmaps; ip; ip = ip_next)
     {
        ip_next = ip->next;
        if ((ip->references <= 0) && (ip->dirty))
          {
             __imlib_RemoveImagePixmapFromCache(ip);
             __imlib_ConsumeImagePixmap(ip);
          }
     }

   while (current_cache > __imlib_GetCacheSize())
     {
        for (ip = pixmaps, ip_del = NULL; ip; ip = ip->next)
          {
             if (ip->references <= 0)
                ip_del = ip;
          }
        if (!ip_del)
           break;

        __imlib_RemoveImagePixmapFromCache(ip_del);
        __imlib_ConsumeImagePixmap(ip_del);

        current_cache = __imlib_CurrentCacheSize();
     }
}

/* find an imagepixmap cache entry by the display and pixmap id */
static ImlibImagePixmap *
__imlib_FindImlibImagePixmapByID(Display * d, Pixmap p)
{
   ImlibImagePixmap   *ip;

   for (ip = pixmaps; ip; ip = ip->next)
     {
        /* if all the pixmap ID & Display match */
        if ((ip->pixmap == p) && (ip->display == d))
          {
#ifdef DEBUG_CACHE
             fprintf(stderr,
                     "[Imlib2]  Match found.  Reference count is %d, pixmap 0x%08lx, mask 0x%08lx\n",
                     ip->references, ip->pixmap, ip->mask);
#endif
             return ip;
          }
     }
   return NULL;
}

/* free a cached pixmap */
void
__imlib_FreePixmap(Display * d, Pixmap p)
{
   ImlibImagePixmap   *ip;

   /* find the pixmap in the cache by display and id */
   ip = __imlib_FindImlibImagePixmapByID(d, p);
   if (ip)
     {
        /* if tis positive reference count */
        if (ip->references > 0)
          {
             /* dereference it by one */
             ip->references--;
#ifdef DEBUG_CACHE
             fprintf(stderr,
                     "[Imlib2]  Reference count is now %d for pixmap 0x%08lx\n",
                     ip->references, ip->pixmap);
#endif
             /* if it becaume 0 reference count - clean the cache up */
             if (ip->references == 0)
                __imlib_CleanupImagePixmapCache();
          }
     }
   else
     {
#ifdef DEBUG_CACHE
        fprintf(stderr, "[Imlib2]  Pixmap 0x%08lx not found.  Freeing.\n", p);
#endif
        XFreePixmap(d, p);
     }
}

/* mark all pixmaps generated from this image as dirty so the cache code */
/* wont pick up on them again since they are now invalid since the original */
/* data they were generated from has changed */
void
__imlib_DirtyPixmapsForImage(const ImlibImage * im)
{
   ImlibImagePixmap   *ip;

   for (ip = pixmaps; ip; ip = ip->next)
     {
        /* if image matches */
        if (ip->image == im)
           ip->dirty = 1;
     }
   __imlib_CleanupImagePixmapCache();
}

int
__imlib_PixmapCacheSize(void)
{
   int                 current_cache = 0;
   ImlibImagePixmap   *ip, *ip_next;

   for (ip = pixmaps; ip; ip = ip_next)
     {
        ip_next = ip->next;

        /* if the pixmap has 0 references */
        if (ip->references == 0)
          {
             /* if the image is invalid */
             if (ip->dirty ||
                 (ip->image && IM_FLAG_ISSET(ip->image, F_INVALID)))
               {
                  __imlib_RemoveImagePixmapFromCache(ip);
                  __imlib_ConsumeImagePixmap(ip);
               }
             else
               {
                  /* add the pixmap data size to the cache size */
                  if (ip->pixmap)
                    {
                       if (ip->depth < 8)
                          current_cache += ip->w * ip->h * (ip->depth / 8);
                       else if (ip->depth == 8)
                          current_cache += ip->w * ip->h;
                       else if (ip->depth <= 16)
                          current_cache += ip->w * ip->h * 2;
                       else if (ip->depth <= 32)
                          current_cache += ip->w * ip->h * 4;
                    }
                  /* if theres a mask add it too */
                  if (ip->mask)
                     current_cache += ip->w * ip->h / 8;
               }
          }
     }

   return current_cache;
}

char
__imlib_CreatePixmapsForImage(Display * d, Drawable w, Visual * v, int depth,
                              Colormap cm, ImlibImage * im, Pixmap * p,
                              Mask * m, int sx, int sy, int sw, int sh, int dw,
                              int dh, char antialias, char hiq,
                              char dither_mask, int mat,
                              ImlibColorModifier * cmod)
{
   ImlibImagePixmap   *ip = NULL;
   Pixmap              pmap = 0;
   Pixmap              mask = 0;
   long long           mod_count = 0;

   if (cmod)
      mod_count = cmod->modification_count;
   ip = __imlib_FindCachedImagePixmap(im, dw, dh, d, v, depth, sx, sy,
                                      sw, sh, cm, antialias, hiq, dither_mask,
                                      mod_count);
   if (ip)
     {
        if (p)
           *p = ip->pixmap;
        if (m)
           *m = ip->mask;
        ip->references++;
#ifdef DEBUG_CACHE
        fprintf(stderr,
                "[Imlib2]  Match found in cache.  Reference count is %d, pixmap 0x%08lx, mask 0x%08lx\n",
                ip->references, ip->pixmap, ip->mask);
#endif
        return 2;
     }
   if (p)
     {
        pmap = XCreatePixmap(d, w, dw, dh, depth);
        *p = pmap;
     }
   if (m)
     {
        if (im->has_alpha)
           mask = XCreatePixmap(d, w, dw, dh, 1);
        *m = mask;
     }
   __imlib_RenderImage(d, im, pmap, mask, v, cm, depth, sx, sy, sw, sh, 0, 0,
                       dw, dh, antialias, hiq, 0, dither_mask, mat, cmod,
                       OP_COPY);
   ip = __imlib_AddImagePixmapToCache(im, pmap, mask,
                                      dw, dh, d, v, depth, sx, sy,
                                      sw, sh, cm, antialias, hiq, dither_mask,
                                      mod_count);
#ifdef DEBUG_CACHE
   fprintf(stderr,
           "[Imlib2]  Created pixmap.  Reference count is %d, pixmap 0x%08lx, mask 0x%08lx\n",
           ip->references, ip->pixmap, ip->mask);
#endif
   return 1;
}
